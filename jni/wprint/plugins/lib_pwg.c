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
#include <cups/raster.h>

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

#define STANDARD_SCALE_FOR_PDF    72.0
#define _MI_TO_PIXELS(n,res)      ((n)*(res)+500)/1000.0
#define _MI_TO_POINTS(n)          _MI_TO_PIXELS(n, STANDARD_SCALE_FOR_PDF)

static const int _resolution[] = { 300, 600, 1200 };


cups_raster_t	*ras_out = NULL;
cups_page_header2_t	header_pwg;		/* Page header */


static void _write_header_pwg(int pixel_width, int pixel_height, cups_page_header2_t *h ) {

    if (h != NULL) {
    	strcpy(h->MediaClass, "PwgRaster");
		strcpy(h->MediaColor, "");
		strcpy(h->MediaType, "");
		strcpy(h->OutputType, "");
		h->AdvanceDistance = 0;
		h->AdvanceMedia = CUPS_ADVANCE_FILE;
		h->Collate = CUPS_FALSE;
		h->CutMedia = CUPS_CUT_NONE;
		//h->Duplex = CUPS_FALSE;
		//h->HWResolution[0] = j_info->resolution;
		//h->HWResolution[1] = j_info->resolution;

		h->cupsPageSize[0] = (float) ( ( pixel_width / h->HWResolution[0]) * 72);
		h->cupsPageSize[1] =  (float) (( pixel_height  / h->HWResolution[1]) * 72);

		h->ImagingBoundingBox[0] = 0;
		h->ImagingBoundingBox[1] = 0;
		h->ImagingBoundingBox[2] = h->cupsPageSize[0];
		h->ImagingBoundingBox[3] = h->cupsPageSize[1];
		h->cupsBorderlessScalingFactor = 1.0;
		h->InsertSheet = CUPS_FALSE;
		h->Jog = CUPS_JOG_NONE;
		h->LeadingEdge = CUPS_EDGE_TOP;
		h->Margins[0] = 0;
		h->Margins[1] = 0;
		h->ManualFeed =CUPS_TRUE;
		h->MediaPosition = 0;
		h->MediaWeight = 0;
		h->MirrorPrint = CUPS_FALSE;
		h->NegativePrint = CUPS_FALSE;
		h->NumCopies = 1;
		h->Orientation = CUPS_ORIENT_0;
		//h->OutputFaceUp = CUPS_FALSE;
		h->PageSize[0] = (int)h->cupsPageSize[0];
		h->PageSize[1] = (int)h->cupsPageSize[1];
		h->Separations = CUPS_TRUE;
		h->TraySwitch = CUPS_TRUE;
		h->Tumble = CUPS_TRUE;
		h->cupsWidth = (int) pixel_width ;
		h->cupsHeight =(int) pixel_height ;
		//h->cupsMediaType = j_info->media_size;
		//h->cupsBitsPerColor = 8; //BITS_PER_CHANNEL
		if(header_pwg.cupsColorSpace == CUPS_CSPACE_RGB)
			h->cupsBitsPerPixel = 3 * h->cupsBitsPerColor; //24 BITS_PER_PIXEL
		else
			h->cupsBitsPerPixel = h->cupsBitsPerColor; //8 BITS_PER_PIXEL
		h->cupsBytesPerLine = (h->cupsBitsPerPixel * pixel_width + 7 ) / 8;
		h->cupsColorOrder = CUPS_ORDER_CHUNKED;
		h->cupsCompression = 0;
		h->cupsRowCount = 1;
		h->cupsRowFeed = 1;
		h->cupsRowStep = 1;
		h->cupsNumColors = 0;
		h->cupsImagingBBox[0] = 0.0;
		h->cupsImagingBBox[1] = 0.0;
		h->cupsImagingBBox[2] = 0.0;
		h->cupsImagingBBox[3] = 0.0;
		int j;
		for( j = 0; j<16; j++)
		{
			h->cupsInteger[j] = (int)1+j;
			h->cupsReal[j] = (float)j+1.1;
			snprintf(h->cupsString[j], 20, "String # %d", j+1);
		}

		strcpy(h->cupsMarkerType, "Marker Type");
		strcpy(h->cupsRenderingIntent, "Rendering Intent");
		strcpy(h->cupsPageSizeName, "Letter");


    }

}


static void _get_pwg_media_size(pcl_job_info_t *job_info, media_size_t media_size, PCLmPageSetup *myPageInfo)
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

				job_info->wprint_ifc->debug(DBG_VERBOSE, "   _get_pwg_media_size(): match found: %d, %s\n",
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
			job_info->wprint_ifc->debug(DBG_VERBOSE, "   _get_pwg_media_size(): media size, %d, NOT FOUND, setting to letter",
	                                    media_size);
		  _get_pwg_media_size(job_info, US_LETTER, myPageInfo);
	  }
}

