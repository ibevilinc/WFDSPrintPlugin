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
import android.os.Handler;
import android.text.TextUtils;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.printerinfo.IPrinterInfo;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.jni.wPrintJobParams;
import com.android.wfds.jni.wPrintPrinterCapabilities;

public class GetFinalJobParametersTask extends AbstractPrinterInfoTask {

    private IwprintJNI mJNI;

    public GetFinalJobParametersTask(AbstractMessage msg,
                                     Handler parentHandler, IwprintJNI jni) {
        super(msg, parentHandler, jni);
        mJNI = jni;
    }

    @Override
    protected Intent doInBackground(Void... params) {
        String address = null;
        if (mBundleData != null) {
            address = mBundleData
                    .getString(MobilePrintConstants.PRINTER_ADDRESS_KEY);
        }
        IPrinterInfo entry = getPrinterInfo(address, mBundleData);

        wPrintJobParams jobParams = null;
        if (!TextUtils.isEmpty(address)) {
            if (entry != null) {
                jobParams = new wPrintJobParams(
                        (wPrintJobParams) entry.getDefaultJobParams());
                jobParams.updateJobParams(mBundleData);
                if (mJNI.callNativeGetFinalJobParameters(address,
                        entry.getPortNum(), jobParams,
                        (wPrintPrinterCapabilities) entry.getPrinterCaps()) < 0) {
                    jobParams = null;
                }
            }
        }

        Intent returnIntent = new Intent();
        if (jobParams != null) {
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_FINAL_PARAMS);
            returnIntent.putExtras(jobParams.getJobParams());
        } else {
            String errorStr = null;
            {
                errorStr = MobilePrintConstants.COMMUNICATION_ERROR;
            }
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR);
            returnIntent.putExtra(MobilePrintConstants.PRINT_ERROR_KEY,
                    errorStr);
        }
        if (!TextUtils.isEmpty(address)) {
            returnIntent.putExtra(MobilePrintConstants.PRINTER_ADDRESS_KEY,
                    address);
        }

        if (mIntent != null) {
            returnIntent.putExtra(MobilePrintConstants.PRINT_REQUEST_ACTION,
                    mIntent.getAction());
        }
        return returnIntent;
    }
}
