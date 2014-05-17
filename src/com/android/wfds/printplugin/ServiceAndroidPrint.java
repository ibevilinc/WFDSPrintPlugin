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
package com.android.wfds.printplugin;

import android.annotation.TargetApi;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentInfo;
import android.print.PrintJobId;
import android.print.PrintJobInfo;
import android.print.PrinterCapabilitiesInfo;
import android.print.PrinterId;
import android.print.PrinterInfo;
import android.printservice.PrintDocument;
import android.printservice.PrintJob;
import android.printservice.PrinterDiscoverySession;
import android.text.TextUtils;
import android.util.Log;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.AbstractAsyncTask;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicBoolean;

@TargetApi(Build.VERSION_CODES.KITKAT)
public class ServiceAndroidPrint extends android.printservice.PrintService {

    private static final String MIME_TYPE_PDF = "application/pdf";
    private static final String PDF_EXTENSION = ".pdf";

    private static class PrintServiceCallbackHandler extends Handler {
        private final WeakReference<ServiceAndroidPrint> mContextRef;

        public PrintServiceCallbackHandler(ServiceAndroidPrint context) {
            mContextRef = new WeakReference<ServiceAndroidPrint>(context);
        }

        @Override
        public void handleMessage(Message msg) {

            ServiceAndroidPrint service = mContextRef.get();

            // check message object
            if ((msg.obj == null) || !(msg.obj instanceof Intent)) {
                return;
            }

            // grab the intent
            Intent intent = (Intent) msg.obj;
            // get the return action
            String action = intent.getAction();

            // pull out the bundle
            Bundle b = intent.getExtras();
            if (b == null) b = new Bundle();

            // extract the request action
            String requestAction = b
                    .getString(MobilePrintConstants.PRINT_REQUEST_ACTION);

            String jobID = b.getString(MobilePrintConstants.PRINT_JOB_HANDLE_KEY);
            PrintInfoHolder jobInfo = (!TextUtils.isEmpty(jobID) ? service.mPrintJobs.get(jobID) : null);

            if (jobInfo == null) {
                return;
            }

            if (action.equals(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_PRINT_JOB_STATUS)) {

                boolean sendLowSuppliesRequest = false;


                String jobStatus = b.getString(MobilePrintConstants.PRINT_JOB_STATUS_KEY);
                String doneResult = b.getString(MobilePrintConstants.PRINT_JOB_DONE_RESULT);

                if (!TextUtils.isEmpty(jobStatus)) {

                    String[] blockedReasons = b.getStringArray(MobilePrintConstants.PRINT_JOB_BLOCKED_STATUS_KEY);

                    if (!TextUtils.isEmpty(doneResult)) {
                        boolean low_toner, low_ink;
                        boolean no_toner, no_ink;
                        low_toner = low_ink = no_toner = no_ink = false;
                        if (blockedReasons != null) {
                            for(String error : blockedReasons) {

                                if (TextUtils.isEmpty(error)) {

                                } else if (error.equals(MobilePrintConstants.BLOCKED_REASON__LOW_ON_INK)) {
                                    low_ink = true;
                                } else if (error.equals(MobilePrintConstants.BLOCKED_REASON__LOW_ON_TONER)) {
                                    low_toner = true;
                                } else if (error.equals(MobilePrintConstants.BLOCKED_REASON__OUT_OF_INK)) {
                                    no_ink = true;
                                } else if (error.equals(MobilePrintConstants.BLOCKED_REASON__OUT_OF_TONER)) {
                                    no_toner = true;
                                } else if (error.equals(MobilePrintConstants.BLOCKED_REASON__REALLY_LOW_ON_INK)) {
                                    low_ink = true;
                                }
                            }
                        }
                        jobInfo.mLowEmptySupplies = (low_ink || low_toner || no_ink || no_toner);
                    }

                    if (MobilePrintConstants.JOB_STATE_RUNNING.equals(jobStatus)) {

                        if (!jobInfo.mPrintJob.isStarted()) {
                            jobInfo.mPrintJob.start();
                        }
                    } else if (MobilePrintConstants.JOB_STATE_BLOCKED.equals(jobStatus)) {

                        sendLowSuppliesRequest = jobInfo.mLowEmptySupplies;

                        String statusText = null;
                        boolean hasErrors, hasWarnings;
                        hasErrors = hasWarnings = false;

                        if (blockedReasons != null) {
                            for(int i = 0; i < blockedReasons.length; i++) {
                                hasWarnings |= !TextUtils.isEmpty(blockedReasons[i]);
                            }
                            if (!hasWarnings) {
                                blockedReasons = null;
                            }
                        }

                        if ((hasErrors || hasWarnings) && (blockedReasons != null)) {
                            int i, numUnknowns;
                            boolean low_toner, low_ink;
                            boolean no_toner, no_ink;
                            boolean door_open, check_printer;
                            boolean no_paper, jammed;
                            boolean printer_busy;

                            printer_busy = low_toner = low_ink = no_toner = no_ink = door_open = check_printer = no_paper = jammed = false;

                            for(i = numUnknowns = 0; i < blockedReasons.length; i++) {
                                if (TextUtils.isEmpty(blockedReasons[i])) {
                                }
                                else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__DOOR_OPEN)) {
                                    door_open = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__JAMMED)) {
                                    jammed = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__LOW_ON_INK)) {
                                    low_ink = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__LOW_ON_TONER)) {
                                    low_toner = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__OUT_OF_INK)) {
                                    no_ink = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__OUT_OF_TONER)) {
                                    no_toner = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__REALLY_LOW_ON_INK)) {
                                    low_ink = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__OUT_OF_PAPER)) {
                                    no_paper = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__SERVICE_REQUEST)) {
                                    check_printer = true;
                                } else if (blockedReasons[i].equals(MobilePrintConstants.BLOCKED_REASON__BUSY)) {
                                    printer_busy  = true;
                                } else {
                                    numUnknowns++;
                                }
                            }
                            if (numUnknowns == blockedReasons.length) {
                                check_printer = true;
                            }
                            final Resources resources = service.getResources();
                            if (door_open) {
                                statusText = resources.getString(R.string.printer_state__door_open);
                            } else if (jammed) {
                                statusText = resources.getString(R.string.printer_state__jammed);
                            } else if (no_paper) {
                                statusText = resources.getString(R.string.printer_state__out_of_paper);
                            } else if (check_printer) {
                                statusText = resources.getString(R.string.printer_state__check_printer);
                            } else if (no_ink) {
                                statusText = resources.getString(R.string.printer_state__out_of_ink);
                            } else if (no_toner) {
                                statusText = resources.getString(R.string.printer_state__out_of_toner);
                            } else if (low_ink) {
                                statusText = resources.getString(R.string.printer_state__low_on_ink);
                            } else if (low_toner) {
                                statusText = resources.getString(R.string.printer_state__low_on_toner);
                            } else if (printer_busy) {
                                statusText = resources.getString(R.string.printer_state__busy);
                            } else {
                                statusText = resources.getString(R.string.printer_state__unknown);
                            }
                        }

                        if (jobInfo.mPrintJob.isQueued()) {
                            jobInfo.mPrintJob.start();
                        }
                        jobInfo.mPrintJob.block(statusText);
                    }

                    else if (MobilePrintConstants.JOB_STATE_DONE.equals(jobStatus)) {
                        sendLowSuppliesRequest = jobInfo.mLowEmptySupplies;

                        service.mPrintJobIdMap.remove(jobInfo.mPrintJob.getId());
                        service.mPrintJobs.remove(jobID);
                        if (jobInfo.mUserCancelled && !jobInfo.mPrintJob.isCancelled()) {
                            // if job was cancelled by the user and the signal hasn't been sent
                            // send it now
                            jobInfo.mPrintJob.cancel();
                        }

                        String errorMessage = null;
                        if (TextUtils.isEmpty(doneResult) ||
                                MobilePrintConstants.JOB_DONE_OK.equals(doneResult)) {
                            jobInfo.mPrintJob.complete();
                        } else if (MobilePrintConstants.JOB_DONE_CORRUPT.equals(doneResult)) {
                            errorMessage = "finished with errors";
                        } else if (MobilePrintConstants.JOB_DONE_ERROR.equals(doneResult)) {
                            errorMessage = "print job failed";
                        } else if (MobilePrintConstants.JOB_DONE_CANCELLED.equals(doneResult)) {
                            errorMessage = "job was cancelled";
                        }

                        jobInfo.mFileCopy.delete();

                        if (TextUtils.isEmpty(errorMessage)) {
                            // handle the case where the job is blocked and then finishes
                            if (jobInfo.mPrintJob.isBlocked()) {
                                jobInfo.mPrintJob.start();
                            }
                            jobInfo.mPrintJob.complete();
                        } else {
                            jobInfo.mPrintJob.fail(errorMessage);
                        }

                    }
                }
                if (b.getBoolean(MobilePrintConstants.PRINT_STATUS_PAGE_START_INFO)) {
                }
                if (sendLowSuppliesRequest) {

                    Intent suppliesRequest = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_GET_SUPPLIES_HANDLING_INTENT);

                    try {
                        service.mPrintServiceMessenger.send(Message.obtain(null, 0, suppliesRequest));
                    } catch (RemoteException ignored) {
                    }
                }

            } else if (action.equals(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR)) {
                if (requestAction.equals(MobilePrintConstants.ACTION_PRINT_SERVICE_PRINT)) {
                    if (jobInfo != null) {
                        jobInfo.mPrintJob.fail(mContextRef.get().getString(R.string.fail_reason__could_not_start_job));
                    }
                }
            } else if (action.equals(MobilePrintConstants.ACTION_PRINT_SERVICE_PRINT)) {
                if (jobInfo.mPrintJob.isQueued()) {
                    jobInfo.mPrintJob.start();
                }
            }

        }
    }

    private PrintServiceCallbackHandler mPrintServiceCallbackHandler;
    private Messenger mPrintServiceCallbackMessenger;
    private Messenger mPrintServiceMessenger = null;

    private ServiceConnection mPrintServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            // initialize the wPrint messenger
            Messenger printServiceMessenger = new Messenger(service);

            // create a request to register for job status
            Message message = Message.obtain(null,
                    0,
                    new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_REGISTER_STATUS_RECEIVER));
            message.replyTo = mPrintServiceCallbackMessenger;

            // send the message
            try {
                printServiceMessenger.send(message);
            } catch (RemoteException e) {
            }
            mPrintServiceMessenger = printServiceMessenger;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            if (mPrintServiceMessenger == null) {
            }
            mPrintServiceMessenger = null;
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();

        // create the callback handler and messenger receiver
        mPrintServiceCallbackHandler = new PrintServiceCallbackHandler(this);
        mPrintServiceCallbackMessenger = new Messenger(mPrintServiceCallbackHandler);

        Intent bindIntent = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINT_SERVICE,
                null,
                ServiceAndroidPrint.this,
                com.android.wfds.printservice.WPrintService.class);
