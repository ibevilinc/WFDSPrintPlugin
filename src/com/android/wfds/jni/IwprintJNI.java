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

public interface IwprintJNI {

    public int callNativeGetCapabilities(String address, int port,
                                         wPrintPrinterCapabilities capabilities);
    public int callNativeGetDefaultJobParameters(String address, int port,
                                                 wPrintJobParams jobParams, wPrintPrinterCapabilities capabilities);
    public int callNativeGetFinalJobParameters(String address, int port,
                                               wPrintJobParams jobParams, wPrintPrinterCapabilities capabilities);
    public void callNativeMonitorStatusStart(int statusID,
                                             WPrintPrinterStatusMonitor monitor);
    public void callNativeMonitorStatusStop(int statusID);
    public int callNativeCancelJob(int jobID);
    public int callNativeResumeJob(int jobID);
    public int callNativeStartJob(String address, int port,
                                  String mime_type, wPrintJobParams jobParams,
                                  wPrintPrinterCapabilities capabilities, String[] fileList, String debugDir);
    public int callNativeEndJob(int jobID);
    public wPrintCallbackParams callNativeGetPrinterStatus(String address,
                                                           int port);
    public int callNativeMonitorStatusSetup(String address, int port);
    public int callNativeInit(JobHandler callbackReceiver, String dataDir,
                              String[] pluginFileList);
    public void callNativeSetApplicationID(String appID);

    public int callNativeSetTimeouts(int tcp_timeout, int udp_timeout,
                                     int udp_retries);
    public void callNativeSetLogLevel(int level);
    public int callNativeExit();
}
