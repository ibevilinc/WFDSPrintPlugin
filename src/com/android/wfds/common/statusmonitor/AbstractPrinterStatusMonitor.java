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

import java.util.ListIterator;
import java.util.Vector;

import android.content.Intent;
import android.os.Message;
import android.os.RemoteException;

import com.android.wfds.common.IStatusParams;
import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.messaging.IMessenger;

public abstract class AbstractPrinterStatusMonitor {
    public static Vector<IMessenger> mClients = new Vector<IMessenger>();
    public Intent mStatusReply = new Intent(
            MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_GET_PRINTER_STATUS);

    public abstract void addClient(IMessenger client);

    public abstract boolean removeClient(IMessenger client);

    public abstract void updateStatus(IStatusParams status);

    public void notifyClient(IMessenger client) {
        if (client != null) {
            try {
                client.send(Message.obtain(null, 0, mStatusReply));
            } catch (RemoteException e) {
            }
        }
    }

    public void notifyClients() {
        ListIterator<IMessenger> iter = mClients.listIterator();
        while (iter.hasNext()) {
            notifyClient(iter.next());
        }
    }
}