//        bindIntent.putExtra(PrintServiceStrings.PRINT_APP_ID_KEY, getResources().getString(R.string.wprint_application_id));

        // bind to the wPrint service
        bindService(bindIntent, mPrintServiceConnection, Context.BIND_AUTO_CREATE);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        // create a request to register for job status
        Message message = Message.obtain(null,
                0,
                new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_UNREGISTER_STATUS_RECEIVER));
        message.replyTo = mPrintServiceCallbackMessenger;

        try {
            if (mPrintServiceMessenger != null)
                mPrintServiceMessenger.send(message);
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        unbindService(mPrintServiceConnection);

        new AsyncTask<File,Void,Void>() {
            private void cleanUpFile(File dir) {
                if (dir == null) {
                    return;
                }
                if (dir.isDirectory()) {
                    File[] dirContents = dir.listFiles();
        
                    if (dirContents != null) {
                        for(File dirFile : dirContents) {
                            cleanUpFile(dirFile);
                        }
                    }
                    if (!getFilesDir().equals(dir)) {
                        dir.delete();
                    }
                } else {
                    dir.delete();
                }
            }
            @Override
            protected Void doInBackground(File... params) {
                if (params != null) {
                    for(File param : params) {
                        cleanUpFile(param);
                    }
                }
                return null;
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, getFilesDir());
    }

    @Override
    protected PrinterDiscoverySession onCreatePrinterDiscoverySession() {
        return new PrinterDiscoverySession() {

            private static final String BONJOUR_DOMAIN = ".local";
            private String selectName(String bonjourName, String bonjourDomainName, String model, String hostname, String address) {

                if (!TextUtils.isEmpty(bonjourName)) {
                    return bonjourName;
                }
                if (!TextUtils.isEmpty(model)) {
                    return model;
                }
                if (!TextUtils.isEmpty(hostname)) {
                    return hostname;
                }
                return address;
            }

            private Uri mDiscoveryURI;
            private AtomicBoolean mStarted = new AtomicBoolean(false);

            private HashMap<String, PrinterInfo> mPrinters = new HashMap<String, PrinterInfo>();
            private HashMap<String, AbstractAsyncTask<?,?,?>> mTrackedPrinters = new HashMap<String, AbstractAsyncTask<?, ?, ?>>();

            class WPrintDiscovery extends AbstractAsyncTask<Void,Bundle,Void> {

                public WPrintDiscovery(Context context) {
                    super(context);
                }

                private Handler mDiscoveryHandler = new Handler() {
                    @Override
                    public void handleMessage (Message msg) {

                        if ((msg != null) && (msg.obj instanceof Intent)) {
                            Bundle data = ((Intent)msg.obj).getExtras();
                            if (data != null) {
                                synchronized (foundPrinterList) {
                                    foundPrinterList.add(data);
                                    foundPrinterList.notifyAll();
                                }
                            }
                        }
                    }
                };
                private Messenger mDiscoveryCallbackMessenger = new Messenger(mDiscoveryHandler);

                private ArrayList<Bundle> foundPrinterList = new ArrayList<Bundle>();
                @Override
                protected Void doInBackground(Void... params) {
                    mDiscoveryURI = new Uri.Builder().scheme(MobilePrintConstants.SCHEME_DISCOVERY).path(UUID.randomUUID().toString()).build();
                    Intent discoveryRequest = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_START_DISCOVERY, mDiscoveryURI);
                    Message msg = Message.obtain(null, 0, discoveryRequest);
                    msg.replyTo = mDiscoveryCallbackMessenger;

                    waitForPrintServiceMessenger();

                    try {
                        mPrintServiceMessenger.send(msg);
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }

                    do {
                        synchronized (foundPrinterList) {
                        try {
                            if (foundPrinterList.isEmpty()) {
                                foundPrinterList.wait();
                            }
                            while(!foundPrinterList.isEmpty()) {
                                publishProgress(foundPrinterList.remove(0));
                            }
                        } catch (InterruptedException ignored) {
                        }
                        }
                    } while(!isCancelled());

                    return null;
                }
            }

            private WPrintDiscovery mDiscoveryTask = null;

            @Override
            public void onStartPrinterDiscovery(final List<PrinterId> printerIds) {

                onStopPrinterDiscovery();

                mDiscoveryTask = new WPrintDiscovery(ServiceAndroidPrint.this);
                mDiscoveryTask
                        .attach(new AbstractAsyncTask.AsyncTaskProgressCallback<Bundle>() {
                            @Override
                            public void onReceiveTaskProgress(AbstractAsyncTask<?, ?, ?> task, LinkedList<Bundle> progress, boolean wasCancelled) {
                                if (!wasCancelled) {
                                    ArrayList<PrinterInfo> printersToReport = new ArrayList<PrinterInfo>();
                                    for(Bundle data : progress) {
                                        String address = data.getString(MobilePrintConstants.DISCOVERY_DEVICE_ADDRESS);
                                        String bonjourName = data.getString(MobilePrintConstants.DISCOVERY_DEVICE_BONJOUR_NAME);
                                        String bonjourDomainName = data.getString(MobilePrintConstants.DISCOVERY_DEVICE_BONJOUR_DOMAIN_NAME);
                                        String hostname = data.getString(MobilePrintConstants.DISCOVERY_DEVICE_HOSTNAME);
                                        String model = data.getString(MobilePrintConstants.DISCOVERY_DEVICE_NAME);

                                        String name = selectName(bonjourName, bonjourDomainName, model, hostname, address);
                                        if (!TextUtils.isEmpty(address)) {
                                            PrinterInfo info = mPrinters.get(address);
                                            PrinterId id;
                                            if (info == null) {
                                                info = new PrinterInfo.Builder(generatePrinterId(address), name, PrinterInfo.STATUS_IDLE).setDescription(address).build();
                                            } else {
                                                final PrinterInfo oldInfo = info;
                                                info = new PrinterInfo.Builder(oldInfo).setName(name).setStatus(PrinterInfo.STATUS_IDLE).setDescription(address).build();
                                                if (info.equals(oldInfo)) {
                                                    // nothing changed
                                                    info = null;
                                                }
                                            }
                                            if (info != null) {
                                                printersToReport.add(info);
                                                mPrinters.put(address, info);
                                            }
                                        }
                                    }
                                    addPrinters(printersToReport);
                                }
                            }
                        })
                        .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            }

            @Override
            public void onStopPrinterDiscovery() {
                mStarted.set(false);
                if (mDiscoveryTask != null) {
                    mDiscoveryTask.detach().cancel(true);
                    mDiscoveryTask = null;
                }
                mPrinters.clear();
            }

            @Override
            public void onValidatePrinters(List<PrinterId> printerIds) {
            }

            private void cancelTracking(PrinterId printerId) {
                final AbstractAsyncTask<?, ? ,?> task = mTrackedPrinters.remove(printerId.getLocalId());
                if (task != null) {
                    task.cancel(true);
                }
            }

            @Override
            public void onStopPrinterStateTracking(PrinterId printerId) {
                cancelTracking(printerId);
            }

            @Override
            public void onStartPrinterStateTracking(final PrinterId printerId) {
                final String localID = printerId.getLocalId();
                cancelTracking(printerId);
                AbstractAsyncTask<Void, Object ,Void> task =
                        new AbstractAsyncTask<Void, Object, Void>(ServiceAndroidPrint.this) {

                            private String getPaperSizeName(List<String> supportedSizes, String paperSize) {

                                int id = 0;
                                // is this a region supported paper size?
                                if (supportedSizes.contains(paperSize)) {
                                    // yes it's supported

                                    // convert the name to something resources is happy with, (ie convert '-' & '.' to '_')
                                    paperSize = paperSize.replace('.', '_');
                                    paperSize = paperSize.replace('-', '_');

                                    // lookup the id
                                    id = getResources().getIdentifier(paperSize, "string", getPackageName());
                                }
                                return ((id > 0) ? getResources().getString(id) : null);
                            }

                            private Handler capsHandler= new Handler() {
                                @Override
                                public void handleMessage(Message msg) {

                                    boolean canDuplex = false;
                                    boolean canPrintColor = false;
                                    boolean hasPhotoTray = false;

                                    final Resources resources = getResources();

                                    if ((msg != null) && (msg.obj instanceof Intent)) {

                                        Intent reply = (Intent)msg.obj;


                                        if (TextUtils.equals(reply.getAction(),
                                                MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_PRINTER_CAPABILITIES)) {

                                            ArrayList<String> capsList;
                                            capsList = reply.getStringArrayListExtra(MobilePrintConstants.PRINT_COLOR_MODE);
                                            if (capsList != null) {
                                                for(String mode : capsList) {
                                                    canPrintColor |= ((mode != null) ? MobilePrintConstants.COLOR_SPACE_COLOR.equals(mode) : false);
                                                }
                                            }

                                            capsList = reply.getStringArrayListExtra(MobilePrintConstants.SIDES);
                                            if (capsList != null) {
                                                for(String mode : capsList ) {
                                                    canDuplex |= (MobilePrintConstants.SIDES_DUPLEX_LONG_EDGE.equals(mode) ||
                                                            MobilePrintConstants.SIDES_DUPLEX_SHORT_EDGE.equals(mode));
                                                }
                                            }

                                            capsList = reply.getStringArrayListExtra(MobilePrintConstants.MEDIA_SOURCE);
                                            if (capsList != null) {
                                                for(String tray : capsList) {
                                                    hasPhotoTray |= ((tray != null) ? MobilePrintConstants.MEDIA_TRAY_PHOTO.equals(tray) : false);
                                                }
                                            }


//                            mCapsBuilder.addInputTray(new PrintAttributes.Tray(
//                                    PrintServiceStrings.MEDIA_TRAY_AUTO, "auto"), true);
//                            if (hasPhotoTray) {
//                                mCapsBuilder.addInputTray(new PrintAttributes.Tray(
//                                        PrintServiceStrings.MEDIA_TRAY_PHOTO, "photo"), false);
//                            }

                                            String defaultMediaSize = resources.getString(R.string.default_media_size_name);
                                            ArrayList<String> paperSizeList = reply.getStringArrayListExtra(MobilePrintConstants.MEDIA_SIZE_NAME);
                                            if ((paperSizeList != null) && !paperSizeList.isEmpty()) {
                                                List<String> supportedPaperSizeList = Arrays.asList(resources.getStringArray(R.array.supported_paper_sizes));
                                                PrintAttributes.MediaSize mediaSize;
                                                for(String paperSize : paperSizeList) {
                                                    String paperSizeLabel = getPaperSizeName(supportedPaperSizeList, paperSize);
                                                    mediaSize = null;
                                                    if (!TextUtils.isEmpty(paperSizeLabel)) {
                                                        if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_LETTER)) {
                                                            mCapsBuilder.addMediaSize(PrintAttributes.MediaSize.NA_LETTER, TextUtils.equals(defaultMediaSize, MobilePrintConstants.MEDIA_SIZE_LETTER));
                                                        } else if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_A4)) {
                                                            mCapsBuilder.addMediaSize(PrintAttributes.MediaSize.ISO_A4, TextUtils.equals(defaultMediaSize, MobilePrintConstants.MEDIA_SIZE_A4));
                                                        } else if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_LEGAL)) {
                                                            mCapsBuilder.addMediaSize(PrintAttributes.MediaSize.NA_LEGAL, TextUtils.equals(defaultMediaSize, MobilePrintConstants.MEDIA_SIZE_LEGAL));
                                                        } else if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_PHOTO_4x6in)) {
                                                            mediaSize = new PrintAttributes.MediaSize(paperSize, paperSizeLabel, 4000, 6000);
                                                        } else if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_PHOTO_5x7)) {
                                                            mediaSize = new PrintAttributes.MediaSize(paperSize, paperSizeLabel, 5000, 7000);
                                                        } else if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_PHOTO_3_5x5)) {
                                                            mediaSize = new PrintAttributes.MediaSize(paperSize, paperSizeLabel, 3500, 5000);
                                                        } else if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_PHOTO_10x15cm)) {
                                                            mediaSize = new PrintAttributes.MediaSize(paperSize, paperSizeLabel, 4000, 6000);
                                                        } else if (TextUtils.equals(paperSize, MobilePrintConstants.MEDIA_SIZE_PHOTO_L)) {
                                                            mediaSize = new PrintAttributes.MediaSize(paperSize, paperSizeLabel, 3500, 5000);
                                                        }
                                                    }

                                                    if (mediaSize != null) {
                                                        mCapsBuilder.addMediaSize(mediaSize, TextUtils.equals(defaultMediaSize, mediaSize.getId()));
                                                    }
                                                }


                                            } else {
                                                mCapsBuilder.addMediaSize(PrintAttributes.MediaSize.NA_LETTER, TextUtils.equals(defaultMediaSize, MobilePrintConstants.MEDIA_SIZE_LETTER));
                                                mCapsBuilder.addMediaSize(PrintAttributes.MediaSize.ISO_A4, TextUtils.equals(defaultMediaSize, MobilePrintConstants.MEDIA_SIZE_A4));
                                            }


                                            mCapsBuilder.addResolution(new PrintAttributes.Resolution(
                                                    resources.getResourceName(R.id.android_print_id__resolution_300), resources.getString(R.string.resolution_300_dpi), 300, 300
                                            ), true);
                                            mCapsBuilder.addResolution(new PrintAttributes.Resolution(
                                                    resources.getResourceName(R.id.android_print_id__resolution_600), resources.getString(R.string.resolution_600_dpi), 600, 600
                                            ), false);

