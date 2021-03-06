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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>
#include <zlib.h>

#include "lib_pcl.h"
#include "ifc_print_job.h"
#include "wprint_image.h"
#include "wprint_jpeg.h"
#include "wprint_debug.h"

#include "wtypes.h"
#include "common_defines.h"
#include "pclm_wrapper_api.h"


#define  __INCLUDE_MEDIA_H__
#include "media2.h"

static const int _resolution[] = { 300, 600, 1200 };

static void _get_pclm_media_size_name(pcl_job_info_t *job_info, media_size_t media_size, char *media_name)
{
	int mediaNumEntries = sizeof(MasterMediaSizeTable2)
			/ sizeof(struct MediaSizeTableElement2);
	int i = 0;

	for (i = 0; i < mediaNumEntries; i++)
	{
		if (media_size == MasterMediaSizeTable2[i].media_size)
		{
		    strncpy(media_name, MasterMediaSizeTable2[i].PCL6Name,
            						strlen(MasterMediaSizeTable2[i].PCL6Name));
            job_info->wprint_ifc->debug(DBG_VERBOSE, "   _get_pclm_media_size_name(): match found: %d, %s\n",
            		                                    media_size, media_name);
			break;  // we found a match, so break out of loop
		}
	}

	if (i == mediaNumEntries)
	{
		// media size not found, defaulting to letter
		job_info->wprint_ifc->debug(DBG_VERBOSE, "   _get_pclm_media_size_name	(): media size, %d, NOT FOUND, setting to letter", media_size);
		_get_pclm_media_size_name(job_info, US_LETTER, media_name);
	}

}



static void _get_pclm_media_size(pcl_job_info_t *job_info, media_size_t media_size, PCLmPageSetup *myPageInfo)
{
	  int            mediaNumEntries=sizeof(MasterMediaSizeTable2)/sizeof(struct MediaSizeTableElement2);
	  int            i=0;

	  do
	  {
		  if(myPageInfo == NULL)
		  {
			  continue;
		  }

		  for(i=0; i<mediaNumEntries; i++)
		  {
			if (media_size == MasterMediaSizeTable2[i].media_size)
			{
				strncpy(myPageInfo->mediaSizeName,
						MasterMediaSizeTable2[i].PCL6Name,
						sizeof(myPageInfo->mediaSizeName) - 1);

				myPageInfo->mediaWidth          = floorf(_MI_TO_POINTS(MasterMediaSizeTable2[i].WidthInInches));
				myPageInfo->mediaHeight         = floorf(_MI_TO_POINTS(MasterMediaSizeTable2[i].HeightInInches));

				job_info->wprint_ifc->debug(DBG_VERBOSE, "   _get_pclm_media_size(): match found: %d, %s\n",
		                                    media_size, MasterMediaSizeTable2[i].PCL6Name);
		        job_info->wprint_ifc->debug(DBG_VERBOSE, "           Calculated mediaWidth=%f\n",
		        							myPageInfo->mediaWidth);
		        job_info->wprint_ifc->debug(DBG_VERBOSE, "           Calculated mediaHeight=%f\n",
		        							myPageInfo->mediaHeight);

				break;  // we found a match, so break out of loop
			}
		  }
	  }
	  while(0);

	  if (i == mediaNumEntries)
	  {
		  // media size not found, defaulting to letter
			job_info->wprint_ifc->debug(DBG_VERBOSE, "   _get_pclm_media_size(): media size, %d, NOT FOUND, setting to letter",
	                                    media_size);
		  _get_pclm_media_size(job_info, US_LETTER, myPageInfo);
	  }
}

/**
 * _start_job():  Sent once per job at the start of the job. Returns a print
 * job handle that is used in other functions of this library. 
 * Returns WPRINT_BAD_JOB_HANDLE for errors
 **/
