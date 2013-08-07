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
package com.android.wfds.printservice.tasks;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.PrintServiceUtil;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.printerinfo.IPrinterInfo;
import com.android.wfds.common.printerinfo.PrinterInfo;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.jni.wPrintJobParams;
import com.android.wfds.jni.wPrintPrinterCapabilities;
import com.android.wfds.printservice.WPrintService;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.util.LruCache;
import android.text.TextUtils;
import android.util.Log;

public abstract class AbstractPrinterInfoTask extends AbstractMessageTask {

    private static final String TAG = GetPrinterCapabilitiesTask.class.getSimpleName();

    private static LruCache<String, IPrinterInfo> mPrinterInfoCache = new LruCache<String, IPrinterInfo>(
            MobilePrintConstants.CAPABILITIES_CACHE_SIZE);

    private IwprintJNI mJNI;

    public AbstractPrinterInfoTask(AbstractMessage msg, Handler parentHandler, IwprintJNI jni) {
        super(msg, parentHandler);

        mJNI = jni;
    }

    protected String findJobUUID(Intent intent) {
        if (intent == null) {
            return null;
        }
        Bundle bundleData = intent.getExtras();
        String jobID = null;
        if (bundleData != null) {
            jobID = bundleData
                    .getString(MobilePrintConstants.PRINT_JOB_HANDLE_KEY);
        }
        return jobID;
    }

    protected IPrinterInfo getPrinterInfo(String address,
                                          Bundle bundleData) {
        IPrinterInfo printerInfo = null;
        int lookupPort = WPrintService.PORT_CHECK_ORDER[0];
        String lookupAddress = address + ":" + lookupPort;
        if (!TextUtils.isEmpty(address)) {
            printerInfo = mPrinterInfoCache.get(lookupAddress);
            if (printerInfo == null) {
                {
                    int port = PrintServiceUtil.isDeviceOnline(address);
                    if (port != MobilePrintConstants.PORT_ERROR) {
                        wPrintPrinterCapabilities printerCaps = new wPrintPrinterCapabilities();
                        wPrintJobParams defaultJobParams = new wPrintJobParams();
                        int result = mJNI.callNativeGetCapabilities(address,
                                port, printerCaps);

                        if (result == 0) {
                            result = mJNI.callNativeGetDefaultJobParameters(
                                    address, port, defaultJobParams,
                                    printerCaps);
                        }
                        if (result == 0) {
                            printerInfo = new PrinterInfo(port,
                                    printerCaps, defaultJobParams, null);
                            {
                                mPrinterInfoCache.put(lookupAddress, printerInfo);
                            }
                        }
                    }
                }
            }
        }
        return printerInfo;
    }
}
