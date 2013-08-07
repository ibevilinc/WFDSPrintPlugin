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

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.printerinfo.IPrinterInfo;
import com.android.wfds.jni.IwprintJNI;

public class GetPrinterCapabilitiesTask extends AbstractPrinterInfoTask {

    public GetPrinterCapabilitiesTask(AbstractMessage msg, Handler parentHandler, IwprintJNI jni) {
        super(msg, parentHandler, jni);
    }

    @Override
    protected Intent doInBackground(Void... arg0) {
        String address = null;
        if (mBundleData != null) {
            address = mBundleData
                    .getString(MobilePrintConstants.PRINTER_ADDRESS_KEY);
        }
        IPrinterInfo entry = getPrinterInfo(address, mBundleData);
        Intent returnIntent = new Intent();
        Bundle response;
        if (entry != null && entry.getErrorResult() == null) {

            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_PRINTER_CAPABILITIES);
            response = entry.getPrinterCaps()
                    .getPrinterCapabilities();
        } else {

            response = new Bundle();
            String error = null;
            if (entry != null) {
                error = entry.getErrorResult();
            }
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR);
            response.putString(
                    MobilePrintConstants.PRINT_ERROR_KEY,
                    TextUtils.isEmpty(error) ? MobilePrintConstants.COMMUNICATION_ERROR
                            : error);
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