//                            mCapsBuilder.addOutputTray(new PrintAttributes.Tray(
//                                    resources.getResourceName(R.id.android_print_id__output_tray_main), "main"), true);

                                            mCapsBuilder.setColorModes(
                                                    PrintAttributes.COLOR_MODE_MONOCHROME | (canPrintColor ? PrintAttributes.COLOR_MODE_COLOR : 0),
                                                    (canPrintColor ? PrintAttributes.COLOR_MODE_COLOR : PrintAttributes.COLOR_MODE_MONOCHROME));
//                            mCapsBuilder.setDuplexModes(
//                                    PrintAttributes.DUPLEX_MODE_NONE |
//                                            (canDuplex ? (PrintAttributes.DUPLEX_MODE_LONG_EDGE | PrintAttributes.DUPLEX_MODE_SHORT_EDGE) : 0),
//                                    PrintAttributes.DUPLEX_MODE_NONE);
//                            mCapsBuilder.setFittingModes(
//                                    PrintAttributes.FITTING_MODE_SCALE_TO_FILL | PrintAttributes.FITTING_MODE_SCALE_TO_FIT | PrintAttributes.FITTING_MODE_NONE,
//                                    PrintAttributes.FITTING_MODE_SCALE_TO_FIT);
                                            mCapsBuilder.setMinMargins(new PrintAttributes.Margins(0, 0, 0, 0));
