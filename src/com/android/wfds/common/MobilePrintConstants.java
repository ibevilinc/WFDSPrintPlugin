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
package com.android.wfds.common;

public class MobilePrintConstants extends com.hp.android.printplugin.support.PrintServiceStrings
{
    public static final String TAG = "PRINT_LIB";

    public static final String PRINT_SERVICE_JOB_THREAD_ID = "JobThread";

    public static final String MIME_TYPE__PCLM = "application/PCLm";
    public static final String MIME_TYPE__PDF = "application/pdf";
    public static final String MIME_TYPE__PWG_RASTER = "image/pwg-raster";

    public static final String JOB_MARGIN_TOP = "job-margin-top";
    public static final String JOB_MARGIN_LEFT = "job-margin-left";
    public static final String JOB_MARGIN_RIGHT = "job-margin-right";
    public static final String JOB_MARGIN_BOTTOM = "job-margin-bottom";

    // Ports
    public static final int PORT_ERROR = -1;
    public static final int PORT_IPP = 631;
    public static final int[] PORT_CHECK_ORDER_IPP = { PORT_IPP };

    // generic
    public static final int CAPABILITIES_CACHE_SIZE = 25;
    public static final String OK = "OK";
    public static final String ERROR = "ERROR";
    
    public static final int WPRINT_SERVICE_MSG__INIT                            = 0;
    public static final int WPRINT_SERVICE_MSG__EXIT                            = 1;
    public static final int WPRINT_SERVICE_MSG__GET_CAPABILITIES                = 2;
    public static final int WPRINT_SERVICE_MSG__GET_FINAL_JOB_PARAMETERS        = 3;
    public static final int WPRINT_SERVICE_MSG__START_JOB                       = 4;
    public static final int WPRINT_SERVICE_MSG__CANCEL_JOB                      = 5;
    public static final int WPRINT_SERVICE_MSG__RESUME_JOB                      = 6;
    public static final int WPRINT_SERVICE_MSG__JOB_STATUS                      = 7;
    public static final int WPRINT_SERVICE_MSG__CANCEL_ALL                      = 8;
    public static final int WPRINT_SERVICE_MSG__REGISTER_STATUS_RECEIVER        = 9;
    public static final int WPRINT_SERVICE_MSG__UNREGISTER_STATUS_RECEIVER      = 10;
    public static final int WPRINT_SERVICE_MSG__GET_JOB_STATUS                  = 11;
    public static final int WPRINT_SERVICE_MSG__GET_ALL_JOBS_STATUS             = 12;
    public static final int WPRINT_SERVICE_MSG__SERVICE_BIND                    = 13;
    public static final int WPRINT_SERVICE_MSG__GET_PRINTER_STATUS              = 14;
    public static final int WPRINT_SERVICE_MSG__START_MONITORING_PRINTER_STATUS = 15;
    public static final int WPRINT_SERVICE_MSG__STOP_MONITORING_PRINTER_STATUS  = 16;
    public static final int WPRINT_SERVICE_MSG__PRINTER_STATUS                  = 17;
    public static final int WPRINT_SERVICE_MSG__SERVICE_UNBIND                  = 18;
    public static final int WPRINT_SERVICE_MSG__TASK_COMPLETE                   = 19;
    public static final int WPRINT_SERVICE_MSG__GET_SUPPLIES_HANDLING_INTENT    = 20;
}
