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
package com.android.wfds.printservice;

import java.io.File;
import java.io.FilenameFilter;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.ListIterator;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import android.app.Service;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.support.v4.util.LruCache;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;

import com.android.wfds.common.IStatusParams;
import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.job.IPrintJob;
import com.android.wfds.common.messaging.ServiceMessage;
import com.android.wfds.common.printerinfo.IPrinterInfo;
import com.android.wfds.common.statusmonitor.AbstractPrinterStatusMonitor;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.jni.WprintJNI;
import com.android.wfds.jni.wPrintCallbackParams;
import com.android.wfds.printplugin.BuildConfig;
import com.android.wfds.printplugin.R;
import com.android.wfds.printservice.tasks.AbstractMessageTask;
import com.android.wfds.printservice.tasks.CancelAllTask;
import com.android.wfds.printservice.tasks.CancelJobTask;
import com.android.wfds.printservice.tasks.GetFinalJobParametersTask;
import com.android.wfds.printservice.tasks.GetPrinterCapabilitiesTask;
import com.android.wfds.printservice.tasks.GetPrinterStatusTask;
import com.android.wfds.printservice.tasks.GetSuppliesHandlingIntentTask;
import com.android.wfds.printservice.tasks.LocalPrinterDiscoveryTask;
import com.android.wfds.printservice.tasks.ResumeJobTask;
import com.android.wfds.printservice.tasks.StartJobTask;
import com.android.wfds.printservice.tasks.StartMonitoringPrinterStatusTask;
import com.android.wfds.printservice.tasks.StopMonitoringPrinterStatusTask;

public class WPrintService extends Service {

    public static int[] PORT_CHECK_ORDER = MobilePrintConstants.PORT_CHECK_ORDER_IPP;

    private static final int QUIT_DELAY = 60000;
    private static boolean mLibrariesLoaded = false;
    //private static boolean mSupportLibrariesLoaded = false;
    private ServiceHandler mServiceHandler;
    private JobHandler mJobHandler;
    private Messenger mServiceMessenger;
    private int mStartID = 0;
    private IwprintJNI mJNI;

    @Override
    public void onCreate() {
        mJNI = new WprintJNI();
        mServiceHandler = new ServiceHandler(this);
        mServiceMessenger = new Messenger(mServiceHandler);
        WPrintService.loadLibraries(getApplicationInfo());
        super.onCreate();
        HandlerThread jobThread = new HandlerThread(
                MobilePrintConstants.PRINT_SERVICE_JOB_THREAD_ID);
        jobThread.start();
        mJobHandler = new JobHandler(this, jobThread.getLooper(), mJNI);
        mJobHandler
                .sendEmptyMessage(MobilePrintConstants.WPRINT_SERVICE_MSG__INIT);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mJobHandler
                .sendEmptyMessage(MobilePrintConstants.WPRINT_SERVICE_MSG__EXIT);
        mJobHandler = null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startID) {
        mStartID = startID;
        int result = super.onStartCommand(intent, flags, startID);
        if (intent == null) {
            queueStopRequest();
        } else if (intent.getAction().equals(
                MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINT_SERVICE)) {
        } else {
            mServiceHandler.sendMessage(mServiceHandler
                    .obtainMessage(0, intent));
        }
        return result;
    }

    public IBinder onBind(Intent intent) {
        IBinder binder = null;
        if ((intent == null) || TextUtils.isEmpty(intent.getAction())) {
        } else if (intent.getAction().equals(
                MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINT_SERVICE)) {
            Intent startIntent = new Intent(this, WPrintService.class);
            startIntent.setAction(intent.getAction());
            startService(startIntent);
            mServiceHandler
                    .removeMessages(MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND);
            mServiceHandler
                    .removeMessages(MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_UNBIND);
            mServiceHandler
                    .sendMessageAtFrontOfQueue(mServiceHandler
                            .obtainMessage(
                                    MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND,
                                    intent));
            binder = mServiceMessenger.getBinder();
        }
        return binder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        mServiceHandler
                .removeMessages(MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_UNBIND);
        mServiceHandler
                .removeMessages(MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND);
        mServiceHandler
                .sendMessageAtFrontOfQueue(mServiceHandler
                        .obtainMessage(MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_UNBIND));
        return true;
    }

