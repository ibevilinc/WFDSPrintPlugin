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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

public class DnsPacket {

    public enum ResourceType {
        UNKNOWN(-1),

        // IPv4 address
        A(1),
        NS(2),
        MD(3),
        MF(4),

        // Canonical name
        CNAME(5),
        SOA(6),
        MB(7),
        MG(8),
        MR(9),
        NULL(10),
        WKS(11),

        // Domain name pointer
        PTR(12),
        HINFO(13),
        MINFO(14),
        MX(15),

        // Arbitrary text string
        TXT(16),

        // IPv6 address (Thomson)
        AAAA(28),

        // Service (RFC 2782)
        SRV(33),

        // Question-only types
        AXFR(252),
        MAILB(253),
        MAILA(254),
        STAR(255);

        private int code;

        private ResourceType(int code) {
            this.code = code;
        }

        public static ResourceType valueOf(int code) {
            for (ResourceType rt : values()) {
                if (rt.code == code) {
                    return rt;
                }
            }
            return UNKNOWN;
        }
    }

    // The most significant bit of the RRCLASS fields can be used either as
    // a request for unicast responses (in question entries) or as an
    // indicator that the resource is unique and all other entries in the
    // cache should be flushed (in response entries). For more details, see
    // draft on Multicast DNS - Cheshire.
    private static final int RRCLASS_MASK = 0x00007FFF;

    private int id;
    private int flags;
    private Question[] questions;
    private Entry[] answers;
    private Entry[] authorities;
    private Entry[] additionals;

    public DnsPacket(int id, int flags, Question[] questions, Entry[] answers, Entry[] authorities,
                     Entry[] additionals) {
        this.id = id;
        this.flags = flags;
        this.questions = questions;
        this.answers = answers;
        this.authorities = authorities;
        this.additionals = additionals;
    }

    public int getId() {
        return this.id;
    }

    public int getFlags() {
        return this.flags;
    }

    public Question[] getQuestions() {
        return this.questions;
    }

    public Entry[] getAnswers() {
        return this.answers;
    }

    public Entry[] getAuthorities() {
        return this.authorities;
    }

    public Entry[] getAdditionals() {
        return this.additionals;
    }


    public static interface Name {

        String[] getLabels();
    }


    public static class CompressedName implements Name {
        private NameSection[] sections;
        private String[] labels;
        private String string;

        public CompressedName(NameSection[] sections) {
            this.sections = sections;
        }

        public int getSizeInBytes() {
            int size = 0;

            for (NameSection section : this.sections) {
                size += section.getSizeInBytes();
            }
            return size;
        }

        public String[] getLabels() {
            if (this.labels == null) {
                List<String> labelList = new ArrayList<String>();

                for (NameSection section : this.sections) {
                    labelList.addAll(Arrays.asList(section.getLabels()));
                }
                this.labels = labelList.toArray(new String[labelList.size()]);
            }
            return this.labels;
        }

        @Override
        public String toString() {
            if (this.string == null) {
                StringBuilder builder = new StringBuilder();

                for (int i = 0; i < this.sections.length; i++) {
                    String sectionString = this.sections[i].toString();

                    if ((i > 0) && (sectionString.length() > 0)) {
                        builder.append(".");
                    }
                    builder.append(sectionString);
                }
                this.string = builder.toString();
            }
            return this.string;
        }

        @Override
        public boolean equals(Object thatObject) {
            if (this == thatObject) {
                return true;
            }
            if (!(thatObject instanceof CompressedName)) {
                return false;
            }
            return this.toString().equals(thatObject.toString());
        }

        @Override
        public int hashCode() {
            return this.toString().hashCode();
        }
    }


    public static interface NameSection extends Name {

        int getSizeInBytes();

        boolean isEmpty();

        boolean isPointer();
    }


    public static class NameLabel implements NameSection {
        private String string;

        public NameLabel(String string) {
            this.string = string;
        }

