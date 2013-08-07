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
package com.android.wfds.jni;

import java.util.ArrayList;

import android.os.Bundle;
import android.text.TextUtils;

import com.android.wfds.common.IStatusParams;
import com.android.wfds.common.MobilePrintConstants;

public final class wPrintCallbackParams implements IStatusParams {

    public int callback_id;
    public int job_id;

    public int job_state;

    public int page_num;
    public int copy_num;
    public int corrupted_file;
    public int current_page;
    public int total_pages;
    public int page_total_update;

    public int blocked_reasons;
    public int job_done_result;

    public String printer_state_str = null;
    public String job_state_str = null;;
    public String job_done_result_str = null;
    public String[] blocked_reasons_strings = null;
    public boolean page_start_info;

    public wPrintCallbackParams() {
    }

    public String getJobID() {
        return Integer.toString(job_id);
    }

    public Bundle fillInStatusBundle(Bundle statusBundle) {
        if (!TextUtils.isEmpty(printer_state_str)) {
            statusBundle.putString(MobilePrintConstants.PRINTER_STATUS_KEY, printer_state_str);
            if ((blocked_reasons_strings != null) && (blocked_reasons_strings.length > 0)) {
                statusBundle.putStringArray(MobilePrintConstants.PRINT_JOB_BLOCKED_STATUS_KEY, blocked_reasons_strings);
            }
        } else if (!TextUtils.isEmpty(job_state_str)) {
            statusBundle.putBoolean(MobilePrintConstants.STATUS_BUNDLE_JOB_STATE, statusBundle.isEmpty());
            statusBundle.putString(MobilePrintConstants.PRINT_JOB_STATUS_KEY, job_state_str);
            if (!TextUtils.isEmpty(job_done_result_str)) {
                statusBundle.putString(MobilePrintConstants.PRINT_JOB_DONE_RESULT, job_done_result_str);
            }
            if ((blocked_reasons_strings != null) && (blocked_reasons_strings.length > 0)) {

                // additional check to make sure we don't send empty entries
                ArrayList<String> reasonsList = new ArrayList<String>();
                for(int i = 0; i < blocked_reasons_strings.length; i++) {
                    if (!TextUtils.isEmpty(blocked_reasons_strings[i])) {
                        reasonsList.add(blocked_reasons_strings[i]);
                    }
                }
                // do we have anything?
                if (!reasonsList.isEmpty()) {
                    // send the blocked reasons
                    String[] reasons = new String[reasonsList.size()];
                    reasonsList.toArray(reasons);
                    statusBundle.putStringArray(MobilePrintConstants.PRINT_JOB_BLOCKED_STATUS_KEY, reasons);
                }
            }
        } else {
            statusBundle.putBoolean(MobilePrintConstants.STATUS_BUNDLE_PAGE_INFO, statusBundle.isEmpty());
            statusBundle.putBoolean(MobilePrintConstants.PRINT_STATUS_PAGE_START_INFO, page_start_info);
            statusBundle.putInt(MobilePrintConstants.PRINT_STATUS_COPY_NUMBER, copy_num);
            statusBundle.putInt(MobilePrintConstants.PRINT_STATUS_PAGE_NUMBER, page_num);
            statusBundle.putInt(MobilePrintConstants.PRINT_STATUS_CURRENT_PAGE, current_page);
            statusBundle.putInt(MobilePrintConstants.PRINT_STATUS_TOTAL_PAGE_COUNT, total_pages);
            statusBundle.putBoolean(MobilePrintConstants.PRINT_STATUS_TOTAL_PAGE_UPDATE, (page_total_update != 0));
            statusBundle.putBoolean(MobilePrintConstants.PRINT_STATUS_PAGE_CORRUPTED, (corrupted_file != 0));
        }
        return statusBundle;
    }

    public Bundle getStatusBundle() {
        return fillInStatusBundle(new Bundle());
    }

    public String getJobState() {
        return job_state_str;
    }

    public String getPrinterState() {
        return printer_state_str;
    }

    public boolean isJobDone() {
        return !TextUtils.isEmpty(job_done_result_str);
    }
}
