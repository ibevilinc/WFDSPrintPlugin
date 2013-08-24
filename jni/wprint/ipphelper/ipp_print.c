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
/* Include the CUPS header files. */
//#include <cups/cups.h>
#include "ipp_print.h"    // you need to include this before http-private.h or wprint.h throws an error.
#include "config.h"
#include "cups.h"
#include <stdio.h>
#include "http-private.h"
#include <fcntl.h>
#include "http.h"
#include <math.h>
#include "ipp.h"
#include "ipphelper.h"
#include "ippstatus.h"

#include "ifc_print_job.h"
#include <string.h>
#include "wtypes.h"
#include "wprint_df_types.h"
#include "wprint_debug.h"

#define  __INCLUDE_MEDIA_H__
#include "../../wprint/plugins/media2.h"


#define HTTP_RESOURCE "/ipp/printer"

#undef ifprint
#define ifprint(statements) ipp_wprint_ifc->debug statements

static int _init(const ifc_print_job_t *this_p,
                  const char *printer_address);
static int _validate_job(const ifc_print_job_t *this_p,
                         wprint_job_params_t *job_params);
static int _start_job(const ifc_print_job_t *this_p,
                      const wprint_job_params_t *job_params);
static int _send_data(const ifc_print_job_t *this_p,
                      const char *buffer,
                      size_t length);
static int _end_job(const ifc_print_job_t *this_p);
static void _destroy(const ifc_print_job_t *this_p);

static const ifc_print_job_t _print_job_ifc =
{
    .init         = _init,
    .validate_job = _validate_job,
    .start_job    = _start_job,
    .send_data    = _send_data,
    .end_job      = _end_job,
    .destroy      = _destroy,
    .enable_timeout = NULL,
};

typedef struct
{
    http_t *http;
    char   printer_uri[1024];
    http_status_t status;
    ifc_print_job_t ifc;
    
} ipp_print_job_t;


const ifc_print_job_t *ipp_get_print_ifc(const ifc_wprint_t *wprint_ifc)
{
	ifprint((DBG_VERBOSE, "ipp_get_print_ifc: Enter"));
    ipp_wprint_ifc = wprint_ifc;
    ipp_print_job_t *ipp_job = (ipp_print_job_t*)malloc(sizeof(ipp_print_job_t));

    if (ipp_job == NULL)
        return(NULL);

    memset(ipp_job, 0, sizeof(ipp_print_job_t));

    ipp_job->http   = NULL;
    ipp_job->status = HTTP_CONTINUE;

    memcpy(&ipp_job->ifc, &_print_job_ifc, sizeof(ifc_print_job_t));

    /* remove this line when job validation is possible */
    //   ipp_job->ifc.validate_job = NULL;

    return(&ipp_job->ifc);
}

static int _init(const ifc_print_job_t *this_p,
                  const char *printer_address)
{
	ifprint((DBG_VERBOSE, "_init: Enter"));
    ipp_print_job_t *ipp_job;
    do
    {
        if (this_p == NULL)
            continue;

        ipp_job = IMPL(ipp_print_job_t, ifc, this_p);
        if (ipp_job->http != NULL)
            httpClose(ipp_job->http);

        httpAssembleURIf(HTTP_URI_CODING_ALL, ipp_job->printer_uri, sizeof(ipp_job->printer_uri), "ipp", NULL, printer_address, ippPort(), "/ipp/printer");
        ipp_job->http = httpConnectEncrypt(printer_address, ippPort(), cupsEncryption());
        httpSetTimeout(ipp_job->http, DEFAULT_IPP_TIMEOUT, NULL, 0);

    } while(0);

    return(0);
}

static void _destroy(const ifc_print_job_t *this_p)
{
    ifprint((DBG_VERBOSE, "_destroy: Enter"));
    ipp_print_job_t *ipp_job;
    do
    {
        if (this_p == NULL)
            continue;

        ipp_job = IMPL(ipp_print_job_t, ifc, this_p);
        if (ipp_job->http != NULL)
            httpClose(ipp_job->http);

        free(ipp_job);
    } while(0);
}

