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
package com.android.wfds.common;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.Socket;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;

import android.content.Context;
import android.net.Uri;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.printservice.WPrintService;

public class PrintServiceUtil
{
    private static final String TAG = PrintServiceUtil.class.getSimpleName();

    public static final int SOCKET_TIMEOUT = 5000;

    private static final int DEBUG_BYTES_PER_LINE = 16;

    public static int isDeviceOnline(String address) {
        Socket socket = null;
        if (TextUtils.isEmpty(address)) {
            return MobilePrintConstants.PORT_ERROR;
        }
        for (int port : WPrintService.PORT_CHECK_ORDER) {
            socket = new Socket();
            try {
                socket.connect(new InetSocketAddress(address, port),
                        SOCKET_TIMEOUT);
                return port;
            } catch (UnknownHostException e) {
            } catch (IOException e) {
            } finally {
                try {
                    if (socket != null)
                        socket.close();
                } catch (IOException e) {
                }
            }
            socket = null;
        }
        return MobilePrintConstants.PORT_ERROR;
    }

    public static String byteArrayToDebugString(byte[] array, int length) {
        int nLines = (length / DEBUG_BYTES_PER_LINE);
        int bytesOnLastLine = (length % DEBUG_BYTES_PER_LINE);
        StringBuilder builder = new StringBuilder();

        for (int i = 0; i < nLines; i++) {
            appendDebugLine(builder, i, DEBUG_BYTES_PER_LINE, DEBUG_BYTES_PER_LINE, array);
        }
        if (bytesOnLastLine > 0) {
            appendDebugLine(builder, nLines, DEBUG_BYTES_PER_LINE, bytesOnLastLine, array);
        }
        return builder.toString();
    }

    private static void appendDebugLine(StringBuilder builder, int lineNumber, int bytesPerLine,
                                        int bytesOnThisLine, byte[] array) {
        builder.append(String.format("%04x", lineNumber * bytesPerLine));
        builder.append(" | ");
        int b, i;
        for (i = 0; i < bytesOnThisLine; i++) {
            b = array[lineNumber * bytesPerLine + i] & 0xff;
            builder.append(String.format("%02x", b));
            builder.append(" ");
        }
        for (i = bytesOnThisLine; i < bytesPerLine; i++) {
            builder.append("   ");
        }
        builder.append("| ");
        for (i = 0; i < bytesOnThisLine; i++) {
            b = array[lineNumber * bytesPerLine + i] & 0xff;
            builder.append(b >= 0x20 && b < 0x80 ? (char) b : '.');
        }
        for (i = bytesOnThisLine; i < bytesPerLine; i++) {
            builder.append(" ");
        }
        builder.append("\n");
    }
}
