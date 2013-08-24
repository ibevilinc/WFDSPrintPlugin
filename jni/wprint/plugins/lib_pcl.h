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
#ifndef __LIB_PCL_H__
#define __LIB_PCL_H__

#include "ifc_print_job.h"
#include "ifc_wprint.h"
#include "lib_wprint.h"
#include "lib_pclm.h"
#include "common_defines.h"

#define _WJOBH_NONE  0
#define _WJOBH_MIN   1
#define _WJOBH_MAX   65535

#define SP_GRAY(Yr,Cbg,Crb) (((Yr<<6) + (Cbg*160) + (Crb<<5)) >> 8)

#define _FREE(X)     { if (X) { free(X); (X) = NULL; } }

#define _START_JOB(JOB_INFO,EXT) \
{ \
    const ifc_wprint_debug_stream_t* debug_ifc = JOB_INFO->wprint_ifc->get_debug_stream_ifc(JOB_INFO->job_handle); \
    if (debug_ifc) { \
        debug_ifc->debug_start_job(JOB_INFO->job_handle, EXT); \
    } \
}

#define _START_PAGE(JOB_INFO,WIDTH,HEIGHT) \
{ \
    const ifc_wprint_debug_stream_t* debug_ifc = JOB_INFO->wprint_ifc->get_debug_stream_ifc(JOB_INFO->job_handle); \
    if (debug_ifc) { \
        debug_ifc->debug_start_page(JOB_INFO->job_handle, JOB_INFO->page_number + 1, WIDTH, HEIGHT); \
    } \
}

#define _PAGE_DATA(JOB_INFO,BUFF,LEN) \
{ \
    const ifc_wprint_debug_stream_t* debug_ifc = JOB_INFO->wprint_ifc->get_debug_stream_ifc(JOB_INFO->job_handle); \
    if (debug_ifc) { \
        debug_ifc->debug_page_data(JOB_INFO->job_handle, BUFF, LEN); \
    } \
}

#define _END_PAGE(JOB_INFO) \
{ \
    const ifc_wprint_debug_stream_t* debug_ifc = JOB_INFO->wprint_ifc->get_debug_stream_ifc(JOB_INFO->job_handle); \
    if (debug_ifc) { \
        debug_ifc->debug_end_page(JOB_INFO->job_handle); \
    } \
}

#define _END_JOB(JOB_INFO) \
{ \
    const ifc_wprint_debug_stream_t* debug_ifc = JOB_INFO->wprint_ifc->get_debug_stream_ifc(JOB_INFO->job_handle); \
    if (debug_ifc) { \
        debug_ifc->debug_end_job(JOB_INFO->job_handle); \
    } \
}

#define _WRITE(JOB_INFO,BUFF,LEN)   \
{ \
    const ifc_wprint_debug_stream_t* debug_ifc = JOB_INFO->wprint_ifc->get_debug_stream_ifc(JOB_INFO->job_handle); \
    if (debug_ifc) { \
        debug_ifc->debug_job_data(JOB_INFO->job_handle, BUFF, LEN); \
    } \
    JOB_INFO->print_ifc->send_data(JOB_INFO->print_ifc, BUFF, LEN); \
}

typedef struct
{
    const ifc_wprint_t    *wprint_ifc;
    const ifc_print_job_t *print_ifc;

    wJob_t  job_handle;
    uint8   *seed_row, *pcl_buff;
    uint8   *halftone_row;
    sint16  *error_buf;
    bool    mono1Bit;
    int     pixel_width, pixel_height;
    media_size_t  media_size;
    int     resolution;
    int     page_number, num_rows;
    int     send_full_row;
    int     rows_to_skip;
    uint8   monochrome;

    // 20120607 - LK - adding support for genPCLm
    int     num_components;
    int     scan_line_width;
    float   standard_scale;
    int     strip_height;

    void *pclmgen_obj;
    PCLmPageSetup  pclm_page_info;
    uint8 *pclm_output_buffer;
    const char *useragent;

} pcl_job_info_t;

typedef struct ifc_pcl_st
{

    /**
     * start_job():  Sent once per job at the start of the job. Returns a print
     * job handle that is used in other functions of this library. 
     * Returns WPRINT_BAD_JOB_HANDLE for errors
     **/


    wJob_t (*start_job) ( wJob_t              job_handlle,
                          pcl_job_info_t      *job_info,
                          media_size_t         media_size, 
                          DF_media_type_t      media_type, 
                          DF_quality_t         print_quality, 
                          int                  resolution,
                          DF_duplex_t          duplex, 
                          duplex_dry_time_t    dry_time,
                          DF_color_space_t     color_space,
                          uint8                mono1Bit,
		          PT_media_src_t       media_tray,
		          float                top_margin,
		          float                left_margin );

    /**
     * end_job(): Sent once per job at the end of the job. A current print job
     * must end for the next one to start. Returns OK or ERROR as the case maybe. 
     **/

    int    (*end_job) ( pcl_job_info_t  *job_info );
    
    /**
     * start_page(): Sent once per page of the job to indicate start of the
     * page and page metrics. 
     * Returns running page# starting with 1 or ERROR.
     **/

    int  (*start_page) ( pcl_job_info_t      *job_info,
                         float               extra_margin,
                         int                 pixel_width,
                         int                 pixel_height);

    /** 
     * end_page(): Sent once per page of the job to indicate end of the page.
     * Returns OK or ERROR.
     **/

    int (*end_page) ( pcl_job_info_t     *job_info,
                      int                page_number );
    
    /**
     * print_swath(): Called several times a page to send a rectangular swath of 
     * RGB data. The array rgb_pixels[] must have (num_rows * pixel_width) pixels.
     * bytes_per_row can be used for 32-bit aligned rows.
     * Returns OK or ERROR.
     **/

    int (*print_swath) ( pcl_job_info_t     *job_info,
                         char               *rgb_pixels,
                         int                start_row,
                         int                num_rows,
                         int                bytes_per_row );

    bool (*canCancelMidPage) ( void );

}  ifc_pcl_t;


ifc_pcl_t  *pclm_connect(void);
ifc_pcl_t  *pwg_connect(void);


#endif  /*  __LIB_PCL_H__  */

