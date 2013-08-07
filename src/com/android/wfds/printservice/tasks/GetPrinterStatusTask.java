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

import java.lang.ref.WeakReference;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;

import com.android.wfds.common.IStatusParams;
import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.printerinfo.IPrinterInfo;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.printservice.WPrintService;

public class GetPrinterStatusTask extends AbstractPrinterInfoTask {

    private WeakReference<WPrintService> mServiceRef;
    private IwprintJNI mJNI;

    public GetPrinterStatusTask(AbstractMessage msg, WPrintService service,
                                IwprintJNI jni) {
        super(msg, service.getJobHandler(), jni);
        mServiceRef = new WeakReference<WPrintService>(service);
        mJNI = jni;
    }

    @Override
    protected Intent doInBackground(Void... params) {
        String address = null;
        IStatusParams status = null;
        String errorResult = null;

        if (mBundleData != null) {
            address = mBundleData
                    .getString(MobilePrintConstants.PRINTER_ADDRESS_KEY);
        }
        {
            IPrinterInfo entry = getPrinterInfo(address, mBundleData);
            if (entry != null) {
                status = mJNI.callNativeGetPrinterStatus(
                        address, entry.getPortNum());
            }
        }
        Intent returnIntent = new Intent();
        Bundle response;
        if (status != null) {
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_GET_PRINTER_STATUS);
            response = status.getStatusBundle();
        } else {
            response = new Bundle();
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR);
            response.putString(
                    MobilePrintConstants.PRINT_ERROR_KEY,
                    (errorResult == null) ? MobilePrintConstants.COMMUNICATION_ERROR
                            : errorResult);
        }
        if (!TextUtils.isEmpty(address)) {
            response.putString(
                    MobilePrintConstants.PRINTER_ADDRESS_KEY,
                    address);
        }
        returnIntent.putExtras(response);
        if (mIntent != null) {
            returnIntent.putExtra(
                    MobilePrintConstants.PRINT_REQUEST_ACTION,
                    mIntent.getAction());
        }
        return returnIntent;
    }
}