//                                    PrintAttributes.ORIENTATION_LANDSCAPE | PrintAttributes.ORIENTATION_PORTRAIT,
//                                    PrintAttributes.ORIENTATION_PORTRAIT);


                                            mProgress.add(mCapsBuilder);
                                            capsSem.release();

                                            ArrayList<String> supportedMimeTypes = reply.getStringArrayListExtra(MobilePrintConstants.MIME_TYPES);
                                            boolean hasPDF = ((supportedMimeTypes != null) ? supportedMimeTypes.contains(MIME_TYPE_PDF) : false);

                                            mProgress.add(Boolean.valueOf(reply.getBooleanExtra(MobilePrintConstants.IS_SUPPORTED, false) && hasPDF));
                                            capsSem.release();

                                        } else if (TextUtils.equals(reply.getAction(), MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_GET_PRINTER_STATUS)) {

                                            // assume we didn't receive anything
                                            int statusValue = PrinterInfo.STATUS_UNAVAILABLE;

                                            // retrieve the status
                                            String printerStatus = reply.getStringExtra(MobilePrintConstants.PRINTER_STATUS_KEY);
                                            // TODO
                                            // look for blocked conditions when the API has support for them

                                            if (!TextUtils.isEmpty(printerStatus)) {
                                                // process the status value
                                                if (printerStatus.equals(MobilePrintConstants.PRINTER_STATE_BLOCKED)) {
                                                    // assume blocked is busy
                                                    statusValue = PrinterInfo.STATUS_BUSY;
                                                } else if (printerStatus.equals(MobilePrintConstants.PRINTER_STATE_RUNNING)) {
                                                    // printer in use
                                                    statusValue = PrinterInfo.STATUS_IDLE;
                                                } else if (printerStatus.equals(MobilePrintConstants.PRINTER_STATE_IDLE)) {
                                                    // printer idle
                                                    statusValue = PrinterInfo.STATUS_IDLE;
                                                } else if (printerStatus.equals(MobilePrintConstants.PRINTER_STATE_UNKNOWN)) {
                                                    // no status available
                                                    // should we assume idle??
                                                    statusValue = PrinterInfo.STATUS_UNAVAILABLE;
                                                }
                                            }

                                            mProgress.add(Integer.valueOf(statusValue));
                                            capsSem.release();

                                        } else if (TextUtils.equals(reply.getAction(), MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR)) {
                                            String requestedAction = reply.getStringExtra(MobilePrintConstants.PRINT_REQUEST_ACTION);
                                            if (TextUtils.equals(requestedAction, MobilePrintConstants.ACTION_PRINT_SERVICE_START_MONITORING_PRINTER_STATUS)) {
                                                mProgress.add(Integer.valueOf(PrinterInfo.STATUS_UNAVAILABLE));
                                                capsSem.release();
                                            } else if (TextUtils.equals(requestedAction, MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINTER_CAPABILTIES)) {
                                                mProgress.add(Boolean.FALSE);
                                                capsSem.release();
                                            }
                                        }
                                    }
                                }
                            };
                            Messenger capsMessenger = new Messenger(capsHandler);


                            private PrinterCapabilitiesInfo.Builder mCapsBuilder = new PrinterCapabilitiesInfo.Builder(printerId);
                            Semaphore capsSem = new Semaphore(0);
                            Integer mStatus = Integer.valueOf(PrinterInfo.STATUS_IDLE);
                            Boolean mSupported = null;
                            PrinterCapabilitiesInfo mCapsInfo = null;

                            List<Object> mProgress = Collections.synchronizedList(new ArrayList<Object>());

                            @Override
                            protected Void doInBackground(Void... voids) {
                                waitForPrintServiceMessenger();
                                Message message;
                                Intent request;

                                String localID = printerId.getLocalId();

                                request = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_GET_PRINTER_CAPABILTIES);
                                request.putExtra(MobilePrintConstants.PRINTER_ADDRESS_KEY, localID);

                                message = Message.obtain(null, 0, request);
                                message.replyTo = capsMessenger;
                                try {
                                    mPrintServiceMessenger.send(message);
                                } catch (RemoteException e) {
                                    e.printStackTrace();
                                }

                                request = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_START_MONITORING_PRINTER_STATUS);
                                request.putExtra(MobilePrintConstants.PRINTER_ADDRESS_KEY, localID);
                                message = Message.obtain(null, 0, request);
                                message.replyTo = capsMessenger;
                                try {
                                    mPrintServiceMessenger.send(message);
                                } catch (RemoteException e) {
                                    e.printStackTrace();
                                }

                                while(!isCancelled()) {
                                    try {
                                        capsSem.acquire();
                                    } catch (InterruptedException ignored) {
                                    }
                                    if (!isCancelled()) {
                                        publishProgress(mProgress.remove(0));
                                    }
                                }

                                request = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_STOP_MONITORING_PRINTER_STATUS);
                                request.putExtra(MobilePrintConstants.PRINTER_ADDRESS_KEY, localID);
                                message = Message.obtain(null, 0, request);
                                message.replyTo = capsMessenger;
                                try {
                                    mPrintServiceMessenger.send(message);
                                } catch (RemoteException e) {
                                    e.printStackTrace();
                                }
                                return null;
                            }

                            @Override
                            protected void onProgressUpdate(Object... progress) {
                                Object data = progress[0];

                                if (data instanceof Integer) {
                                    mStatus = (Integer)data;
                                } else if (data instanceof PrinterCapabilitiesInfo.Builder) {
                                    mCapsInfo = ((PrinterCapabilitiesInfo.Builder)data).build();
                                } else if (data instanceof Boolean) {
                                    mSupported = (Boolean)data;
                                }
                                if ((mSupported != null) && (mStatus != null)) {
                                    PrinterInfo oldInfo = mPrinters.get(localID);
                                    if (oldInfo == null) {
                                        return;
                                    }
                                    PrinterInfo newInfo;
                                    if (mSupported) {
                                        newInfo = new PrinterInfo.Builder(oldInfo).setCapabilities(mCapsInfo).setStatus(mStatus).build();
                                    } else {
                                        newInfo = new PrinterInfo.Builder(oldInfo).setStatus(PrinterInfo.STATUS_UNAVAILABLE).build();
                                    }

                                    mPrinters.put(localID, newInfo);

                                    ArrayList<PrinterInfo> list = new ArrayList<PrinterInfo>(1);
                                    list.add(newInfo);
                                    addPrinters(list);
                                }
                            }

                        };
                task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                mTrackedPrinters.put(localID, task);
            }

            @Override
            public void onDestroy() {

            }
        };
    }

    private static class PrintInfoHolder {

        final PrintJob mPrintJob;

        boolean mUserCancelled = false;
        boolean mLowEmptySupplies = false;
        File mFileCopy;

        PrintInfoHolder(PrintJob printJob) {
            mPrintJob = printJob;
        }
    }

    private HashMap<PrintJobId, String> mPrintJobIdMap = new HashMap<PrintJobId, String>();
    private HashMap<String, PrintInfoHolder> mPrintJobs = new HashMap<String, PrintInfoHolder>();

    @Override
    public void onPrintJobQueued(final PrintJob printJob) {

        final PrintJobInfo printJobInfo = printJob.getInfo();
        final PrintAttributes printAttributes = printJobInfo.getAttributes();
        final PrintDocument printDocument = printJob.getDocument();
        final PrintDocumentInfo printDocumentInfo = printDocument.getInfo();
        final String jobID = UUID.randomUUID().toString();
        final ParcelFileDescriptor pFD = printDocument.getData();

        mPrintJobIdMap.put(printJob.getId(), jobID);

        new AbstractAsyncTask<Void, Void, Void>(ServiceAndroidPrint.this) {

            @Override
            protected Void doInBackground(Void... voids) {
                waitForPrintServiceMessenger();

                PrintInfoHolder printInfo = new PrintInfoHolder(printJob);

                printInfo.mFileCopy = new File(getFilesDir(), jobID + PDF_EXTENSION);

                InputStream in = new BufferedInputStream(new FileInputStream(pFD.getFileDescriptor()));
                OutputStream out = null;
                try {
                    out = new BufferedOutputStream(new FileOutputStream(printInfo.mFileCopy));
                    final byte[] buffer = new byte[8192];
                    while (true) {
                        if (isCancelled()) {
                            break;
                        }
                        final int readByteCount = in.read(buffer);
                        if (readByteCount < 0) {
                            break;
                        }
                        out.write(buffer, 0, readByteCount);
                    }
                } catch (IOException ioe) {
                    throw new RuntimeException(ioe);
                } finally {
                    try {
                        if (in != null) in.close();
                    } catch(IOException e) {

                    }
                    try {
                        if (out != null) out.flush();
                    } catch(IOException e) {

                    }
                    try {
                        if (out != null) out.close();
                    } catch(IOException e) {

                    }
                }

                Intent printJobRequest = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_PRINT);
                printJobRequest.setData(new Uri.Builder().scheme(MobilePrintConstants.SCHEME_JOB_ID).path(jobID).build());
                printJobRequest.setType(MIME_TYPE_PDF);

                printJobRequest.putExtra(MobilePrintConstants.PRINT_JOB_HANDLE_KEY, jobID);
                printJobRequest.putExtra(MobilePrintConstants.COPIES, printJobInfo.getCopies());

                boolean printAllPages = false;
                PageRange[] pageRanges = printJobInfo.getPages();
                if (pageRanges != null && pageRanges.length > 0) {

                    StringBuilder pageRangeBuilder = new StringBuilder();
                    for(PageRange range : pageRanges) {

                        if (range.equals(PageRange.ALL_PAGES)) {
                            printAllPages = true;

                        } else {
                            int start = range.getStart()+1;
                            int end   = range.getEnd()+1;

                            if (pageRangeBuilder.length() > 0) {
                                pageRangeBuilder.append(',');
                            }

                            if (start == end) {
                                pageRangeBuilder.append(String.valueOf(start));
                            } else if (start < end) {
                                pageRangeBuilder.append(String.valueOf(start));
                                pageRangeBuilder.append('-');
                                pageRangeBuilder.append(String.valueOf(end));
                            }
                        }
                    }
                    printAllPages |= (pageRangeBuilder.length() == 0);
                    printJobRequest.putExtra(MobilePrintConstants.PAGE_RANGE, pageRangeBuilder.toString());

                } else {
                    printAllPages = true;
                }
                if (printAllPages) {
                    printJobRequest.removeExtra(MobilePrintConstants.PAGE_RANGE);
                }

                printJobRequest.putExtra(MobilePrintConstants.PRINTER_ADDRESS_KEY, printJobInfo.getPrinterId().getLocalId());

                printJobRequest.putExtra(MobilePrintConstants.PRINT_COLOR_MODE,
                        ((printAttributes.getColorMode() == PrintAttributes.COLOR_MODE_MONOCHROME) ?
                                MobilePrintConstants.COLOR_SPACE_MONOCHROME : MobilePrintConstants.COLOR_SPACE_COLOR));

                printJobRequest.putExtra(MobilePrintConstants.FILL_PAGE, false);
                printJobRequest.putExtra(MobilePrintConstants.FIT_TO_PAGE, false);

                String[] fileList = new String[1];
                fileList[0] = printInfo.mFileCopy.getAbsolutePath();
                printJobRequest.putExtra(MobilePrintConstants.PRINT_FILE_LIST, fileList);


                PrintAttributes.MediaSize paperSize = printJobInfo.getAttributes().getMediaSize();
                String paperSizeID = ((paperSize != null) ? paperSize.getId() : null);
                if (TextUtils.isEmpty(paperSizeID)) {
                    paperSizeID = MobilePrintConstants.MEDIA_SIZE_LETTER;
                }

                if (paperSize == null) {

                } else if (PrintAttributes.MediaSize.NA_LETTER.equals(paperSize)) {
                    paperSizeID = MobilePrintConstants.MEDIA_SIZE_LETTER;
                } else if (PrintAttributes.MediaSize.ISO_A4.equals(paperSize)) {
                    paperSizeID = MobilePrintConstants.MEDIA_SIZE_A4;
                } else if (PrintAttributes.MediaSize.NA_LEGAL.equals(paperSize)) {
                    paperSizeID = MobilePrintConstants.MEDIA_SIZE_LEGAL;
                } else {
                    paperSizeID = paperSize.getId();
                }

                printJobRequest.putExtra(MobilePrintConstants.MEDIA_SIZE_NAME, paperSizeID);

//                switch(printAttributes.getFittingMode()) {
//                    case PrintAttributes.FITTING_MODE_SCALE_TO_FIT:
//                        printJobRequest.putExtra(PrintServiceStrings.FIT_TO_PAGE, true);
//                        break;
//                    case PrintAttributes.FITTING_MODE_SCALE_TO_FILL:
//                        printJobRequest.putExtra(PrintServiceStrings.FILL_PAGE, true);
//                    default:
//                        break;
//                }

                SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(ServiceAndroidPrint.this);

                printJobRequest.putExtra(MobilePrintConstants.SIDES, MobilePrintConstants.SIDES_SIMPLEX);
                printJobRequest.putExtra(MobilePrintConstants.FULL_BLEED, MobilePrintConstants.FULL_BLEED_OFF);
                printJobRequest.putExtra(MobilePrintConstants.PRINT_DOCUMENT_CATEGORY, MobilePrintConstants.PRINT_DOCUMENT_CATEGORY__DOCUMENT);
                switch(printDocumentInfo.getContentType()) {
                    case PrintDocumentInfo.CONTENT_TYPE_PHOTO: {
                        printJobRequest.putExtra(MobilePrintConstants.FULL_BLEED, MobilePrintConstants.FULL_BLEED_ON);
                        printJobRequest.putExtra(MobilePrintConstants.FILL_PAGE, true);
                        printJobRequest.putExtra(MobilePrintConstants.PRINT_DOCUMENT_CATEGORY, MobilePrintConstants.PRINT_DOCUMENT_CATEGORY__PHOTO);
                        break;
                    }

                    case PrintDocumentInfo.CONTENT_TYPE_DOCUMENT:
                        printJobRequest.putExtra(MobilePrintConstants.PRINT_DOCUMENT_CATEGORY, MobilePrintConstants.PRINT_DOCUMENT_CATEGORY__DOCUMENT);
                        boolean duplex = prefs.getBoolean(getString(R.string.preference_key__two_sided_enabled), getResources().getBoolean(R.bool.default__two_sided_enabled));
                        if (duplex) {
                            printJobRequest.putExtra(MobilePrintConstants.SIDES, MobilePrintConstants.SIDES_DUPLEX_LONG_EDGE);
                        }
                        printJobRequest.putExtra(MobilePrintConstants.FIT_TO_PAGE, true);
                        break;

                    default:
                        break;
                }

                printJobRequest.putExtra(MobilePrintConstants.ORIENTATION_REQUESTED, MobilePrintConstants.ORIENTATION_AUTO);

//                switch(printAttributes.getOrientation()) {
//                    case PrintAttributes.ORIENTATION_PORTRAIT:
//                        printJobRequest.putExtra(MobilePrintConstants.ORIENTATION_REQUESTED, MobilePrintConstants.ORIENTATION_PORTRAIT);
//                        break;
//                    case PrintAttributes.ORIENTATION_LANDSCAPE:
//                        printJobRequest.putExtra(MobilePrintConstantsMobilePrintConstants.ORIENTATION_REQUESTED, MobilePrintConstants.ORIENTATION_LANDSCAPE);
//                        break;
//                    default:
//                        break;
//                }

//                printJobRequest.putExtra(MobilePrintConstants.MEDIA_SOURCE, printAttributes.getInputTray().getId());
                printJobRequest.putExtra(MobilePrintConstants.ALIGNMENT, MobilePrintConstants.ALIGN_CENTER);

                mPrintJobs.put(jobID, printInfo);

                Message printRequest = Message.obtain(null, 0, printJobRequest);
                printRequest.replyTo = mPrintServiceCallbackMessenger;

                try {
                    mPrintServiceMessenger.send(printRequest);
                } catch(RemoteException e) {
                    printJob.fail(getString(R.string.fail_reason__service_failed));
                    mPrintJobs.remove(jobID);
                    mPrintJobIdMap.remove(printJob.getId());
                }

                return null;
            }

        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    @Override
    protected void onRequestCancelPrintJob(final PrintJob printJob) {

        final String jobID = mPrintJobIdMap.get(printJob.getId());
        PrintInfoHolder info = mPrintJobs.get(jobID);
        info.mUserCancelled = true;

        final PrintInfoHolder jobInfo = mPrintJobs.get(jobID);

        if (jobInfo == null) {
            return;
        }

        new AbstractAsyncTask<Void, Void, Void>(ServiceAndroidPrint.this) {

            Handler mCancelHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    if ((msg == null) || !(msg.obj instanceof Intent)) {
                        return;
                    }

                    Intent cancelResult = (Intent)msg.obj;

                    String cancelJobID = cancelResult.getStringExtra(MobilePrintConstants.PRINT_JOB_HANDLE_KEY);

                    if (jobID.equals(cancelJobID) && cancelResult.getAction().equals(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_CANCEL_JOB)) {
                        printJob.cancel();
                    }
                }
            };
            private Messenger cancelMessenger = new Messenger(mCancelHandler);

            @Override
            protected Void doInBackground(Void... voids) {
                waitForPrintServiceMessenger();

                Intent cancelRequest = new Intent(MobilePrintConstants.ACTION_PRINT_SERVICE_CANCEL_PRINT_JOB);
                cancelRequest.putExtra(MobilePrintConstants.PRINT_JOB_HANDLE_KEY, jobID);

                Message msg = Message.obtain(null, 0, cancelRequest);
                msg.replyTo= cancelMessenger;
                try {
                    mPrintServiceMessenger.send(msg);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }

                return null;
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void waitForPrintServiceMessenger() {
        while(mPrintServiceMessenger == null) {
            try {
                Thread.sleep(25);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
