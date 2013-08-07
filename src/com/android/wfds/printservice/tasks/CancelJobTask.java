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
import android.text.TextUtils;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.job.IPrintJob;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.printservice.WPrintService;

public class CancelJobTask extends AbstractPrinterInfoTask {

    private WeakReference<WPrintService> mServiceRef;

    public CancelJobTask(AbstractMessage msg, WPrintService service,
                         IwprintJNI jni) {
        super(msg, service.getJobHandler(), jni);
        mServiceRef = new WeakReference<WPrintService>(service);
    }

    @Override
    protected Intent doInBackground(Void... params) {
        int result = -1;
        String jobUUID = findJobUUID(mIntent);
        IPrintJob job = mServiceRef.get().getJobHandler().findJob(null, jobUUID);
        if (job != null) {
            if (!job.getIsJobCanceled()) {
                result = job.cancelJob();
            } else {
                result = 0; // woohoo!! job is already cancelled
            }
        }

        Intent returnIntent = new Intent();
        if (result < 0) {
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR);
            returnIntent.putExtra(
                    MobilePrintConstants.PRINT_ERROR_KEY,
                    MobilePrintConstants.COMMUNICATION_ERROR);
        } else {
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_CANCEL_JOB);
        }
        if (!TextUtils.isEmpty(jobUUID)) {
            returnIntent.putExtra(
                    MobilePrintConstants.PRINT_JOB_HANDLE_KEY,
                    jobUUID);
        }
        if (mIntent != null) {
            returnIntent.putExtra(
                    MobilePrintConstants.PRINT_REQUEST_ACTION,
                    mIntent.getAction());
        }
        return returnIntent;
    }
}