static void _get_pclm_media_size(media_size_t media_size,
		float *mediaWidth, float *mediaHeight)
{
	int mediaNumEntries = sizeof(MasterMediaSizeTable2)
			/ sizeof(struct MediaSizeTableElement2);
	int i = 0;

	for (i = 0; i < mediaNumEntries; i++)
	{
		if (media_size == MasterMediaSizeTable2[i].media_size)
		{

			*mediaWidth = floorf((MasterMediaSizeTable2[i].WidthInInches * 2540.0f) / 1000.0f);
			*mediaHeight = floorf(MasterMediaSizeTable2[i].HeightInInches * 2540.0f / 1000.0f);

			ifprint(
					(DBG_VERBOSE, "   _get_pclm_media_size(): match found: %d, %s\n", media_size, MasterMediaSizeTable2[i].PCL6Name));
			ifprint(
					(DBG_VERBOSE, "           Calculated mediaWidth=%f\n", *mediaWidth));
			ifprint(
					(DBG_VERBOSE, "           Calculated mediaHeight=%f\n", *mediaHeight));

			break;  // we found a match, so break out of loop
		}
	}

	if (i == mediaNumEntries)
	{
		// media size not found, defaulting to letter
		ifprint(
				(DBG_VERBOSE, "   _get_pclm_media_size(): media size, %d, NOT FOUND, setting to letter", media_size));
		_get_pclm_media_size(US_LETTER, mediaWidth, mediaHeight);
	}
}




static ipp_t* _fill_job(int ipp_op,
                        char *printer_uri,
                        const wprint_job_params_t *job_params)
{
	ifprint((DBG_VERBOSE, "_fill_job: Enter"));
    ipp_t           *request = NULL;               /* IPP request object */
    ipp_attribute_t *attrptr;         /* Attribute pointer */
    ipp_t *col[2];
    int col_index = -1;

    if (job_params == NULL) return NULL;

    request = ippNewRequest(ipp_op);
    if (request == NULL)
        return(request);

    if(set_ipp_version(request, printer_uri, NULL, ipp_version_has_been_resolved) !=0)
    {
    	// 20120302 - LK - added ippDelete(request) to clean up allocated request before
    	// returning (resource leak reported by Coverity)
        ippDelete(request);
    	return NULL;
    }
    bool is_2_0_capable = job_params->ipp_2_0_supported;
    bool is_ePCL_ipp_capable = job_params->ePCL_ipp_supported;

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsUser());

    ifprint((DBG_VERBOSE, "_fill_job: pcl_type(%d), print_format(%s)", job_params->pcl_type, job_params->print_format));

