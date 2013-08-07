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
#ifndef __LIB_WPRINT_H__
#define __LIB_WPRINT_H__

#include "wtypes.h"
#include "wprint_df_types.h"
#include "mime_types.h"
#include "printer_capabilities_types.h"
#include "wprint_status_types.h"
#include "ifc_osimg.h"

#define WPRINT_BAD_JOB_HANDLE    ((wJob_t) ERROR)

#define _INTERFACE_MAJOR_VERSION  1
#define _INTERFACE_MINOR_VERSION  0

#define _PLUGIN_MAJOR_VERSION 1

#define WPRINT_INTERFACE_VERSION ((uint32)((_INTERFACE_MAJOR_VERSION << 16) | (_INTERFACE_MINOR_VERSION & 0xffff)))
#define WPRINT_PLUGIN_VERSION(X) ((uint32)((_PLUGIN_MAJOR_VERSION << 16) | (X & 0xffff)))

#define major_version(X)  ((X >> 16) & 0xffff)
#define minor_version(X)  ((X >>  0) & 0xffff)

#define STRIPE_HEIGHT    16
#define BUFFERED_ROWS    (STRIPE_HEIGHT * 8)

#define  MAX_MIME_LENGTH             64
#define  MAX_PRINTER_ADDR_LENGTH     64
#define  MAX_FILENAME_LENGTH         32
#define  MAX_PATHNAME_LENGTH        256

#ifdef __cplusplus
extern "C"
{
#endif

typedef PT_size_t media_size_t;


typedef enum 
{
   DUPLEX_DRY_TIME_NORMAL,     /*  18 seconds  */
   DUPLEX_DRY_TIME_LOWER,      /*  10 seconds  */
   DUPLEX_DRY_TIME_MINIMUM     /*   5 seconds  */

}  duplex_dry_time_t;


typedef enum
{
   PORT_INVALID = -1,
   PORT_FILE    = 0,
   PORT_IPP     = 631,

}  port_t;

typedef enum
{
   PCLNONE,
   PCLm,
   PCLJPEG,
   PCLPWG,

   PCL_NUM_TYPES

}  pcl_t;

typedef enum
{
    AUTO_ROTATE,
    CENTER_VERTICAL,
    CENTER_HORIZONTAL,
    ROTATE_BACK_PAGE,
    BACK_PAGE_PREROTATED,
    AUTO_SCALE,
    AUTO_FIT,
    PORTRAIT_MODE,
    LANDSCAPE_MODE,
    CENTER_ON_ORIENTATION,
} render_flags_t;

#define RENDER_FLAG_AUTO_ROTATE           (1 << AUTO_ROTATE)
#define RENDER_FLAG_ROTATE_BACK_PAGE      (1 << ROTATE_BACK_PAGE)
#define RENDER_FLAG_BACK_PAGE_PREROTATED  (1 << BACK_PAGE_PREROTATED)
#define RENDER_FLAG_CENTER_VERTICAL       (1 << CENTER_VERTICAL)
#define RENDER_FLAG_CENTER_HORIZONTAL     (1 << CENTER_HORIZONTAL)
#define RENDER_FLAG_AUTO_SCALE            (1 << AUTO_SCALE)
#define RENDER_FLAG_AUTO_FIT              (1 << AUTO_FIT)
#define RENDER_FLAG_PORTRAIT_MODE         (1 << PORTRAIT_MODE)
#define RENDER_FLAG_LANDSCAPE_MODE        (1 << LANDSCAPE_MODE)
#define RENDER_FLAG_CENTER_ON_ORIENTATION (1 << CENTER_ON_ORIENTATION)

#define AUTO_SCALE_RENDER_FLAGS          (RENDER_FLAG_AUTO_SCALE | \
                                          RENDER_FLAG_AUTO_ROTATE | \
                                          RENDER_FLAG_CENTER_VERTICAL | \
                                          RENDER_FLAG_CENTER_HORIZONTAL)

#define AUTO_FIT_RENDER_FLAGS            (RENDER_FLAG_AUTO_FIT | \
                                          RENDER_FLAG_AUTO_ROTATE | \
                                          RENDER_FLAG_CENTER_ON_ORIENTATION)

