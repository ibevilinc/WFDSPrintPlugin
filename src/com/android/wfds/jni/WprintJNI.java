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

import com.android.wfds.common.statusmonitor.WPrintPrinterStatusMonitor;
import com.android.wfds.printservice.WPrintService.JobHandler;

public class WprintJNI implements IwprintJNI {

    @Override
    public int callNativeGetCapabilities(String address, int port,
                                         wPrintPrinterCapabilities capabilities) {
        return nativeGetCapabilities(address, port, capabilities);
    }

    @Override
    public int callNativeGetDefaultJobParameters(String address, int port,
                                                 wPrintJobParams jobParams, wPrintPrinterCapabilities capabilities) {
        return nativeGetDefaultJobParameters(address, port, jobParams, capabilities);
    }

    @Override
    public int callNativeGetFinalJobParameters(String address, int port,
                                               wPrintJobParams jobParams, wPrintPrinterCapabilities capabilities) {
        return nativeGetFinalJobParameters(address, port, jobParams, capabilities);
    }

    @Override
    public int callNativeCancelJob(int jobID) {
        return nativeCancelJob(jobID);
    }

    @Override
    public int callNativeResumeJob(int jobID) {
        return nativeResumeJob(jobID);
    }

    @Override
    public void callNativeMonitorStatusStart(int statusID,
                                             WPrintPrinterStatusMonitor monitor) {
        nativeMonitorStatusStart(statusID, monitor);
    }

    @Override
    public void callNativeMonitorStatusStop(int statusID) {
        nativeMonitorStatusStop(statusID);
    }

    @Override
    public int callNativeStartJob(String address, int port, String mime_type,
                                  wPrintJobParams jobParams, wPrintPrinterCapabilities capabilities,
                                  String[] fileList, String debugDir) {
        return nativeStartJob(address, port, mime_type, jobParams, capabilities, fileList, debugDir);
    }

    @Override
    public int callNativeEndJob(int jobID) {
        return nativeEndJob(jobID);
    }

    @Override
    public wPrintCallbackParams callNativeGetPrinterStatus(String address,
                                                           int port) {
        return nativeGetPrinterStatus(address, port);
    }

    @Override
    public int callNativeMonitorStatusSetup(String address, int port) {
        return nativeMonitorStatusSetup(address, port);
    }

    @Override
    public int callNativeInit(JobHandler callbackReceiver, String dataDir,
                              String[] pluginFileList) {
        return nativeInit(callbackReceiver, dataDir, pluginFileList);
    }

    @Override
    public void callNativeSetApplicationID(String appID) {
        nativeSetApplicationID(appID);
    }

    @Override
    public int callNativeSetTimeouts(int tcp_timeout, int udp_timeout,
                                     int udp_retries) {
        return nativeSetTimeouts(tcp_timeout, udp_timeout, udp_retries);
    }

    @Override
    public void callNativeSetLogLevel(int level) {
        nativeSetLogLevel(level);
    }

    @Override
    public int callNativeExit() {
        return nativeExit();
    }

    private native int nativeGetCapabilities(String address, int port,
                                             wPrintPrinterCapabilities capabilities);
    private native int nativeGetDefaultJobParameters(String address, int port,
                                                     wPrintJobParams jobParams, wPrintPrinterCapabilities capabilities);
    private native int nativeCancelJob(int jobID);
    private native int nativeResumeJob(int jobID);
    private native void nativeMonitorStatusStart(int statusID,
                                                 WPrintPrinterStatusMonitor monitor);
    private native void nativeMonitorStatusStop(int statusID);
    private native int nativeGetFinalJobParameters(String address, int port,
                                                   wPrintJobParams jobParams, wPrintPrinterCapabilities capabilities);
    public native int nativeStartJob(String address, int port,
                                     String mime_type, wPrintJobParams jobParams,
                                     wPrintPrinterCapabilities capabilities, String[] fileList,
                                     String debugDir);
    public native int nativeEndJob(int jobID);
    public native wPrintCallbackParams nativeGetPrinterStatus(String address,
                                                              int port);
    public native int nativeMonitorStatusSetup(String address, int port);

    public native int nativeInit(JobHandler callbackReceiver, String dataDir,
                                 String[] pluginFileList);
    public native void nativeSetApplicationID(String appID);
    public native int nativeSetTimeouts(int tcp_timeout, int udp_timeout,
                                        int udp_retries);
    public native void nativeSetLogLevel(int level);
    public native int nativeExit();

}