// ifdef'in out PDF code since PDF pages are now rendered to a print-ready format (PCL3, PCLm, etc.) within this library
    bool is_PDF_job = strcmp(job_params->print_format, PRINT_FORMAT_PDF) == 0;
    //document-format Needs to be the next attribute because some ink devices puke if not
    if (is_PDF_job)
	{
		if (is_2_0_capable)
		{
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_MIMETYPE,
							"document-format", NULL, PRINT_FORMAT_PDF);
			ifprint((DBG_VERBOSE, "_fill_job: setting document-format: %s", PRINT_FORMAT_PDF));
		}
		else
		{
			//we need to do this because VEP devices don't print pdfs when we send the other PRINT_FORMAT_PDF
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_MIMETYPE,
					"document-format", NULL, (job_params->acceptsPCLm ? PRINT_FORMAT_PCLM : PRINT_FORMAT_PDF));
			ifprint((DBG_VERBOSE, "_fill_job: setting document-format: %s", PRINT_FORMAT_PCLM));
		}
		//only add copies info to the job if it is a pdf as copies are included in rendered jobs
		ippAddInteger(request, IPP_TAG_JOB, IPP_TAG_INTEGER, "copies", job_params->num_copies);

		if (is_ePCL_ipp_capable)
		{
			if (job_params->render_flags & RENDER_FLAG_AUTO_SCALE)
			{
				ippAddBoolean(request, IPP_TAG_JOB, "pdf-fit-to-page", 1); //true
			}
		}
	}
    else
    {

		switch (job_params->pcl_type)
		{
		case PCLm:
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_MIMETYPE,
					"document-format", NULL, (job_params->acceptsPCLm ? PRINT_FORMAT_PCLM : PRINT_FORMAT_PDF));
			ifprint((DBG_VERBOSE, "_fill_job: setting document-format: %s", PRINT_FORMAT_PCLM));
			if (is_ePCL_ipp_capable)
			{
				ippAddResolution(request, IPP_TAG_JOB, "pclm-source-resolution",
						IPP_RES_PER_INCH, job_params->pixel_units,
						job_params->pixel_units);
			}
			break;
		case PCLPWG:
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_MIMETYPE,
					"document-format", NULL, PRINT_FORMAT_PWG);
			ifprint((DBG_VERBOSE, "_fill_job: setting document-format: %s", PRINT_FORMAT_PWG));
			/*if (is_ePCL_ipp_capable)
			{
				ippAddResolution(request, IPP_TAG_JOB, "pclm-source-resolution",
						IPP_RES_PER_INCH, job_params->pixel_units,
						job_params->pixel_units);
			}*/
			break;
		case PCLNONE:
		default:
			ifprint((DBG_VERBOSE, "_fill_job: setting document-format: %s", PRINT_FORMAT_AUTO));
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_MIMETYPE,
					"document-format", NULL, PRINT_FORMAT_AUTO);
			break;
		}

		if(is_ePCL_ipp_capable)
		{
			ippAddBoolean(request, IPP_TAG_JOB, "margins-pre-applied", 1); //true
		}
    }

    ippAddInteger(request, IPP_TAG_JOB, IPP_TAG_ENUM, "print-quality", ippMapPQ(job_params->print_quality));
    ippAddResolution(request, IPP_TAG_JOB, "printer-resolution", IPP_RES_PER_INCH,job_params->pixel_units,job_params->pixel_units);
    if (job_params->duplex == DF_DUPLEX_MODE_BOOK)
		ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, IPP_SIDES_TAG, NULL, IPP_SIDES_TWO_SIDED_LONG_EDGE);
	else if (job_params->duplex == DF_DUPLEX_MODE_TABLET)
		ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, IPP_SIDES_TAG, NULL, IPP_SIDES_TWO_SIDED_SHORT_EDGE);
	else
		ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, IPP_SIDES_TAG, NULL, IPP_SIDES_ONE_SIDED);
    if (job_params->color_space == DF_COLOR_SPACE_MONO)
        ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, IPP_OUTPUT_MODE_TAG, NULL, IPP_OUTPUT_MODE_MONO);
    else
        ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, IPP_OUTPUT_MODE_TAG, NULL, IPP_OUTPUT_MODE_COLOR);

    if(is_2_0_capable)
    {
    	//TODO: what is media-source?
		//not friendly to 1.1 devices.
		if (job_params->media_tray != PT_SRC_AUTO_SELECT)
		{
			if (job_params->media_tray == PT_SOURCE_PHOTO_TRAY)
			{
				col[++col_index] = ippNew();
				ippAddString(col[col_index], IPP_TAG_JOB, IPP_TAG_KEYWORD, "media-source", NULL, "main-tray");
			}
			else if (job_params->media_tray == PT_SOURCE_TRAY_1)
			{
				col[++col_index] = ippNew();
				ippAddString(col[col_index], IPP_TAG_JOB, IPP_TAG_KEYWORD, "media-source", NULL, "main-tray");
			}
		}

		//MEDIA-Type is IPP 2.0 only
		//put margins in with media-type
		//Client-Error-Attribute-Or-Values-Not-Supported
		col[++col_index] = ippNew();
		// set margins - if negative margins, set to full-bleed (zeros); otherwise set calculated values
		if (job_params->borderless) {
			ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-bottom-margin", 0);
			ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-top-margin", 0);
			ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-left-margin", 0);
			ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-right-margin", 0);

		}
		else
		{
			//ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-bottom-margin", 0);
			//ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-top-margin", 0);
			//ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-left-margin", 0);
			//ippAddInteger(col[col_index], IPP_TAG_JOB, IPP_TAG_INTEGER, "media-right-margin", 0);
		}

		//ippAddString(col[col_index], IPP_TAG_JOB, IPP_TAG_KEYWORD, "media-source", NULL, "main");

		switch(job_params->media_type)
		{
			case DF_MEDIA_ADVANCED_PHOTO:
			case DF_MEDIA_PHOTO_GLOSSY:
			case DF_MEDIA_PHOTO_MATTE:
			case DF_MEDIA_PREMIUM_PHOTO:
			case DF_MEDIA_OTHER_PHOTO:
				ippAddString(col[col_index], IPP_TAG_JOB, IPP_TAG_KEYWORD, "media-type", NULL, "photographic-glossy");
				break;
			default:
				ippAddString(col[col_index], IPP_TAG_JOB, IPP_TAG_KEYWORD, "media-type", NULL, "stationery");
				break;
		}
		float mediaWidth;
		float mediaHeight;
		_get_pclm_media_size(job_params->media_size, &mediaWidth, &mediaHeight);


		ipp_t *mediaSize = ippNew();
		ippAddInteger(mediaSize, IPP_TAG_JOB, IPP_TAG_INTEGER, "x-dimension", mediaWidth);
		ippAddInteger(mediaSize, IPP_TAG_JOB, IPP_TAG_INTEGER, "y-dimension", mediaHeight);
		ippAddCollection(col[col_index], IPP_TAG_JOB, "media-size", mediaSize);

		ippAddCollections(request, IPP_TAG_JOB, "media-col", col_index + 1,
        				(const ipp_t**) col);
        		while (col_index >= 0)
        			ippDelete(col[col_index--]);

            }else{
            	ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, "media", NULL, mapDFMediaToIPPKeyword(job_params->media_size));

    }

    ifprint((DBG_LOG, "_fill_job (%d): request  \n", ipp_op));
    for (attrptr = ippFirstAttribute(request); attrptr; attrptr = ippNextAttribute(request))
        print_attr(attrptr);

    return(request);
}

