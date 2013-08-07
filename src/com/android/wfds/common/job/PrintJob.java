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
package com.android.wfds.common.job;

import java.util.UUID;

import android.os.Bundle;
import android.text.TextUtils;

import com.android.wfds.common.IStatusParams;
import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.jni.IwprintJNI;

public class PrintJob implements IPrintJob {

    private final Integer mJobID;
    private final Bundle mSettings;
    private IStatusParams mJobStatus = null;
    private IStatusParams mPageStatus = null;
    private final String mJobUUID;
    private boolean mCanceled = false;
    private IwprintJNI mJNI = null;

    public PrintJob(int jobID, Bundle settings, IwprintJNI jni) {
        mJNI = jni;
        mJobID = Integer.valueOf(jobID);
        mSettings = settings;
        String uuid = mSettings
                .getString(MobilePrintConstants.PRINT_JOB_HANDLE_KEY);
        if (TextUtils.isEmpty(uuid)) {
            uuid = UUID.randomUUID().toString();
        }
        mJobUUID = uuid;
    }

    public String getJobID() {
        return mJobID.toString();
    }

    public Bundle getSettings() {
        return mSettings;
    }

    public String getJobUUID() {
        return mJobUUID;
    }

    public IStatusParams getJobStatus() {
        return mJobStatus;
    }

    public void setJobStatus(IStatusParams params) {
        mJobStatus = params;
    }

    public IStatusParams getPageStatus() {
        return mPageStatus;
    }

    public void setPageStatus(IStatusParams params) {
        mPageStatus = params;
    }

    public boolean getIsJobCanceled() {
        return mCanceled;
    }

    public int cancelJob() {
        int canceled = mJNI.callNativeCancelJob(
                mJobID.intValue());
        mCanceled = true;
        return canceled;
    }

    public int resumeJob() {
        return mJNI.callNativeResumeJob(mJobID.intValue());
    }

    public void endJob() {
        mJNI.callNativeEndJob(mJobID.intValue());
    }
}