#define ORIENTATION_RENDER_FLAGS         (RENDER_FLAG_AUTO_ROTATE | \
                                          RENDER_FLAG_PORTRAIT_MODE | \
                                          RENDER_FLAG_LANDSCAPE_MODE | \
                                          RENDER_FLAG_CENTER_ON_ORIENTATION)

typedef void (*wprintSyncCb_t)  (wJob_t  job_id, void  *parm);

typedef struct
{
   DF_media_size_t        media_size;
   DF_media_type_t        media_type;
   DF_quality_t           print_quality;
   DF_duplex_t            duplex;
   duplex_dry_time_t      dry_time;
   DF_color_space_t       color_space;
   PT_media_src_t         media_tray;
   unsigned int           num_copies;
   bool                   borderless;
   unsigned int           render_flags;

   float                  job_top_margin;
   float                  job_left_margin;
   float                  job_right_margin;
   float                  job_bottom_margin;

   /* variables below this line should not be changed by anything other than wprint */
   bool                   renderInReverseOrder;
   bool                   mono1Bit;

   /* these values are pixels */
   unsigned int           print_top_margin;
   unsigned int           print_left_margin;
   unsigned int           print_right_margin;
   unsigned int           print_bottom_margin;

   /* these values are in pixels */
   unsigned int           pixel_units;
   unsigned int           width;
   unsigned int           height;
   unsigned int           printable_area_width;
   unsigned int           printable_area_height;
   unsigned int           strip_height;

   bool                   cancelled;
   bool                   last_page;
   int                    page_num;
   int                    copy_num;
   int                    copy_page_num;
   int                    page_corrupted;
   bool                   page_printing;
   bool                   page_backside;

   /* these values are in inches */
   float                  page_width;
   float                  page_height;
   float                  page_top_margin;
   float                  page_left_margin;
   float                  page_right_margin;
   float                  page_bottom_margin;

   char                   *print_format;
   pcl_t                  pcl_type;
   void                   *plugin_data;
   bool 				  ipp_1_0_supported;
   bool					  ipp_2_0_supported;
   bool					  ePCL_ipp_supported;
   const char             *useragent;
   char                   docCategory[10];

}  wprint_job_params_t;

typedef enum
{
   JOB_QUEUED = 1,
   JOB_RUNNING,
   JOB_BLOCKED,
   JOB_DONE

}  wprint_job_state_t;

typedef enum
{
   WPRINT_CB_PARAM_JOB_STATE,
   WPRINT_CB_PARAM_PAGE_INFO,
   WPRINT_CB_PARAM_PAGE_START_INFO,

}  wprint_cb_param_id_t;

typedef struct 
{
   char                printer_addr[MAX_PRINTER_ADDR_LENGTH];
   char                pathname[MAX_PATHNAME_LENGTH];
   wprint_job_state_t  state;

   unsigned int blocked_reasons;
   int job_done_result;

   int current_page;
   int total_pages;

} wprint_job_desc_t;

typedef struct
{
   int                 page_num;
   int                 copy_num;
   int                 corrupted_file;
   int                 current_page;
   int                 total_pages;
   int                 page_total_update;

} wprint_page_info_t;

typedef struct
{
   wprint_cb_param_id_t   id;

   union
   {
      wprint_job_state_t       state;
      wprint_page_info_t       page_info;

   }  param;

   unsigned int blocked_reasons;
   int job_done_result;

} wprint_job_callback_params_t;


typedef struct
{
   uint32         version;

   char const ** (*get_mime_types)     (void);
   char const ** (*get_print_formats)  (void);

   int    (*start_job)      ( wJob_t               job_handle,
                              void                 *wprint_ifc,
                              void                 *print_ifc,
                              wprint_job_params_t  *job_params);

   int     (*print_page)    ( wJob_t               job_handle,
                              wprint_job_params_t  *job_params,
                              char                 *mime_type,
                              char                 *pathname);

   int     (*print_blank_page) ( wJob_t               job_handle,
                                 wprint_job_params_t  *job_params);

   int     (*end_job)       ( wJob_t               job_handle,
                              wprint_job_params_t  *job_params);

}  wprint_plugin_t;

typedef wprint_plugin_t * (*plugin_reg_t) (void);

