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

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.UUID;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.job.IPrintJob;
import com.android.wfds.common.job.PrintJob;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.printerinfo.IPrinterInfo;
import com.android.wfds.jni.IwprintJNI;
import com.android.wfds.jni.wPrintJobParams;
import com.android.wfds.jni.wPrintPrinterCapabilities;
import com.android.wfds.printservice.WPrintService;

public class StartJobTask extends AbstractPrinterInfoTask {

	private static final String TAG = GetPrinterCapabilitiesTask.class
			.getSimpleName();

	private WeakReference<WPrintService> mServiceRef;
	private IwprintJNI mJNI;

	public StartJobTask(AbstractMessage msg, WPrintService service,
			IwprintJNI jni) {
		super(msg, service.getJobHandler(), jni);
		mServiceRef = new WeakReference<WPrintService>(service);
		mJNI = jni;
	}

	@Override
	protected Intent doInBackground(Void... params) {
		String address = null;
		String errorResult = null;
		String jobUUID = findJobUUID(mIntent);
		int result = -1;
		// Coverity 72910 fix
		if (mBundleData == null) {
			// TODO return an error in this case to the client
			Log.e(TAG,
					"Print address not present in bundle, can't start print job!");
			return null;
		}

		address = mBundleData
				.getString(MobilePrintConstants.PRINTER_ADDRESS_KEY);

		IPrinterInfo entry = getPrinterInfo(address, mBundleData);
		result = ((entry == null) ? -1 : 0);

		String mimeType = null;
		if (null == mIntent || TextUtils.isEmpty(mIntent.getType())) {
			mimeType = MobilePrintConstants.MIME_TYPE_FALLBACK;
		} else {
			mimeType = mIntent.getType();
			if (mimeType.equals("image/*")) {
				mimeType = MobilePrintConstants.MIME_TYPE_FALLBACK;
			}
		}

		String[] fileList = null;
		if (mIntent != null) {
			Uri uri = mIntent.getData();
			if (uri != null) {
				String scheme = uri.getScheme();
				String path = uri.getPath();
				if (!TextUtils.isEmpty(scheme)) {
					if (scheme.equals(ContentResolver.SCHEME_FILE)) {
						if (!TextUtils.isEmpty(path)) {
							fileList = new String[1];
							fileList[0] = path;
						}
					} // no need for else since fileList will be null and
						// missing-file-error will be returned
				}
			}
		}
		if (!TextUtils.isEmpty(jobUUID)) {
			mBundleData.putString(MobilePrintConstants.PRINT_JOB_HANDLE_KEY,
					jobUUID);
		}
		if (mBundleData.containsKey(MobilePrintConstants.PRINT_FILE_LIST)) {
			fileList = mBundleData
					.getStringArray(MobilePrintConstants.PRINT_FILE_LIST);
		}
		if ((fileList != null) && (fileList.length != 0)) {

            if (entry != null) {
				// local wifi job
				wPrintJobParams jobParams = new wPrintJobParams(
						(wPrintJobParams) entry.getDefaultJobParams());
				jobParams.updateJobParams(mBundleData);
				mJNI.callNativeGetFinalJobParameters(address, entry.getPortNum(),
						jobParams,
						(wPrintPrinterCapabilities) entry.getPrinterCaps());

				ArrayList<String> resolvedFileList = new ArrayList<String>(
						fileList.length);
				for (String file : fileList) {
					resolvedFileList.add(file);
				}

				if (result >= 0) {
					String[] resolvedFileListArray = new String[resolvedFileList
							.size()];
					resolvedFileList.toArray(resolvedFileListArray);

					String debugDir = null;
					File debugFile = new File(Environment.getExternalStorageDirectory() + "/wprintdbg/" + mServiceRef.get().getPackageName());
					if (debugFile.exists()) {
						File parentDir = new File(debugFile.getParentFile(), UUID.randomUUID().toString());
						parentDir.mkdirs();
						debugDir = parentDir.getAbsolutePath();
					}

					result = mJNI.callNativeStartJob(address,
							entry.getPortNum(), mimeType, jobParams,
							(wPrintPrinterCapabilities) entry.getPrinterCaps(),
							resolvedFileListArray, debugDir);
				}
				Log.d("WPRINT", "job start result: " + result);
			} else {
				result = -1;
				Log.d("WPRINT", "job start result: " + result);
			}

		} else {
			result = -1;
		}

		Intent returnIntent = new Intent();
		if (result >= 0) {
			returnIntent
					.setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_PRINTER_STARTED);
			IPrintJob printJob = null;
			printJob = new PrintJob(result, mBundleData, mJNI);

			mServiceRef.get().getJobHandler().addJobToList(printJob);
			returnIntent.putExtras(mBundleData);
		} else {
			Bundle response = new Bundle();
			returnIntent
					.setAction(MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_ERROR);
			if (fileList == null || fileList.length == 0) {
				response.putString(MobilePrintConstants.PRINT_ERROR_KEY,
						MobilePrintConstants.MISSING_FILE_ERROR);
			} else {
				response.putString(
						MobilePrintConstants.PRINT_ERROR_KEY,
						(errorResult == null) ? MobilePrintConstants.COMMUNICATION_ERROR
								: errorResult);
			}
			returnIntent.putExtras(response);
		}
		if (!TextUtils.isEmpty(address)) {
			returnIntent.putExtra(MobilePrintConstants.PRINTER_ADDRESS_KEY,
					address);
		}
		if (!TextUtils.isEmpty(jobUUID)) {
			returnIntent.putExtra(MobilePrintConstants.PRINT_JOB_HANDLE_KEY,
					jobUUID);
		}

		if (mIntent != null) {
			returnIntent.putExtra(MobilePrintConstants.PRINT_REQUEST_ACTION,
					mIntent.getAction());
		}
		return returnIntent;
	}
}