static wJob_t _start_job ( wJob_t              job_handle,
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
                           float                left_margin )
{
	int outBuffSize = 0;

	if (job_info == NULL)
	{
	   return(_WJOBH_NONE);
	}

	if (job_info->job_handle != _WJOBH_NONE)
	{
	   if (job_info->wprint_ifc != NULL)
	   {
		   job_info->wprint_ifc->debug(DBG_ERROR, "lib_pclm: _start_job() required cleanup");
	   }

	   job_info->job_handle = _WJOBH_NONE;
	}

	if ((job_info->wprint_ifc == NULL) ||
	   (job_info->print_ifc == NULL))
	{
	   return(_WJOBH_NONE);
	}


	job_info->wprint_ifc->debug(DBG_LOG,
		   "lib_pclm: _start_job(), media_size %d, media_type %d, pq %d, dt %d, %s, media_tray %d margins T %f L %f",
		   media_size, media_type, print_quality, dry_time,
		   (duplex == DF_DUPLEX_MODE_NONE) ? "simplex" : "duplex",
		   media_tray, top_margin, left_margin);

	job_info->job_handle = job_handle;

    _START_JOB(job_info, "pdf");

	job_info->resolution = resolution;
	job_info->media_size = media_size;
	job_info->standard_scale = (float)resolution/(float)72;

	//  initialize static variables
	job_info->pclm_output_buffer = NULL;
	job_info->seed_row = job_info->pcl_buff = NULL;    // unused
	job_info->pixel_width = job_info->pixel_height = job_info->page_number = job_info->num_rows = 0;

	memset((void*)&job_info->pclm_page_info,0x0,sizeof(PCLmPageSetup));
	_get_pclm_media_size_name(job_info, media_size, &job_info->pclm_page_info.mediaSizeName[0]);

	if(left_margin < 0.0f || top_margin < 0.0f)
	{
		job_info->pclm_page_info.mediaWidthOffset=0.0f;
		job_info->pclm_page_info.mediaHeightOffset=0.0f;
	}
	else
	{
		job_info->pclm_page_info.mediaWidthOffset=(left_margin * (float)72);
		job_info->pclm_page_info.mediaHeightOffset=(top_margin * (float)72);
	}

	job_info->wprint_ifc->debug(DBG_LOG,"lib_pclm: _start_job(), mediaHeightOffsets W %f H %f",
	            job_info->pclm_page_info.mediaWidthOffset, job_info->pclm_page_info.mediaHeightOffset);

	job_info->pclm_page_info.pageOrigin=top_left;	// REVISIT

    job_info->monochrome = (color_space == DF_COLOR_SPACE_MONO);
	job_info->pclm_page_info.dstColorSpaceSpefication=deviceRGB;
	if(color_space==DF_COLOR_SPACE_MONO)
	{
		job_info->pclm_page_info.dstColorSpaceSpefication=grayScale;
	}
	else if(color_space==DF_COLOR_SPACE_COLOR)
	{
		job_info->pclm_page_info.dstColorSpaceSpefication=deviceRGB;
	}
	else if(color_space==DF_COLOR_SPACE_ADOBE_RGB)
	{
		job_info->pclm_page_info.dstColorSpaceSpefication=adobeRGB;
	}

	job_info->pclm_page_info.stripHeight=job_info->strip_height;

	job_info->pclm_page_info.destinationResolution=res600;
	if(resolution==300)
	{
		job_info->pclm_page_info.destinationResolution=res300;
	}
	else if(resolution==600)
	{
		job_info->pclm_page_info.destinationResolution=res600;
	}
	else if(resolution==1200)
	{
		job_info->pclm_page_info.destinationResolution=res1200;
	}

	if(duplex == DF_DUPLEX_MODE_BOOK)
	{
		job_info->pclm_page_info.duplexDisposition=duplex_longEdge;
	}
	else if(duplex == DF_DUPLEX_MODE_TABLET)
	{
		job_info->pclm_page_info.duplexDisposition=duplex_shortEdge;
	}
	else
	{
		job_info->pclm_page_info.duplexDisposition=simplex;
	}

	job_info->pclm_page_info.mirrorBackside=false;

	job_info->pclmgen_obj = (void*)CreatePCLmGen();
	PCLmStartJob(job_info->pclmgen_obj, (void**)&job_info->pclm_output_buffer, &outBuffSize, false);
	_WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);

	return(job_info->job_handle);
}