static int _validate_job(const ifc_print_job_t *this_p,
                         wprint_job_params_t *job_params)
{
	ifprint((DBG_VERBOSE, "_validate_job: Enter"));
    int result = -1;
    int service_unavailable_retry_count = 0;
    int bad_request_retry_count = 0;
    ipp_print_job_t *ipp_job;
    ipp_t           *response; /* IPP response object */
    ipp_t           *request = NULL;               /* IPP request object */
    ipp_status_t    ipp_status = IPP_OK; /* Status of IPP request */
 
    ifprint((DBG_VERBOSE, "_validate_job: ** validatePrintJob:  Entry"));
    do
	{
    	ipp_status = IPP_OK; //reset ipp_status
		if (this_p == NULL)
			break;

		if (job_params == NULL)
			break;

		ipp_job = IMPL(ipp_print_job_t, ifc, this_p);
		if (ipp_job->http == NULL)
			break;

		request = _fill_job(IPP_VALIDATE_JOB, ipp_job->printer_uri, job_params);

		if ((response = cupsDoRequest(ipp_job->http, request, HTTP_RESOURCE)) == NULL)
		{
			ipp_status = cupsLastError();
			ifprint((DBG_ERROR, "_validate_job:  validatePrintJob:  response is null:  ipp_status %d %s",ipp_status, ippErrorString(ipp_status) ));
			if ((ipp_status == IPP_SERVICE_UNAVAILABLE) && (service_unavailable_retry_count < IPP_SERVICE_UNAVAILBLE_MAX_RETRIES))
			{
				ifprint((DBG_ERROR, "  test_and_set_ipp_version:  1282 received, retrying %d of %d", service_unavailable_retry_count, IPP_SERVICE_UNAVAILBLE_MAX_RETRIES));
				service_unavailable_retry_count++;
				continue;
			}
			if (ipp_status == IPP_BAD_REQUEST)
			{
				ifprint((DBG_ERROR, "IPP_Status of IPP_BAD_REQUEST received. retry (%d) of (%d)", bad_request_retry_count, IPP_BAD_REQUEST_MAX_RETRIES));
				if(bad_request_retry_count>IPP_BAD_REQUEST_MAX_RETRIES)
					break;
				bad_request_retry_count++;
				continue;
			}
		}
		else
		{
			ipp_status = cupsLastError();
			if (ipp_status == IPP_BAD_REQUEST)
			{
				ifprint((DBG_ERROR, "IPP_Status of IPP_BAD_REQUEST received. retry (%d) of (%d)", bad_request_retry_count, IPP_BAD_REQUEST_MAX_RETRIES));
				if(bad_request_retry_count>IPP_BAD_REQUEST_MAX_RETRIES)
					break;
				bad_request_retry_count++;
				ippDelete(response);
				continue;
			}
			ifprint((DBG_VERBOSE, "_validate_job: %s ipp_status %d  %x received:", ippOpString(IPP_VALIDATE_JOB), ipp_status, ipp_status));
			ipp_attribute_t *attrptr; /* Attribute pointer */
			for (attrptr = ippFirstAttribute(response); attrptr; attrptr = ippNextAttribute(response)) 
				print_attr(attrptr);
			ifprint((DBG_VERBOSE, ""));

			ippDelete(response);
#if 0
			ifprint((DBG_VERBOSE, "    ippStatus Codes: IPP_OK_SUBST %x IPP_OK_CONFLICT %x IPP_BAD_REQUEST %x IPP_SERVICE_UNAVAILABLE %x IPP_OPERATION_NOT_SUPPORTED %x", IPP_OK_SUBST, IPP_OK_CONFLICT, IPP_BAD_REQUEST, IPP_SERVICE_UNAVAILABLE, IPP_OPERATION_NOT_SUPPORTED ));
			ifprint((DBG_VERBOSE, "    ippStatus Codes: IPP_PRINTER_BUSY %x  IPP_VERSION_NOT_SUPPORTED %x  IPP_NOT_AUTHORIZED %x",IPP_PRINTER_BUSY, IPP_VERSION_NOT_SUPPORTED, IPP_NOT_AUTHORIZED));
			ifprint((DBG_VERBOSE, "    ippStatus Codes: IPP_DOCUMENT_FORMAT_ERROR %x IPP_FORBIDDEN %x   IPP_NOT_FOUND %x",IPP_DOCUMENT_FORMAT_ERROR, IPP_FORBIDDEN, IPP_NOT_FOUND));
			ifprint((DBG_VERBOSE, "    ippStatus Codes: IPP_DOCUMENT_FORMAT %x IPP_ATTRIBUTES_NOT_SETTABLE %x   IPP_ATTRIBUTES %x IPP_INTERNAL_ERROR %x",IPP_DOCUMENT_FORMAT, IPP_ATTRIBUTES_NOT_SETTABLE, IPP_ATTRIBUTES, IPP_INTERNAL_ERROR));
			ifprint((DBG_VERBOSE, "    ippStatus Codes: IPP_OK_SUBST %x IPP_OK_CONFLICT %d  IPP_SERVICE_UNAVAILABLE %d  IPP_BAD_REQUEST %d",IPP_OK_SUBST, IPP_OK_CONFLICT, IPP_SERVICE_UNAVAILABLE, IPP_BAD_REQUEST));
			ifprint((DBG_VERBOSE, "    ippStatus Codes: IPP_PRINTER_BUSY %d  IPP_VERSION_NOT_SUPPORTED %d  IPP_NOT_AUTHORIZED %d",IPP_PRINTER_BUSY, IPP_VERSION_NOT_SUPPORTED, IPP_NOT_AUTHORIZED));
			ifprint((DBG_VERBOSE, "    ippStatus Codes: IPP_DOCUMENT_FORMAT_ERROR %d IPP_FORBIDDEN %d   IPP_NOT_FOUND %d",IPP_DOCUMENT_FORMAT_ERROR, IPP_FORBIDDEN, IPP_NOT_FOUND));
#endif
		}
        
		result = ((strncmp(ippErrorString(ipp_status), ippErrorString(IPP_OK), strlen(ippErrorString(IPP_OK))) == 0) ? OK : ERROR);
		break;
	}
	while (1);

    ifprint((DBG_VERBOSE, "_validate_job: ** validate_job result: %d", result));

    return(result);
}