static ssize_t				/* O - Bytes written or -1 */
_pwg_io_write(void          *ctx,	/* I - File descriptor pointer */
              unsigned char *buf,	/* I - Bytes to write */
	          size_t        bytes)	/* I - Number of bytes to write */
{
	pcl_job_info_t      *pwg_job_info = (pcl_job_info_t *) ctx;
	_WRITE(pwg_job_info, buf, bytes);
	return bytes;
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
		   job_info->wprint_ifc->debug(DBG_ERROR, "lib_pwg: _start_job() required cleanup");
	   }

	   job_info->job_handle = _WJOBH_NONE;
	}

	if ((job_info->wprint_ifc == NULL) ||
	   (job_info->print_ifc == NULL))
	{
	   return(_WJOBH_NONE);
	}


	job_info->wprint_ifc->debug(DBG_LOG,
		   "lib_pwg: _start_job(), media_size %d, media_type %d, pq %d, dt %d, %s, media_tray %d",
		   media_size, media_type, print_quality, dry_time,
		   (duplex == DF_DUPLEX_MODE_NONE) ? "simplex" : "duplex",
		   media_tray);

	job_info->job_handle = job_handle;

    _START_JOB(job_info, "pwg");

    header_pwg.HWResolution[0] = resolution;
    header_pwg.HWResolution[1] = resolution;

	job_info->resolution = resolution;
	job_info->media_size = media_size;
	job_info->standard_scale = (float)resolution/(float)72;

	//  initialize static variables
	job_info->pclm_output_buffer = NULL;
	job_info->seed_row = job_info->pcl_buff = NULL;    // unused
	job_info->pixel_width = job_info->pixel_height = job_info->page_number = job_info->num_rows = 0;

	memset((void*)&job_info->pclm_page_info,0x0,sizeof(PCLmPageSetup));
	_get_pwg_media_size(job_info, media_size, &job_info->pclm_page_info);

	if(left_margin < 0.0f || top_margin < 0.0f)
	{
		job_info->pclm_page_info.mediaWidthOffset=0.0f;
		job_info->pclm_page_info.mediaHeightOffset=0.0f;
	}
	else
	{
		job_info->pclm_page_info.mediaWidthOffset=left_margin;
		job_info->pclm_page_info.mediaHeightOffset=top_margin;
	}

	header_pwg.cupsMediaType = media_size;

	job_info->pclm_page_info.pageOrigin=top_left;	// REVISIT

	job_info->pclm_page_info.dstColorSpaceSpefication=deviceRGB;
	if(color_space==DF_COLOR_SPACE_MONO)
	{
		header_pwg.cupsColorSpace = CUPS_CSPACE_RGB;
		//job_info->pclm_page_info.dstColorSpaceSpefication=grayScale;
		job_info->pclm_page_info.dstColorSpaceSpefication=deviceRGB;
	}
	else if(color_space==DF_COLOR_SPACE_COLOR)
	{
		job_info->pclm_page_info.dstColorSpaceSpefication=deviceRGB;
		header_pwg.cupsColorSpace = CUPS_CSPACE_RGB;
	}
	else if(color_space==DF_COLOR_SPACE_ADOBE_RGB)
	{
		job_info->pclm_page_info.dstColorSpaceSpefication=adobeRGB;
		header_pwg.cupsColorSpace = CUPS_CSPACE_RGB;
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
		header_pwg.Duplex = CUPS_TRUE;
	}
	else if(duplex == DF_DUPLEX_MODE_TABLET)
	{
		job_info->pclm_page_info.duplexDisposition=duplex_shortEdge;
		header_pwg.Duplex = CUPS_TRUE;
	}
	else
	{
		job_info->pclm_page_info.duplexDisposition=simplex;
		header_pwg.Duplex = CUPS_FALSE;
	}

	job_info->pclm_page_info.mirrorBackside=false;
	header_pwg.OutputFaceUp = CUPS_FALSE;
	header_pwg.cupsBitsPerColor = BITS_PER_CHANNEL;

	//job_info->pclmgen_obj = (void*)CreatePCLmGen();
	//PCLmStartJob(job_info->pclmgen_obj, (void**)&job_info->pclm_output_buffer, &outBuffSize, false);
	ras_out = cupsRasterOpenIO(_pwg_io_write, (void *) job_info, CUPS_RASTER_WRITE_PWG);
	//_WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);

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
    //ubyte *pclm_output_buff = job_info->pclm_output_buffer;
	//int outBuffSize = 0;
	//char outstr[512];


    _START_PAGE(job_info, pixel_width, pixel_height);

	page_info->sourceHeight = (float)pixel_height/job_info->standard_scale;
	page_info->sourceWidth = (float)pixel_width/job_info->standard_scale;
	job_info->wprint_ifc->debug(DBG_LOG,
			"lib_pwg: _start_page(), strip height=%d, image width=%d, image height=%d, scaled width=%f, scaled height=%f",
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
	page_info->colorContent=color_content;
	page_info->srcColorSpaceSpefication=deviceRGB;

    page_info->compTypeRequested=compressDCT;	// REVISIT: possibly get this value dynamically from device via IPP (ePCL)
    											//			however, current ink devices report RLE as the default compression
    											//			type, which compresses much worse than JPEG or FLATE
    job_info->scan_line_width = BYTES_PER_PIXEL(pixel_width);

	//Fill up the pwg header
	_write_header_pwg(pixel_width, pixel_height, &header_pwg);

	job_info->wprint_ifc->debug(DBG_LOG, "cupsWidth = %d", header_pwg.cupsWidth);
	job_info->wprint_ifc->debug(DBG_LOG, "cupsHeight = %d", header_pwg.cupsHeight);
	job_info->wprint_ifc->debug(DBG_LOG, "cupsBitsPerColor = %d", header_pwg.cupsBitsPerColor);
	job_info->wprint_ifc->debug(DBG_LOG, "cupsBitsPerPixel = %d", header_pwg.cupsBitsPerPixel);
	job_info->wprint_ifc->debug(DBG_LOG, "cupsBytesPerLine = %d", header_pwg.cupsBytesPerLine);
	job_info->wprint_ifc->debug(DBG_LOG, "cupsColorOrder = %d", header_pwg.cupsColorOrder);
	job_info->wprint_ifc->debug(DBG_LOG, "cupsColorSpace = %d", header_pwg.cupsColorSpace);


	cupsRasterWriteHeader2(ras_out, &header_pwg);

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
	int outBuffSize = job_info->strip_height*job_info->scan_line_width;

    _PAGE_DATA(job_info, rgb_pixels, (num_rows * bytes_per_row));

	job_info->wprint_ifc->debug(DBG_VERBOSE,
       "lib_pwg: _print_swath(): page #%d, buffSize=%d, rows %d - %d (%d rows), bytes per row %d\n",
       	   	   	   	   job_info->page_number,
       	   	   	   	   job_info->strip_height*job_info->scan_line_width,
                       start_row,
                       start_row+num_rows-1,
                       num_rows,
                       bytes_per_row);
	// if the inBufferSize is ever used in genPCLm, change the input parameter to pass in image_info->printable_width*num_components*strip_height
	// it is currently pixel_width (from _start_page()) * num_components * strip_height
	//PCLmEncapsulate(job_info->pclmgen_obj, rgb_pixels, job_info->strip_height*job_info->scan_line_width, num_rows, (void**)&job_info->pclm_output_buffer, &outBuffSize);
	//_WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);
	if(ras_out != NULL)
	{

		unsigned result = cupsRasterWritePixels(ras_out, rgb_pixels, outBuffSize);
		job_info->wprint_ifc->debug(DBG_VERBOSE,
		       "lib_pwg:  cupsRasterWritePixels return %d\n", result);
	}
	else
		job_info->wprint_ifc->debug(DBG_VERBOSE,   "lib_pwg:  cupsRasterWritePixels raster is null ###############################333 \n");

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

	job_info->wprint_ifc->debug(DBG_LOG, "lib_pcwg: _end_page()");
	//PCLmEndPage(job_info->pclmgen_obj, (void**)&job_info->pclm_output_buffer, &outBuffSize);
	//_WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);

    _END_PAGE(job_info);

	return(OK);
}  /*  _end_page  */

/**
 * _end_job(): Sent once per job at the end of the job. A current print job
 * must end for the next one to start. Returns OK or ERROR as the case maybe. 
 **/
static int  _end_job( pcl_job_info_t *job_info )
{
	//int outBuffSize = 0;

	job_info->wprint_ifc->debug(DBG_LOG, "lib_pwg: _end_job()");
	//PCLmEndJob(job_info->pclmgen_obj, (void**)&job_info->pclm_output_buffer, &outBuffSize);
	//_WRITE(job_info, (const char *)job_info->pclm_output_buffer, outBuffSize);
	PCLmFreeBuffer(job_info->pclmgen_obj, job_info->pclm_output_buffer);
	//DestroyPCLmGen(job_info->pclmgen_obj);

    _END_JOB(job_info);
	cupsRasterClose(ras_out);

	return(OK);
}  /*  _end_job  */

static bool _canCancelMidPage( void )
{
    return(false);
}

ifc_pcl_t  *pwg_connect(void)
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

}  /*  pwg_connect */

