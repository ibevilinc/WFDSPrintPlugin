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
package com.android.wfds.common.messaging;

import com.android.wfds.common.messaging.IMessenger;

import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;

public class ServiceMessenger implements IMessenger {
    private Messenger mMessenger;

    public ServiceMessenger(Messenger messenger) {
        mMessenger = messenger;
    }

    public void send(Message message) throws RemoteException {
        if (null != mMessenger) {
            mMessenger.send(message);
        }
    }

    public Messenger getMessenger(){
        return mMessenger;
    }

    @Override
    public boolean equals(Object e){
        if(null != e){
            return ((ServiceMessenger)e).getMessenger().equals(mMessenger);
        }
        return false;
    }
}

