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
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.statusmonitor.AbstractPrinterStatusMonitor;
import com.android.wfds.printservice.WPrintService;

public class StopMonitoringPrinterStatusTask extends AbstractMessageTask {

    private WeakReference<WPrintService> mServiceRef;

    public StopMonitoringPrinterStatusTask(AbstractMessage msg,
                                           WPrintService service) {
        super(msg, service.getJobHandler());
        mServiceRef = new WeakReference<WPrintService>(service);
    }

    @Override
    protected Intent doInBackground(Void... params) {
        String address = null;

        // nothing to unregister
        if (mRequest.replyTo == null) {
            return null;
        }

        // get the printer address
        if (mBundleData != null) {
            address = mBundleData
                    .getString(MobilePrintConstants.PRINTER_ADDRESS_KEY);
        }

        // lookup the entry
        if (!TextUtils.isEmpty(address)) {
            synchronized (mServiceRef.get().getJobHandler().mPrinterStatusMonitorSyncObject) {

                AbstractPrinterStatusMonitor monitor = mServiceRef.get().getJobHandler().mPrinterStatusMonitorMap
                        .get(address);
                if (monitor != null) {
                    if (monitor.removeClient(mRequest.replyTo)) {
                        mServiceRef.get().getJobHandler().mPrinterStatusMonitorMap.remove(address);
                    }
                }
            }
        }

        return null;
    }
}