/**
 * _start_page(): Sent once per page of the job to indicate start of the
 * page and page metrics. Returns running page# starting with 1 or tsERROR.
 **/
static int _start_page ( pcl_job_info_t *job_info,
                         float          extra_margin,
                         int            pixel_width,
                         int            pixel_height)
{
    PCLmPageSetup  *page_info = &job_info->pclm_page_info;
    ubyte *pclm_output_buff = job_info->pclm_output_buffer;
	int outBuffSize = 0;
	char outstr[512];

    _START_PAGE(job_info, pixel_width, pixel_height);

	page_info->sourceHeight = (float)pixel_height/job_info->standard_scale;
	page_info->sourceWidth = (float)pixel_width/job_info->standard_scale;
	job_info->wprint_ifc->debug(DBG_LOG,
			"lib_pclm: _start_page(), strip height=%d, image width=%d, image height=%d, scaled width=%f, scaled height=%f",
				page_info->stripHeight,
				pixel_width,
				pixel_height,
				page_info->sourceWidth,
				page_info->sourceHeight);
    if(job_info->num_components==3)
    {
    	page_info->colorContent=color_content;
    	page_info->srcColorSpaceSpefication=deviceRGB;
    }
    else
    {
    	page_info->colorContent=gray_content;
    	page_info->srcColorSpaceSpefication=grayScale;
    }

    page_info->compTypeRequested=compressDCT;	// REVISIT: possibly get this value dynamically from device via IPP (ePCL)
    											//			however, current ink devices report RLE as the default compression
    											//			type, which compresses much worse than JPEG or FLATE
    job_info->scan_line_width = pixel_width*job_info->num_components;

	snprintf(outstr, sizeof(outstr), "%%============= Job Ticket =============\n");
	_WRITE(job_info, outstr, strlen(outstr));

    snprintf(outstr, sizeof(outstr), "%%PCLm (Ver: %f) JobTicket\n", PCLM_Ver);
    _WRITE(job_info, outstr, strlen(outstr));

    snprintf(outstr,sizeof(outstr), "%%   Media requested : %s  (%f x %f)\n",
    		page_info->mediaSizeName, page_info->sourceWidth, page_info->sourceHeight);
    _WRITE(job_info, outstr, strlen(outstr));

    snprintf(outstr,sizeof(outstr), "%%   currStripHeight=%d\n",  page_info->stripHeight);
    _WRITE(job_info, outstr, strlen(outstr));

    snprintf(outstr,sizeof(outstr), "%%   numStrips=%d\n",  (pixel_height + page_info->stripHeight - 1)/page_info->stripHeight);
    _WRITE(job_info, outstr, strlen(outstr));

    snprintf(outstr,sizeof(outstr), "%%   Output Resolution=%d\n", _resolution[page_info->destinationResolution]);
    _WRITE(job_info, outstr, strlen(outstr));

    snprintf(outstr,sizeof(outstr), "%%   Output Compression=%s\n",
            (page_info->compTypeRequested == compressFlate)?"Flate":"JPEG");
    _WRITE(job_info, outstr, strlen(outstr));


    page_info->SourceWidthPixels = pixel_width;
    page_info->SourceHeightPixels = pixel_height;


    int res1= PCLmGetMediaDimensions(job_info->pclmgen_obj, page_info->mediaSizeName, page_info);

    job_info->wprint_ifc->debug(DBG_LOG,
            			"lib_pclm: PCLmGetMediaDimensions(%d), mediaSizeName=%s, mediaheight=%f, mediawidth=%f, mheightpixels=%d, mwidthpixels=%d\n", res1,
            				job_info->pclm_page_info.mediaSizeName,
        					job_info->pclm_page_info.mediaWidth,
            				job_info->pclm_page_info.mediaHeight,
            				job_info->pclm_page_info.mediaWidthInPixels,
            				job_info->pclm_page_info.mediaHeightInPixels    );

    PCLmStartPage(job_info->pclmgen_obj, page_info, (void**)&pclm_output_buff, &outBuffSize);
	_WRITE(job_info, (const char *)pclm_output_buff, outBuffSize);

	job_info->page_number++;
    return(job_info->page_number);
}  /*  _start_page  */