static int _start_job(const ifc_print_job_t *this_p,
                      const wprint_job_params_t *job_params)
{
	ifprint((DBG_VERBOSE, "_start_job: Enter"));
    int result = -1;
    ipp_print_job_t *ipp_job;
    ipp_t           *request = NULL;               /* IPP request object */
    ipp_status_t    ipp_status = IPP_OK; /* Status of IPP request */

    ifprint((DBG_VERBOSE, "_start_job entry"));
    do
    {
        if (this_p == NULL) {
            ifprint((DBG_ERROR, "_start_job; this_p == NULL"));
            continue;
        }


        ipp_job = IMPL(ipp_print_job_t, ifc, this_p);
        //cupsSetAgent(job_params->useragent);
        request = _fill_job(IPP_PRINT_JOB, ipp_job->printer_uri, job_params);

        if (request == NULL)
            continue;

        ipp_job->status = cupsSendRequest(ipp_job->http, request, HTTP_RESOURCE, 0);
        if (ipp_job->status != HTTP_CONTINUE)
        {
            _cupsSetHTTPError(ipp_job->status);
            ipp_status = cupsLastError();
        }
        ippDelete(request);
        ifprint((DBG_LOG, "\n-> Exit print_StartJob httpPrint fd %d status %d ipp_status %d", ipp_job->http->fd,  ipp_job->status, ipp_status));

        result = ((ipp_job->status == HTTP_CONTINUE) ? 0 : -1);

    } while(0);

    return(result);
}

