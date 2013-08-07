/*
(c) Copyright 2013 Hewlett-Packard Development Company, L.P.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
package com.android.wfds.discoveryservice.parsers;

import android.util.Log;

import java.io.UnsupportedEncodingException;
import java.net.DatagramPacket;
import java.util.ArrayList;

public class DnsParser {
    private static final String TAG = DnsParser.class.getSimpleName();

    private static final int MINIMUM_PACKET_SIZE = 12;
    private static final int BYTE_LENGTH = 8;
    private static final int INT16_LENGTH = 16;
    private static final int INT32_LENGTH = 32;
    private static final int BYTE_MASK = 0x000000FF;
    private static final int NAME_POINTER_MASK = 0xFFFFFFC0;
    private static final String NAME_ENCODING = "UTF-8";

    private byte[] data;
    private int length;
    private int offset;

    public DnsPacket parse(DatagramPacket packet) throws DnsException {
        this.data = packet.getData();
        this.length = packet.getLength();
        this.offset = packet.getOffset();
        if ((this.length - this.offset) < MINIMUM_PACKET_SIZE) {
            throw new DnsException("Invalid mDNS packet: insufficient data.");
        }
        int id = this.parseUInt16();
        int flags = this.parseUInt16();
        int nQuestions = this.parseUInt16();
        int nAnswers = this.parseUInt16();
        int nAuthorities = this.parseUInt16();
        int nAdditionals = this.parseUInt16();
        DnsPacket.Question[] questions = this.parseQuestions(nQuestions);
        DnsPacket.Entry[] answers = this.parseEntries(nAnswers);
        DnsPacket.Entry[] authorities = this.parseEntries(nAuthorities);
        DnsPacket.Entry[] additionals = this.parseEntries(nAdditionals);

        return new DnsPacket(id, flags, questions, answers, authorities, additionals);
    }

    private DnsPacket.Question[] parseQuestions(int nQuestions) throws DnsException {
        if (nQuestions > 0) {
            ArrayList<DnsPacket.Question> buffer = new ArrayList<DnsPacket.Question>(nQuestions);

            for (int i = 0; i < nQuestions; i++) {
                buffer.add(this.parseQuestion());
            }
            return buffer.toArray(new DnsPacket.Question[buffer.size()]);
        }
        return new DnsPacket.Question[0];
    }

    private final DnsPacket.Entry[] parseEntries(int nEntries) throws DnsException {
        ArrayList<DnsPacket.Entry> entries = new ArrayList<DnsPacket.Entry>(nEntries);

        for (int i = 0; i < nEntries; i++) {
            entries.add(this.parseEntry());
        }
        return entries.toArray(new DnsPacket.Entry[entries.size()]);
    }

    private final DnsPacket.Question parseQuestion() throws DnsException {
        DnsPacket.Name name = this.parseName();
        int type = this.parseUInt16();
        int clazz = this.parseUInt16();
        DnsPacket.ResourceType resourceType = DnsPacket.ResourceType.valueOf(type);

        return new DnsPacket.Question(name, resourceType, clazz);
    }

    private final DnsPacket.Entry parseEntry() throws DnsException {
        DnsPacket.Name name = this.parseName();
        int type = this.parseUInt16();
        int clazz = this.parseUInt16();
        int ttl = this.parseInt32();
        int len = this.parseUInt16();
        DnsPacket.ResourceType resourceType = DnsPacket.ResourceType.valueOf(type);

        switch (resourceType) {
            case A:
            case AAAA:
                return new DnsPacket.Address(name, resourceType, clazz, ttl, this.parseBytes(len));
            case CNAME:
            case PTR:
                return new DnsPacket.Ptr(name, resourceType, clazz, ttl, this.parseName());
            case TXT:
                return new DnsPacket.Txt(name, resourceType, clazz, ttl, this.parseBytes(len));
            case SRV:
                int priority = this.parseUInt16();
                int weight = this.parseUInt16();
                int port = this.parseUInt16();
                DnsPacket.Name target = this.parseName();

                return new DnsPacket.Srv(name, resourceType, clazz, ttl, priority, weight,
                        port, target);
            default:
                return new DnsPacket.GenericEntry(name, resourceType, clazz, ttl,
                        this.parseBytes(len));
        }
    }

    private final int parseUInt8() throws DnsException {
        return this.data[this.offset++] & BYTE_MASK;
    }

    private final int parseUInt16() throws DnsException {
        return this.parseInt(INT16_LENGTH);
    }

    private final int parseInt32() throws DnsException {
        return this.parseInt(INT32_LENGTH);
    }

    private final int parseInt(int nBits) throws DnsException {
        int nBytes = nBits / BYTE_LENGTH;
        int intNumber = 0;

        if ((this.length - this.offset) < nBytes) {
            throw new DnsException("Failed to read an int field: insufficient data.");
        }
        for (int i = 0; i < nBytes; i++) {
            int shiftBits = (nBytes - i - 1) * BYTE_LENGTH;

            intNumber |= (this.parseUInt8() << shiftBits);
        }
        return intNumber;
    }

    private final byte[] parseBytes(int len) throws DnsException {
        if ((this.length - this.offset) < len) {
            throw new DnsException("Failed to read byte array: insufficient data.");
        }
        byte[] bytes = new byte[len];

        System.arraycopy(this.data, this.offset, bytes, 0, len);
        this.offset += len;
        return bytes;
    }

    private DnsPacket.CompressedName parseName() throws DnsException {
        if ((this.length - this.offset) < 1) {
            throw new DnsException("Failed to read a name: insufficient data.");
        }
        DnsPacket.CompressedName name = this.readName(this.offset);

        this.offset += name.getSizeInBytes();
        return name;
    }

    private DnsPacket.CompressedName readName(int nameDataOffset) throws DnsException {
        int dataOffset = nameDataOffset;
        ArrayList<DnsPacket.NameSection> sectionList = new ArrayList<DnsPacket.NameSection>();
        DnsPacket.NameSection section = null;

        do {
            section = this.readNameSection(dataOffset);
            sectionList.add(section);
            dataOffset += section.getSizeInBytes();
        } while (!section.isEmpty() && !section.isPointer());
        return new DnsPacket.CompressedName(sectionList.toArray(new DnsPacket.NameSection[sectionList.size()]));
    }

    private DnsPacket.NameSection readNameSection(int dataOffset) throws DnsException {
        byte labelLength = this.data[dataOffset];

        if ((labelLength & NAME_POINTER_MASK) == NAME_POINTER_MASK) {
            return this.readNamePointer(dataOffset);
        } else if ((labelLength & NAME_POINTER_MASK) == 0) {
            return this.readNameLabel(dataOffset, labelLength);
        }
        Log.w(TAG, "The two last bits are not 00 nor 11. Will consider it is a regular length: " + labelLength);
        return this.readNameLabel(dataOffset, labelLength);
    }

    private DnsPacket.NameLabel readNameLabel(int dataOffset, int labelLength) throws DnsException {
        byte[] labelBuffer = new byte[labelLength];

        System.arraycopy(this.data, dataOffset + 1, labelBuffer, 0, labelLength);
        try {
            return new DnsPacket.NameLabel(new String(labelBuffer, NAME_ENCODING));
        } catch (UnsupportedEncodingException exc) {
            throw new DnsException("Unsupported encoding to parse DNS name: " + NAME_ENCODING, exc);
        }
    }

    private DnsPacket.NamePointer readNamePointer(int dataOffset)  throws DnsException {
        int nameOffset = this.readNameOffset(dataOffset);

        return new DnsPacket.NamePointer(this.readName(nameOffset));
    }

    private int readNameOffset(int dataOffset) {
        int first = (this.data[dataOffset] & ~NAME_POINTER_MASK);
        int second = this.data[dataOffset + 1] & BYTE_MASK;

        return ((first << BYTE_LENGTH) | second);
    }
}
