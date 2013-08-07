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

import android.text.TextUtils;
import android.util.Log;

import java.io.UnsupportedEncodingException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Arrays;

import com.android.wfds.common.MobilePrintConstants;

public class BonjourParser {
    private static final String TAG = BonjourParser.class.getSimpleName();

    public static final String TXTVERS = "txtvers";
    public static final String RP = "rp";
    public static final String NOTE = "note";
    public static final String QTOTAL = "qtotal";
    public static final String PRIORITY = "priority";
    public static final String TY = "ty";
    public static final String PRODUCT = "product";
    public static final String PDL = "pdl";
    public static final String ADMINURL = "adminurl";
    public static final String USB_MFG = "usb_MFG";
    public static final String USB_MDL = "usb_MDL";
    public static final String TRANSPARENT = "Transparent";
    public static final String BINARY = "Binary";
    public static final String TBCP = "TBCP";

    private static final String VALUE_ENCODING = "UTF-8";
    private static final String IPP_SERVICE_NAME = "_ipp._tcp";
    private static final int 	IPV4_LENGTH = 4;

    private DnsService service;

    public BonjourParser(DnsService service) {
        this.service = service;
    }

    public String getBonjourName() {
        return this.service.getName().getLabels()[0];
    }

    public String getHostname() {
        return this.service.getHostname().getLabels()[0];
    }

    public InetAddress getAddress() throws BonjourException {
        byte[] address = this.getFirstIpv4Address();

        try {
            return InetAddress.getByAddress(address);
        } catch (UnknownHostException exc) {
            throw new BonjourException("Printer address is unknown host: "
                    + Arrays.toString(address), exc);
        }
    }

    private byte[] getFirstIpv4Address() {
        byte[][] addresses = this.service.getAddresses();

        for (byte[] address : addresses) {
            if (IPV4_LENGTH == address.length) {
                return address;
            }
        }
        Log.w(TAG, "Could not find any 4 byte address. Will return the first one of the list: "
                + Arrays.toString(addresses[0]));
        return addresses[0];
    }

    public String getModel() throws BonjourException {
        return this.getAttribute(TY);
    }

    public boolean isSupportedPrinter() throws BonjourException {
        return canPrintPCLm() || canPrintPWGRaster();
    }

    public String getPDLs() throws BonjourException {
        return this.getAttribute(PDL);
    }

    public boolean canPrintPCLm() throws BonjourException {
        if (this.isIPPService()) {
            String pdlValue = this.getAttribute(PDL);
            return !TextUtils.isEmpty(pdlValue) && pdlValue.contains(MobilePrintConstants.MIME_TYPE__PCLM);
        }
        return false;
    }

    public boolean canPrintPWGRaster() throws BonjourException {
        if (this.isIPPService()) {
            String pdlValue = this.getAttribute(PDL);
            return !TextUtils.isEmpty(pdlValue) && pdlValue.contains(MobilePrintConstants.MIME_TYPE__PWG_RASTER);
        }
        return false;
    }

    private boolean isIPPService() {
        String serviceName = this.service.getName().toString();

        return serviceName.contains(IPP_SERVICE_NAME);
    }

    public boolean hasAttribute(String key) {
        return this.service.getAttributes().containsKey(key);
    }

    public String getAttribute(String key) throws BonjourException {
        byte[] value = this.service.getAttributes().get(key);

        if (value == null) {
            return null;
        }
        try {
            return new String(value, VALUE_ENCODING);
        } catch (UnsupportedEncodingException exc) {
            throw new BonjourException("Unsupported encoding to read attribute value: "
                    + VALUE_ENCODING, exc);
        }
    }
}