    @Override
    public void onRebind(Intent intent) {
        if ((intent == null) || TextUtils.isEmpty(intent.getAction())) {
        } else if (intent.getAction().equals(
                MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINT_SERVICE)) {
            Intent startIntent = new Intent(this, WPrintService.class);
            startIntent.setAction(intent.getAction());
            mServiceHandler
                    .removeMessages(MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND);
            mServiceHandler
                    .removeMessages(MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_UNBIND);
            mServiceHandler
                    .sendMessageAtFrontOfQueue(mServiceHandler
                            .obtainMessage(
                                    MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND,
                                    intent));
        }
    }

    /**
     * ServiceHandler - entry point for all incoming messages to the
     * WPrintService. All print job related messages will be forwarded to the
     * JobHandler.
     */
    private static class ServiceHandler extends Handler {

        private final WeakReference<WPrintService> mServiceRef;

        public ServiceHandler(WPrintService service) {
            super();
            mServiceRef = new WeakReference<WPrintService>(service);
        }

        @Override
        public void handleMessage(Message msg) {
            Intent intent = null;
            if ((msg.obj != null) && (msg.obj instanceof Intent)) {
                intent = (Intent) msg.obj;
            }
            WPrintService service = mServiceRef.get();
            if (service == null) {
                return;
            } else if (msg.obj == null) {
                service.queueStopRequest();
            } else {
                boolean priorityMessage = false;
                int msgWhat = 0;
                if (msg.what == MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND;
                    priorityMessage = true;
                } else if (intent == null) {
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINTER_CAPABILTIES)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__GET_CAPABILITIES;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_GET_FINAL_PRINT_SETTINGS)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__GET_FINAL_JOB_PARAMETERS;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_RESUME_PRINT_JOB)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__RESUME_JOB;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_CANCEL_PRINT_JOB)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__CANCEL_JOB;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_CANCEL_ALL_PRINT_JOBS)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__CANCEL_ALL;
                } else if (intent.getAction().equals(
                        MobilePrintConstants.ACTION_PRINT_SERVICE_PRINT)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__START_JOB;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_REGISTER_STATUS_RECEIVER)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__REGISTER_STATUS_RECEIVER;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_UNREGISTER_STATUS_RECEIVER)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__UNREGISTER_STATUS_RECEIVER;
                    priorityMessage = true;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINTER_STATUS)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__GET_PRINTER_STATUS;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_START_MONITORING_PRINTER_STATUS)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__START_MONITORING_PRINTER_STATUS;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_STOP_MONITORING_PRINTER_STATUS)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__STOP_MONITORING_PRINTER_STATUS;
                    priorityMessage = true;
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_START_DISCOVERY)) {
                    Bundle extras = intent.getExtras();
                    String[] services = null;
                    if (null != extras) {
                        services = extras
                                .getStringArray(MobilePrintConstants.DISCOVERY_SERVICES);
                    }
                    if (null == services) {
                        executeTask(new LocalPrinterDiscoveryTask(
                                mServiceRef.get(), new ServiceMessage(msg), service.getJobHandler()));
                    } else {
                        for (String serviceName : services) {
                            if (MobilePrintConstants.DISCOVERY_SERVICE_LOCAL
                                    .equals(serviceName)) {
                                executeTask(new LocalPrinterDiscoveryTask(
                                        mServiceRef.get(), new ServiceMessage(
                                        msg), service.getJobHandler()));
                            } else {
                                Intent returnIntent = new Intent(
                                        MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR);
                                returnIntent
                                        .putExtra(
                                                MobilePrintConstants.DISCOVERY_ERROR_KEY,
                                                "Unsupported discovery service name: "
                                                        + serviceName);
                                try {
                                    msg.replyTo.send(Message.obtain(null, 0,
                                            returnIntent));
                                } catch (RemoteException e) {
                                    e.printStackTrace();
                                }
                            }
                        }
                    }
                } else if (intent
                        .getAction()
                        .equals(MobilePrintConstants.ACTION_PRINT_SERVICE_GET_SUPPLIES_HANDLING_INTENT)) {
                    msgWhat = MobilePrintConstants.WPRINT_SERVICE_MSG__GET_SUPPLIES_HANDLING_INTENT;
                }
                if (msgWhat != 0) {
                    Message jobMessage = Message.obtain(msg);
                    jobMessage.what = msgWhat;
                    if (priorityMessage) {
                        mServiceRef.get().mJobHandler
                                .sendMessageAtFrontOfQueue(jobMessage);
                    } else {
                        mServiceRef.get().mJobHandler.sendMessage(jobMessage);
                    }
                }
            }
        }
    }

    public static class JobHandler extends Handler {

        private final WeakReference<WPrintService> mServiceRef;
        private AtomicInteger mNumTasks = new AtomicInteger(0);

        private LinkedList<IPrintJob> mJobList = new LinkedList<IPrintJob>();
        private LinkedList<Messenger> mStatusCallbacks = new LinkedList<Messenger>();
        private ArrayList<IStatusParams> mPendingStatus = new ArrayList<IStatusParams>();
        public ConcurrentHashMap<String, AbstractPrinterStatusMonitor> mPrinterStatusMonitorMap = new ConcurrentHashMap<String, AbstractPrinterStatusMonitor>();
        public Object mPrinterStatusMonitorSyncObject = new Object();
        private boolean mInitialized = false;
        private boolean mIsServiceBound = false;

        private LruCache<String, IPrinterInfo> mPrinterInfoCache;
        private IwprintJNI mJNI;

        public JobHandler(WPrintService service, Looper looper, IwprintJNI jni) {
            super(looper);
            mPrinterInfoCache = new LruCache<String, IPrinterInfo>(
                    MobilePrintConstants.CAPABILITIES_CACHE_SIZE);
            mServiceRef = new WeakReference<WPrintService>(service);
            mJNI = jni;
        }

        @SuppressWarnings("unchecked")
        @Override
        public void handleMessage(Message msg) {

            WPrintService service = mServiceRef.get();
            Intent intent = null;
            Bundle bundleData = null;
            if ((msg.obj != null) && (msg.obj instanceof Intent)) {
                intent = (Intent) msg.obj;
                bundleData = ((intent != null) ? intent.getExtras() : null);
            }
            service.removeStopRequest();

            if (!mInitialized) {
                // make sure initialization is the first thing that happens
                initialize();
            }

            switch (msg.what) {
                case MobilePrintConstants.WPRINT_SERVICE_MSG__INIT: {
                    // should already be initialized by the time we get here so just
                    // break
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__GET_CAPABILITIES: {
                    executeTask(new GetPrinterCapabilitiesTask(new ServiceMessage(msg), service.getJobHandler(), mJNI));
                    break;

                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__GET_FINAL_JOB_PARAMETERS: {
                    executeTask(new GetFinalJobParametersTask(new ServiceMessage(msg), service.getJobHandler(), mJNI));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__START_JOB: {
                    executeTask(new StartJobTask(new ServiceMessage(msg), service, mJNI));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__JOB_STATUS: {
                    IStatusParams params = null;
                    if (msg.obj instanceof IStatusParams) {
                        params = (IStatusParams) msg.obj;
                    }

                    processPendingStatus();
                    if (params != null) {
                        if (!processStatus(params)) {
                            mPendingStatus.add(params);
                        }
                    }
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__CANCEL_JOB: {
                    executeTask(new CancelJobTask(new ServiceMessage(msg), service, mJNI));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__RESUME_JOB: {
                    executeTask(new ResumeJobTask(new ServiceMessage(msg), service, mJNI));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__CANCEL_ALL: {
                    executeTask(new CancelAllTask(new ServiceMessage(msg), service));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__REGISTER_STATUS_RECEIVER: {
                    synchronized (mStatusCallbacks) {
                        if ((msg.replyTo != null)
                                && !mStatusCallbacks.contains(msg.replyTo)) {
                            mStatusCallbacks.add(msg.replyTo);
                        }
                    }
                    break;

                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__UNREGISTER_STATUS_RECEIVER: {
                    synchronized (mStatusCallbacks) {
                        if (msg.replyTo != null) {
                            mStatusCallbacks.remove(msg.replyTo);
                        }
                    }
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__EXIT: {
                    mPrinterInfoCache.evictAll();
                    mJNI.callNativeExit();
                    getLooper().quit();
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND:
                case MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_UNBIND: {
                    mIsServiceBound = (msg.what == MobilePrintConstants.WPRINT_SERVICE_MSG__SERVICE_BIND);
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__GET_PRINTER_STATUS: {
                    executeTask(new GetPrinterStatusTask(new ServiceMessage(msg), service, mJNI));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__START_MONITORING_PRINTER_STATUS: {
                    executeTask(new StartMonitoringPrinterStatusTask(new ServiceMessage(msg), service, mJNI));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__STOP_MONITORING_PRINTER_STATUS: {
                    executeTask(new StopMonitoringPrinterStatusTask(new ServiceMessage(msg), service));
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__GET_SUPPLIES_HANDLING_INTENT: {
                    executeTask(new GetSuppliesHandlingIntentTask(new ServiceMessage(msg), service.getJobHandler(), mJNI));
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__PRINTER_STATUS: {
                    Pair<AbstractPrinterStatusMonitor, wPrintCallbackParams> statusPair = null;
                    if (msg.obj instanceof Pair<?, ?>) {
                        statusPair = (Pair<AbstractPrinterStatusMonitor, wPrintCallbackParams>) msg.obj;
                    }
                    if ((statusPair != null) && (statusPair.first != null)) {
                        statusPair.first.updateStatus(statusPair.second);
                    }
                    break;
                }

                case MobilePrintConstants.WPRINT_SERVICE_MSG__TASK_COMPLETE: {
                    processPendingStatus();
                    break;
                }
                default: {
                    break;
                }
            }

            if ((msg.what != MobilePrintConstants.WPRINT_SERVICE_MSG__EXIT)
                    &&(!mIsServiceBound)&& (mNumTasks.get() == 0)){
                synchronized (mJobList) {
                    if(mJobList.isEmpty()){
                        if( mPrinterStatusMonitorMap.isEmpty()) {
                            service.queueStopRequest();
                        }
                    }
                }
            }
        }

        public void addJobToList(IPrintJob printJob){
            synchronized (mJobList) {
                mJobList.add(printJob);
            }
        }

        public IPrintJob findJob(String jobHandle, String uuid) {
            if (TextUtils.isEmpty(jobHandle) && TextUtils.isEmpty(uuid)) {
                return null;
            }
            synchronized (mJobList) {
                ListIterator<IPrintJob> iter = mJobList.listIterator();
                while (iter.hasNext()) {
                    IPrintJob entry = iter.next();
                    if (((!TextUtils.isEmpty(jobHandle)) && (entry.getJobID()
                            .equals(jobHandle)))
                            || (!TextUtils.isEmpty(uuid) && entry.getJobUUID()
                            .equals(uuid))) {
                        return entry;
                    }
                }
            }
            return null;
        }

        public int cancelAll() {
            int result = 0;
            synchronized (mJobList) {
                ListIterator<IPrintJob> iter = mJobList.listIterator(mJobList
                        .size());
                // cancel in reverse order
                while (iter.hasPrevious()) {
                    IPrintJob entry = iter.previous();
                    if (!entry.getIsJobCanceled()) {
                        if (entry.cancelJob() < 0) {
                            result = -1;
                        }
                    }
                }

            }
            return result;
        }

        private void initialize(){
            WPrintService service = mServiceRef.get();
            mInitialized = true;
            ArrayList<String> pluginList = new ArrayList<String>();
            ApplicationInfo appInfo = service.getApplicationInfo();

            String nativeLibDir = null;
            int currentapiVersion = android.os.Build.VERSION.SDK_INT;
            if (currentapiVersion <= android.os.Build.VERSION_CODES.FROYO) {
                String dataDir = service.getApplicationInfo().dataDir;
                nativeLibDir = dataDir + "/lib";
            } else {
                nativeLibDir = appInfo.nativeLibraryDir;
            }
            getPlugins(nativeLibDir, pluginList);
            String envVal = System.getenv("LD_LIBRARY_PATH");
            if (!TextUtils.isEmpty(envVal)) {
                String[] ldLibs = envVal.split(":");
                if ((ldLibs != null) && (ldLibs.length > 0)) {
                    for (int libIndex = 0; libIndex < ldLibs.length; libIndex++) {
                        getPlugins(ldLibs[libIndex], pluginList);
                    }
                }
            }

            filterAPIPlugins(pluginList);

            String[] plugins = null;
            if (pluginList.size() > 0) {
                plugins = new String[pluginList.size()];
                pluginList.toArray(plugins);
            }

            mJNI.callNativeSetLogLevel(BuildConfig.DEBUG ? Log.VERBOSE
                    : Log.ASSERT);
            mJNI.callNativeInit(this, service.getApplicationInfo().dataDir,
                    plugins);
            final int libIdentifierDefaultID = service.getResources()
                    .getIdentifier(
                            service.getResources().getResourceName(
                                    R.string.wprint_application_id),
                            "string", service.getPackageName());
            String defaultID = service.getString(libIdentifierDefaultID);
            mJNI.callNativeSetApplicationID(defaultID);
        }

        @SuppressWarnings("unused")
        private void jobCallback(int jobID, wPrintCallbackParams params) {
            sendMessage(obtainMessage(
                    MobilePrintConstants.WPRINT_SERVICE_MSG__JOB_STATUS, jobID,
                    0, params));
        }

        public void sendJobStatusMessage(IStatusParams params) {
            sendMessage(obtainMessage(
                    MobilePrintConstants.WPRINT_SERVICE_MSG__JOB_STATUS, 0, 0,
                    params));
        }

        public void sendPrinterStatusMessage(String address,
                                             IStatusParams params) {
            sendMessage(obtainMessage(
                    MobilePrintConstants.WPRINT_SERVICE_MSG__PRINTER_STATUS, 0,
                    0,
                    Pair.create(mPrinterStatusMonitorMap.get(address), params)));
        }

        private void getPlugins(String dir, ArrayList<String> pluginList) {
            FilenameFilter pluginFilter = new FilenameFilter() {
                @Override
                public boolean accept(File dir, String filename) {
                    return (filename.startsWith("libwprintplugin_") && filename
                            .endsWith(".so"));
                }
            };
            File libsDir = new File(dir);
            File[] libsList = libsDir.listFiles(pluginFilter);
            if ((libsList != null) && (libsList.length > 0)) {
                for (int i = 0; i < libsList.length; i++) {
                    pluginList.add(libsList[i].getAbsolutePath());
                }
            }
        }

        private static class APIPairComparator implements
                Comparator<Pair<String, Integer>> {

            @Override
            public int compare(Pair<String, Integer> lhs,
                               Pair<String, Integer> rhs) {
                String lhsSub = lhs.first.substring(0,
                        lhs.first.lastIndexOf('_'));
                String rhsSub = rhs.first.substring(0,
                        rhs.first.lastIndexOf('_'));

                int result = lhsSub.compareTo(rhsSub);
                if (result == 0) {
                    // same plugin, sort by API, version 0 is considered greater
                    // than a non-zero value
                    if (lhs.second.intValue() == rhs.second.intValue())
                        return 0;
                    else if (lhs.second.intValue() == 0)
                        return 1;
                    else if (rhs.second.intValue() == 0)
                        return -1;
                    else
                        return lhs.second.compareTo(rhs.second);
                } else {
                    // different plugin, sort by name
                    return result;
                }
            }

        }

        private void filterAPIPlugins(ArrayList<String> pluginList) {
            ArrayList<Pair<String, Integer>> apiPlugins = new ArrayList<Pair<String, Integer>>();

            // find all the versioned api plugins
            ListIterator<String> pluginIter = pluginList.listIterator();
            while (pluginIter.hasNext()) {
                String plugin = pluginIter.next();
                if (plugin.endsWith("api.so")) {
                    int start = plugin.lastIndexOf('r') + 1;
                    int end = plugin.lastIndexOf('a');
                    Integer api = Integer.valueOf(plugin.substring(start, end));
                    apiPlugins.add(Pair.create(plugin, api));
                    pluginIter.remove();
                }
            }

            // check if we're done
            if (apiPlugins.isEmpty()) {
                return;
            }

            // sort the api versioned plugins to make it easier to pick the
            // right one
            Collections.sort(apiPlugins, new APIPairComparator());

            // determine the best versioned plugin to use
            ListIterator<Pair<String, Integer>> apiIter = apiPlugins
                    .listIterator();
            Pair<String, Integer> candidate = null;
            ArrayList<String> candidates = new ArrayList<String>();
            while (apiIter.hasNext()) {
                Pair<String, Integer> entry = apiIter.next();
                if (candidate != null) {
                    String prevSub = candidate.first.substring(0,
                            candidate.first.lastIndexOf('_'));
                    String curSub = entry.first.substring(0,
                            entry.first.lastIndexOf('_'));
                    if (prevSub.equals(curSub)) {
                        if (entry.second.intValue() <= android.os.Build.VERSION.SDK_INT) {
                            candidate = entry;
                        }
                    } else {
                        // different plugin

                        // add the candidate if we had one
                        if (candidate != null)
                            candidates.add(candidate.first);
                        candidate = null;
                    }
                }
                if ((candidate == null)
                        && (entry.second.intValue() <= android.os.Build.VERSION.SDK_INT)) {
                    candidate = entry;
                }
            }
            if (candidate != null)
                candidates.add(candidate.first);

            // Make sure we don't have api and non-api versions of a plugin
            ListIterator<String> candidatesIter = candidates.listIterator();
            while (candidatesIter.hasNext()) {
                String entry = candidatesIter.next();
                String plugin = entry.substring(entry.lastIndexOf('/') + 1,
                        entry.lastIndexOf('_'));

                pluginIter = pluginList.listIterator();
                while (pluginIter.hasNext()) {
                    String check = pluginIter.next();
                    if (plugin
                            .equals(check.substring(check.lastIndexOf('/') + 1,
                                    check.lastIndexOf('.')))) {
                        pluginIter.remove();
                    }
                }
                pluginList.add(entry);
            }
        }

        private boolean processStatus(IStatusParams params) {
            boolean jobFound = false;
            String jobID = null;

            if (params != null) {
                jobID = params.getJobID();
            }
            IPrintJob job = findJob(jobID, null);
            if (job != null) {
                jobFound = true;
                Bundle statusBundle = null;
                if (params != null) {
                    statusBundle = params.getStatusBundle();
                    if (!TextUtils.isEmpty(params.getJobState())) {
                        job.setJobStatus(params);
                        if (params.isJobDone()) {
                            job.endJob();
                            synchronized (mJobList) {
                                mJobList.remove(job);
                            }
                        }
                        // job status already present, fill in page status if avaialable
                        if (job.getPageStatus() != null) {
                            job.getPageStatus().fillInStatusBundle(statusBundle);
                        }
                    } else {
                        job.setPageStatus(params);
                        // page status already present, fill in job status if avaialable
                        if (job.getJobStatus() != null) {
                            job.getJobStatus().fillInStatusBundle(statusBundle);
                        }
                    }
                }

                Intent statusIntent = new Intent(
                        MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_PRINT_JOB_STATUS);
                if (statusBundle != null) {
                    statusBundle.putString(
                            MobilePrintConstants.PRINT_JOB_HANDLE_KEY,
                            job.getJobUUID());
                    statusIntent.putExtras(statusBundle);
                }

                synchronized (mStatusCallbacks) {
                    if (!mStatusCallbacks.isEmpty()) {
                        ListIterator<Messenger> iter = mStatusCallbacks
                                .listIterator();
                        while (iter.hasNext()) {
                            try {
                                iter.next().send(
                                        Message.obtain(null, 0, statusIntent));
                            } catch (RemoteException e) {

                            }
                        }
                    }
                }

            }
            return jobFound;
        }

        private void processPendingStatus() {
            ArrayList<IStatusParams> statusProcessed = new ArrayList<IStatusParams>();
            for (IStatusParams status : mPendingStatus) {
                if (processStatus(status))
                    statusProcessed.add(status);
            }
            for (IStatusParams status : statusProcessed) {
                mPendingStatus.remove(status);
            }
        }
    }

    private static synchronized void loadLibraries(ApplicationInfo appInfo) {
        if (!mLibrariesLoaded) {
            if (((appInfo.flags & ApplicationInfo.FLAG_SYSTEM) == 0)
                    || ((appInfo.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0)) {
                System.loadLibrary("wfdscrypto");
                System.loadLibrary("wfdsssl");
                System.loadLibrary("wfdsjpeg");
                System.loadLibrary("wfdspng");
                System.loadLibrary("wfdscups");
                System.loadLibrary("wfds");
            } else {
                System.load(Environment.getRootDirectory()
                        + "/lib/libwfdscrypto.so");
                System.load(Environment.getRootDirectory()
                        + "/lib/libwfdsssl.so");
                System.load(Environment.getRootDirectory()
                        + "/lib/libwfdsjpeg.so");
                System.load(Environment.getRootDirectory()
                        + "/lib/libwfdspng.so");
                System.load(Environment.getRootDirectory()
                        + "/lib/libwfdscups.so");
                System.load(Environment.getRootDirectory()
                        + "/lib/libwfds.so");
            }
            mLibrariesLoaded = true;
        }
    }

    private Runnable mQuitRunnable = new Runnable() {
        @Override
        public void run() {
            stopSelf(mStartID);
        }
    };

    private synchronized void removeStopRequest() {
        mServiceHandler.removeCallbacks(mQuitRunnable);
    }

    private synchronized void queueStopRequest() {
        removeStopRequest();
        mServiceHandler.postDelayed(mQuitRunnable, QUIT_DELAY);
    }

    public void sendJobStatusMessage(IStatusParams params) {
        mJobHandler.sendJobStatusMessage(params);
    }

    public void sendPrinterStatusMessage(String address, IStatusParams params) {
        mJobHandler.sendPrinterStatusMessage(address, params);
    }

    public JobHandler getJobHandler() {
        return mJobHandler;
    }

    private static <T extends AbstractMessageTask> void executeTask(T task) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        } else {
            task.execute();
        }
    }
}
