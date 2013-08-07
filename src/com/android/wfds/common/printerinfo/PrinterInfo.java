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
package com.android.wfds.common.printerinfo;

import com.android.wfds.common.IDefaultJobParams;
import com.android.wfds.common.IPrinterCapabilities;
import com.android.wfds.jni.wPrintJobParams;
import com.android.wfds.jni.wPrintPrinterCapabilities;

public class PrinterInfo implements IPrinterInfo {

    private final wPrintPrinterCapabilities mCaps;
    private final wPrintJobParams mDefaultJobParams;
    private final int mPortNumber;
    private final String mErrorResult;

    public PrinterInfo(int portNumber, wPrintPrinterCapabilities caps,
                       wPrintJobParams defaultJobParams, String error) {
        mPortNumber = portNumber;
        mCaps = caps;
        mDefaultJobParams = defaultJobParams;
        mErrorResult = error;
    }

    @Override
    public int getPortNum() {
        return mPortNumber;
    }

    @Override
    public IPrinterCapabilities getPrinterCaps() {
        return mCaps;
    }

    @Override
    public IDefaultJobParams getDefaultJobParams() {
        return mDefaultJobParams;
    }

    @Override
    public String getErrorResult() {
        return mErrorResult;
    }
}
