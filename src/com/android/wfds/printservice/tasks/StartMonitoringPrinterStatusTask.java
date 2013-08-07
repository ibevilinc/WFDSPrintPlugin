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
import com.android.wfds.common.printerinfo.IPrinterInfo;
import com.android.wfds.common.statusmonitor.AbstractPrinterStatusMonitor;
import com.android.wfds.common.statusmonitor.WPrintPrinterStatusMonitor;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.printservice.WPrintService;

public class StartMonitoringPrinterStatusTask extends AbstractPrinterInfoTask {

    private WeakReference<WPrintService> mServiceRef;

    private IwprintJNI mJNI;

    public StartMonitoringPrinterStatusTask(AbstractMessage msg, WPrintService service,
                                            IwprintJNI jni) {
        super(msg, service.getJobHandler(), jni);
        mServiceRef = new WeakReference<WPrintService>(service);
        mJNI = jni;
    }

    @Override
    protected Intent doInBackground(Void... params) {
        String address = null;
        boolean printToFile = false;
        boolean sendError = false;
        String statusError = null;

        // nothing to register
        if (mRequest.replyTo == null) {
            return null;
        }

        if (mBundleData != null) {
            address = mBundleData
                    .getString(MobilePrintConstants.PRINTER_ADDRESS_KEY);
        }

        if (!TextUtils.isEmpty(address)) {
            synchronized (mServiceRef.get().getJobHandler().mPrinterStatusMonitorSyncObject) {


                // lookup a pre-existing entry
                AbstractPrinterStatusMonitor monitor = mServiceRef.get().getJobHandler().mPrinterStatusMonitorMap
                        .get(address);
                if (monitor != null) {
                    monitor.addClient(mRequest.replyTo);
                }
                else {
                    IPrinterInfo entry = getPrinterInfo(
                            address, mBundleData);
                    {
                        if (entry != null) {
                            int statusID = mJNI.callNativeMonitorStatusSetup(
                                    address,
                                    entry.getPortNum());
                            monitor = new WPrintPrinterStatusMonitor(
                                    mServiceRef.get(), mJNI, address, statusID);
                            if (statusID == 0) {
                                // no monitor, get some status and
                                // send it back
                                monitor.updateStatus(mJNI.callNativeGetPrinterStatus(
                                        address,
                                        entry.getPortNum()));
                            }
                            monitor.addClient(mRequest.replyTo);
                            if (statusID != 0) {
                                // store monitor instance
                                mServiceRef.get().getJobHandler().mPrinterStatusMonitorMap.put(
                                        address, monitor);
                            } else {
                                // cleanup the monitor
                                monitor.removeClient(mRequest.replyTo);
                                monitor = null;
                            }
                        } else {
                            sendError = true;
                        }
                    }
                }
                //waiting for a short period before releasing synchronized to allow lower layer threads to get started
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        } else {
            sendError = true;
        }
        Intent returnIntent = null;
        if (sendError) {
            returnIntent = new Intent();
            returnIntent
                    .setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR);
            returnIntent
                    .putExtra(
                            MobilePrintConstants.PRINT_ERROR_KEY,
                            (statusError == null) ? MobilePrintConstants.COMMUNICATION_ERROR
                                    : statusError);
            if (mIntent != null) {
                returnIntent
                        .putExtra(
                                MobilePrintConstants.PRINT_REQUEST_ACTION,
                                mIntent.getAction());
            }
        }
        return returnIntent;
    }
}