/**
 * _print_swath(): Called several times a page to send a rectangular swath of 
 * RGB data. The array rgb_pixels[] must have (num_rows * pixel_width) pixels.
 * Returns OK or ERROR.
 **/
static int _print_swath ( pcl_job_info_t *job_info,
                          char           *rgb_pixels,
                          int            start_row,
                          int            num_rows,
                          int            bytes_per_row )
{
	int outBuffSize = 0;

    _PAGE_DATA(job_info, rgb_pixels, (num_rows * bytes_per_row));

    if (job_info->monochrome) {
        unsigned char *buff = (unsigned char *)rgb_pixels;
        int nbytes = (num_rows * bytes_per_row);
        int readIndex, writeIndex;
        for(readIndex = writeIndex = 0; readIndex < nbytes; readIndex += BYTES_PER_PIXEL(1)) {
            unsigned char gray = SP_GRAY(buff[readIndex + 0], buff[readIndex + 1], buff[readIndex + 2]);
            buff[writeIndex++] = gray;
            buff[writeIndex++] = gray;
            buff[writeIndex++] = gray;
        }
    }

	job_info->wprint_ifc->debug(DBG_VERBOSE,
       "lib_pclm: _print_swath(): page #%d, buffSize=%d, rows %d - %d (%d rows), bytes per row %d\n",
       	   	   	   	   job_info->page_number,
       	   	   	   	   job_info->strip_height*job_info->scan_line_width,
                       start_row,
                       start_row+num_rows-1,
                       num_rows,
                       bytes_per_row);
	// if the inBufferSize is ever used in genPCLm, change the input parameter to pass in image_info->printable_width*num_components*strip_height
	// it is currently pixel_width (from _start_page()) * num_components * strip_height
	PCLmEncapsulate(job_info->pclmgen_obj, rgb_pixels, job_info->strip_height*job_info->scan_line_width, num_rows, (void**)&job_info->pclm_output_buffer, &outBuffSize);
	_WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);

	return(OK);
}  /*  _print_swath  */

/**
 * _end_page(): Sent once per page of the job to indicate end of the page.
 * Returns OK or ERROR.
 **/
static int _end_page ( pcl_job_info_t *job_info,
                       int            page_number )
{
    int outBuffSize = 0;

    if (page_number == -1) {
        job_info->wprint_ifc->debug(DBG_LOG, "lib_pclm: _end_page(): writing blank page");
        _start_page(job_info, 0.0f/*extra_margin*/, 0/*pixel_width*/, 0/*pixel_height*/);
    }
    job_info->wprint_ifc->debug(DBG_LOG, "lib_pclm: _end_page()");
    PCLmEndPage(job_info->pclmgen_obj, (void**)&job_info->pclm_output_buffer, &outBuffSize);
    _WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);
    _END_PAGE(job_info);

    return(OK);
}  /*  _end_page  */

/**
 * _end_job(): Sent once per job at the end of the job. A current print job
 * must end for the next one to start. Returns OK or ERROR as the case maybe. 
 **/
static int  _end_job( pcl_job_info_t *job_info )
{
	int outBuffSize = 0;

	job_info->wprint_ifc->debug(DBG_LOG, "lib_pclm: _end_job()");
	PCLmEndJob(job_info->pclmgen_obj, (void**)&job_info->pclm_output_buffer, &outBuffSize);
	_WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);
	PCLmFreeBuffer(job_info->pclmgen_obj, job_info->pclm_output_buffer);
	DestroyPCLmGen(job_info->pclmgen_obj);

    _END_JOB(job_info);

	return(OK);
}  /*  _end_job  */

static bool _canCancelMidPage( void )
{
    return(false);
}

ifc_pcl_t  *pclm_connect(void)
{
  static const ifc_pcl_t  _pcl_ifc = 
  {
      _start_job,
      _end_job,
      _start_page,
      _end_page,
      _print_swath,
      _canCancelMidPage
  };

  return((ifc_pcl_t *) &_pcl_ifc);

}  /*  pclm_connect */