/**
 * wprintInit():  Initialize the library. Identify and gather capabilities of 
 * available plug-ins. 
 * Returns the number of plugins found or ERROR
 **/

int wprintInit(void);

/**
 * wprintGetCapabilities(): Gets the capabilities of the specified printer
 * Returns OK or ERROR
 **/

int wprintGetCapabilities(const char               *printer_addr,
                          int                      port_num,
                          printer_capabilities_t   *printer_cap);



/**
 * wprintGetDefaultJobParams(): Fills in the job params structure with default
 * values. 
 * Returns OK or ERROR
 **/

int wprintGetDefaultJobParams(const char           *printer_addr,
                              int                  port_num,
                              wprint_job_params_t  *job_params,
                              printer_capabilities_t   *printer_cap);

/**
 * wprintGetFinalJobParams(): Fills in the job params structure with default
 * values. 
 * Returns OK or ERROR
 **/

int wprintGetFinalJobParams(const char           *printer_addr,
                            int                  port_num,
                            wprint_job_params_t  *job_param,
                            printer_capabilities_t   *printer_cap);


/**
 * wprintStartJob():  Sent once per job at the start of the job. Returns a print
 * job handle that is used in other functions of this library. 
 * Returns WPRINT_BAD_JOB_HANDLE for errors
 **/


wJob_t wprintStartJob(const char            *printer_addr,
                      port_t                port_num,
                      wprint_job_params_t   *job_params,
                      printer_capabilities_t   *printer_cap,
                      char                  *mime_type,
                      char                  *pathname,
                      wprintSyncCb_t        cb_fn,
                      const char            *debugDir);

/**
 * wprintEndJob(): Sent once per job at the end of the job. A current print job
 * must end for the next one to start. Returns OK or ERROR as the case maybe. 
 **/

int    wprintEndJob(wJob_t  job_handle);


/**
 * wprintPage(): Sent once per page of a multi-page job to deliver a page 
 * image in a previously specified MIME type. The page_number must increment
 * from 1.  last_page flag is true, if it is the last page of the job.
 *
 * top/left/right/bottom margin are the incremental per page margins in pixels
 * at the current print resolution that are added on top of the physical page
 * page margins, passing in 0 results in the default page margins being used.
 *
 * Returns OK or ERROR as the case maybe. 
 **/

int    wprintPage(wJob_t   job_handle,
                  int      page_number,
                  char     *filename,
                  bool     last_page,
                  unsigned int top_margin,
                  unsigned int left_margin,
                  unsigned int right_margin,
                  unsigned int bottom_margin);

/**
 * wprintCancelJob(): cancel a spooled or running job 
 * Returns OK or ERROR
 **/

int wprintCancelJob(wJob_t   job_handle);


/**
 * wprintResumePrint(): resumes a suspended printer that is awaiting user action 
 * Returns OK or ERROR
 **/

int wprintResumePrint(wJob_t job_handle);

/**
 * wprintJobDesc(): queries the print subsystem for a particular job status
 **/
                   
int wprintJobDesc(wJob_t job_handle, wprint_job_desc_t  *job);

/**
 * wprintExit():  shuts down the print subsystem
 **/

int wprintExit(void);

char * getPCLTypeString(pcl_t pclenum);

void wprintGetTimeouts(long int *tcp_timeout_msec, long int *udp_timeout_msec, int *udp_retries);

void wprintSetTimeouts(long int tpc_timeout_msec, long int udp_timeout_msec, int udp_retries);

int wprintGetPrinterStatus(const char           *printer_addr,
                            int                  port_num,
                            printer_state_dyn_t *printer_status);

wStatus_t wprintStatusMonitorSetup(const char *printer_addr,
                                   int        port_num);

int wprintStatusMonitorStart(wStatus_t      status_handle,
                             void (*status_callback)(const printer_state_dyn_t *new_status,
                                                     const printer_state_dyn_t *old_status,
                                                     void *param), 
                             void           *param);

void wprintStatusMonitorStop(wStatus_t status_handle);

void wprintSetApplicationID(const char *id);
const char* wprintGetApplicationID();

void wprintSetOSImgIFC(const ifc_osimg_t *osimg_ifc);

#ifdef __cplusplus
}
#endif

#endif  /*  __LIB_WPRINT_H__  */

