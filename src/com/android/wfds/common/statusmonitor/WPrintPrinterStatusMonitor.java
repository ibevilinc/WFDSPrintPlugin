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
package com.android.wfds.common.statusmonitor;

import java.lang.ref.WeakReference;

import android.os.Bundle;
import android.util.Log;
import android.util.Pair;

import com.android.wfds.common.IStatusParams;
import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.messaging.IMessenger;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.jni.wPrintCallbackParams;
import com.android.wfds.printservice.WPrintService;
import com.android.wfds.printservice.tasks.GetPrinterCapabilitiesTask;

public class WPrintPrinterStatusMonitor extends AbstractPrinterStatusMonitor {

    private static final String TAG = WPrintPrinterStatusMonitor.class
            .getSimpleName();

    private final WeakReference<WPrintService> mContextRef;
    private Bundle mCurrentStatus = null;
    private IwprintJNI mJNI = null;
    private final int mStatusID;
    private final String mPrinterAddress;

    public WPrintPrinterStatusMonitor(WPrintService context, IwprintJNI jni,
                                      String printerAddress, int statusID)
            throws IllegalArgumentException {
        if (context == null) {
            throw new IllegalArgumentException("context can't be null");
        }
        mContextRef = new WeakReference<WPrintService>(context);
        mJNI = jni;
        mPrinterAddress = printerAddress;
        mStatusID = statusID;
        if (mStatusID != 0) {
            Log.d(TAG, "starting printer status monitor");
            mJNI.callNativeMonitorStatusStart(mStatusID, this);
        }
    }

    public void addClient(IMessenger client) {
        if ((client != null) && !mClients.contains(client)) {
            if ((mCurrentStatus != null) && !mCurrentStatus.isEmpty()) {
                notifyClient(client);
            }
            mClients.add(client);
        }
    }

    public boolean removeClient(IMessenger client) {
        boolean shouldQuit;
        mClients.remove(client);
        shouldQuit = mClients.isEmpty();
        if (shouldQuit && (mStatusID != 0)) {
            Log.d(TAG, "stopping printer status monitor");
            mJNI.callNativeMonitorStatusStop(mStatusID);
        }
        return mClients.isEmpty();
    }

    public void updateStatus(IStatusParams status) {
        Bundle statusBundle = null;
        if (status != null) {
            statusBundle = status.getStatusBundle();
            statusBundle.putString(MobilePrintConstants.PRINTER_ADDRESS_KEY,
                    mPrinterAddress);
            statusBundle
                    .putString(
                            MobilePrintConstants.PRINT_REQUEST_ACTION,
                            MobilePrintConstants.ACTION_PRINT_SERVICE_START_MONITORING_PRINTER_STATUS);
        }
        if ((statusBundle != null) && !statusBundle.isEmpty()) {
            if ((mCurrentStatus == null)
                    || !mCurrentStatus.equals(statusBundle)) {
                mCurrentStatus = statusBundle;
                mStatusReply.replaceExtras(mCurrentStatus);
                notifyClients();
            }
        }
    }

    @SuppressWarnings("unused")
    private void callbackReceiver(wPrintCallbackParams status) {
        WPrintService context = mContextRef.get();
        if ((context != null) && (context.getJobHandler() != null)) {
            context.getJobHandler()
                    .sendMessage(
                            context.getJobHandler()
                                    .obtainMessage(
                                            MobilePrintConstants.WPRINT_SERVICE_MSG__PRINTER_STATUS,
                                            Pair.create(this, status)));
        }
    }
}
