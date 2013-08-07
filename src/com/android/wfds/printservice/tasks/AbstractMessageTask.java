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
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.messaging.ServiceMessage;

public abstract class AbstractMessageTask extends AsyncTask<Void, Void, Intent>
{

    protected final AbstractMessage mRequest;
    protected final Intent mIntent;
    protected final Bundle mBundleData;
    protected final Handler mParentHandler;

    public AbstractMessageTask(AbstractMessage msg, Handler parentHandler)
    {
        super();
        mRequest = new ServiceMessage(msg.obtain());

        Intent intent = null;
        Bundle bundleData = null;
        if ((mRequest.getMessage().obj != null) && (mRequest.getMessage().obj instanceof Intent))
        {
            intent = (Intent) mRequest.getMessage().obj;
            bundleData = ((intent != null) ? intent.getExtras() : null);
        }
        mIntent = intent;
        mBundleData = bundleData;
        mParentHandler = parentHandler;
    }

    protected void onPostExecute(Intent result) {
        if (result != null) {
            if (mRequest.replyTo != null) {
                try {
                    mRequest.replyTo.send(Message.obtain(null, 0, result));
                } catch (RemoteException e) {
                }
            }
        }
        if (null != mParentHandler) {
            mParentHandler
                    .sendEmptyMessage(MobilePrintConstants.WPRINT_SERVICE_MSG__TASK_COMPLETE);
        }
    }
}