        public int getSizeInBytes() {
            return this.string.length() + 1;
        }

        public String[] getLabels() {
            return new String[] {this.string};
        }

        public boolean isEmpty() {
            return this.string.length() == 0;
        }

        public boolean isPointer() {
            return false;
        }

        @Override
        public String toString() {
            return this.string;
        }
    }


    public static class NamePointer implements NameSection {
        private Name pointedName;

        public NamePointer(Name pointedName) {
            this.pointedName = pointedName;
        }

        public int getSizeInBytes() {
            return 2;
        }

        public String[] getLabels() {
            return this.pointedName.getLabels();
        }

        public boolean isEmpty() {
            return false;
        }

        public boolean isPointer() {
            return true;
        }

        @Override
        public String toString() {
            return this.pointedName.toString();
        }
    }


    public static abstract class BasicEntry {
        private static final String FORMAT = "DNS basic entry [name=%s; type=%s; class=%d]";

        private Name name;
        private ResourceType type;
        private int clazz;
        private boolean flag;

        protected BasicEntry(Name name, ResourceType type, int clazz) {
            this.name = name;
            this.type = type;
            this.clazz = (clazz & RRCLASS_MASK);
            this.flag = ((clazz & ~RRCLASS_MASK) != 0);
        }

        public Name getName() {
            return this.name;
        }

        public ResourceType getType() {
            return this.type;
        }

        public int getClazz() {
            return this.clazz;
        }

        public boolean isUnique() {
            return this.flag;
        }

        public boolean isUnicastQuestion() {
            return this.flag;
        }

        @Override
        public String toString() {
            return String.format(Locale.US, FORMAT, this.name, this.type, Integer.valueOf(this.clazz));
        }
    }


    public static class Question extends BasicEntry {

        public Question(Name name, ResourceType type, int clazz) {
            super(name, type, clazz);
        }
    }


    public static abstract class Entry extends BasicEntry {
        private int ttl;

        public Entry(Name name, ResourceType type, int clazz, int ttl) {
            super(name, type, clazz);
            this.ttl = ttl;
        }

        public int getTtl() {
            return this.ttl;
        }
    }


    public static class GenericEntry extends Entry {
        private byte[] data;

        public GenericEntry(Name name, ResourceType type, int clazz, int ttl, byte[] data) {
            super(name, type, clazz, ttl);
            this.data = data;
        }

        public byte[] getData() {
            return this.data;
        }
    }


    public static class Address extends Entry {
        private byte[] address;

        public Address(Name name, ResourceType type, int clazz, int ttl, byte[] bytes) {
            super(name, type, clazz, ttl);
            this.address = bytes;
        }

        public byte[] getAddress() {
            return this.address;
        }
    }


    public static class Ptr extends Entry {
        private Name pointedName;

        public Ptr(Name name, ResourceType type, int clazz, int ttl, Name pointedName) {
            super(name, type, clazz, ttl);
            this.pointedName = pointedName;
        }

        public Name getPointedName() {
            return this.pointedName;
        }
    }


    public static class Txt extends Entry {
        private byte[] text;

        public Txt(Name name, ResourceType type, int clazz, int ttl, byte[] bytes) {
            super(name, type, clazz, ttl);
            this.text = bytes;
        }

        public byte[] getText() {
            return this.text;
        }
    }


    public static class Srv extends Entry {
        private final int priority;
        private final int weight;
        private final int port;
        private final Name target;

        public Srv(Name name, ResourceType type, int clazz, int ttl, int priority, int weight,
                   int port, Name target) {
            super(name, type, clazz, ttl);
            this.priority = priority;
            this.weight = weight;
            this.port = port;
            this.target = target;
        }

        public int getPriority() {
            return this.priority;
        }

        public int getWeight() {
            return this.weight;
        }

        public int getPort() {
            return this.port;
        }

        public Name getTarget() {
            return this.target;
        }
    }
}