static int _send_data(const ifc_print_job_t *this_p,
                      const char *buffer,
                      size_t length)
{
    int result = -1;
    ipp_print_job_t *ipp_job;
    do
    {
        if (this_p == NULL)
            continue;

        ipp_job = IMPL(ipp_print_job_t, ifc, this_p);
        if (ipp_job->http == NULL)
            continue;

        if (ipp_job->status != HTTP_CONTINUE)
            continue;

        if (length != 0)
            ipp_job->status = cupsWriteRequestData(ipp_job->http, buffer, length);
        result = ((ipp_job->status == HTTP_CONTINUE) ? length : -1);
    } while(0);
    return(result);
}

static int _end_job(const ifc_print_job_t *this_p)
{
	ifprint((DBG_VERBOSE, "_end_job: Enter"));
    int result = -1;
    ipp_t           *response; /* IPP response object */
    ipp_attribute_t *attr; /* Attribute pointer */
    ipp_attribute_t *attrptr;        /* Attribute pointer */
    int op = IPP_PRINT_JOB;
    ipp_print_job_t *ipp_job;
    int job_id = -1;

    char buffer[1024];

    do
    {
        if (this_p == NULL)
            continue;

        ipp_job = IMPL(ipp_print_job_t, ifc, this_p);

        if (ipp_job->http == NULL)
            continue;

        ifprint((DBG_VERBOSE, " ** print_EndJob  entry httpPrint %d", ipp_job->http->fd));

        ipp_job->status = cupsWriteRequestData(ipp_job->http, buffer, 0);

        if (ipp_job->status != HTTP_CONTINUE)
        {
            response = NULL;
            ifprint((DBG_ERROR, "   Error: from cupsWriteRequestData  http.fd %d:  status %d", ipp_job->http->fd,  ipp_job->status));
        } 
        else
        {
            result = 0;
            ifprint((DBG_VERBOSE, "  0 length Bytes sent  status %d",  ipp_job->status));
            response = cupsGetResponse(ipp_job->http, HTTP_RESOURCE);

            if ((attr = ippFindAttribute(response, "job-id", IPP_TAG_INTEGER)) == NULL)
            {
                ifprint((DBG_ERROR, "  sent cupsDoRequest  %s  job id is null; received",ippOpString(op))); 
            }
            else
            {
                job_id = ippGetInteger(attr, 0);
                ifprint((DBG_LOG, "  sent cupsDoRequest  %s job_id %d; received", ippOpString(op), job_id));
            }
	
            if (response != NULL)
			{
                for(attrptr = ippFirstAttribute(response); attrptr; attrptr = ippNextAttribute(response))
					print_attr(attrptr);
				ippDelete(response);
			}
        }	
        ifprint((DBG_VERBOSE, " ** print_EndJob exit status %d job_id %d", ipp_job->status, job_id));

    } while(0);
    return(result);
}
