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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ippstatus_capabilities.h"   // move these to above the calls to cups files
#include "ipphelper.h"
#include "cups.h"
//#include <stdio.h>
#include "http-private.h"
#include <fcntl.h>
#include "http.h"
#include "ipp.h"
#include "wtypes.h"
#include "wprint_debug.h"

static const char *pattrs[] =   /* Requested printer attributes */
{
	 "ipp-versions-supported",
	 "printer-make-and-model",
	 "printer-name",
	 "printer-uri-supported",
	 "printer-more-info",
	 "charset-configured",
	 "charset-supported",
	 "color-supported",
	 "compression-supported",
	 "copies-supported",
	 "document-format-supported",
	 "fit-to-page-default",
	 "finishings-supported",
	 "job-creation-attributes-supported",
	 "media-col-default",
	 "media-col-supported",
	 "media-default",
	 "media-left-margin-supported",
	 "media-right-margin-supported",
	 "media-top-margin-supported",
	 "media-bottom-margin-supported",
	 "media-default",
	 "media-size-supported",
	 "media-source-supported",
	 "media-supported", 
	 "media-type-supported", 
	 "operations-supported",
	 "output-bin-supported",
	 "page-bottom-default",
	 "page-left-default",
	 "page-right-default",
	 "page-top-default",
	 "printer-dns-sd-name",
	 "printer-info",
	 "print-quality-supported",
	 "printer-resolution-supported",
	 "printer-settable-attributes-supported",
	 "printer-type",
	 "reference-uri-schemes-supported",
	 "sides-supported",
     "printer-device-id",
     "epcl-version-supported",
	 "pdf-fit-to-page-supported",
	 "pclm-raster-back-side",
	 "pclm-strip-height-supported",
	 "pclm-strip-height-preferred",
	 "pclm-compression-method-preferred",
     "pclm-source-resolution-supported",
     "printer-supply-info-uri"
};

#undef ifprint
#define ifprint(statements) ipp_wprint_ifc->debug statements

static void _init(const ifc_printer_capabilities_t* this_p,
                  const char *printer_address);
static int _get_capabilities(const ifc_printer_capabilities_t* this_p,
                             printer_capabilities_t *capabilities);
static void _destroy(const ifc_printer_capabilities_t* this_p);
                   

static ifc_printer_capabilities_t _capabilities_ifc = {
   .init               = _init,
   .get_capabilities   = _get_capabilities,
   .get_margins        = NULL,
   .destroy            = _destroy,
};

typedef struct
{
    http_t *http;
    char printer_uri[1024];
    ifc_printer_capabilities_t ifc;
} ipp_capabilities_t;

const ifc_printer_capabilities_t *ipp_status_get_capabilities_ifc(const ifc_wprint_t *wprint_ifc)
{
    ipp_wprint_ifc = wprint_ifc;
    ifprint((DBG_VERBOSE, "ippstatus_capabilities: ipp_status_get_capabilities_ifc: Enter"));
    ipp_capabilities_t *caps = (ipp_capabilities_t*)malloc(sizeof(ipp_capabilities_t));
    if (caps == NULL)
        return(NULL);

    memset(caps, 0, sizeof(ipp_capabilities_t));
    caps->http = NULL;

    memcpy(&caps->ifc, &_capabilities_ifc, sizeof(ifc_printer_capabilities_t));
    return(&caps->ifc);
}

static void _init(const ifc_printer_capabilities_t *this_p,
                  const char *printer_address)
{
	ifprint((DBG_VERBOSE, "ippstatus_capabilities: _init: Enter"));
    ipp_capabilities_t *caps;
    do
    {
        if (this_p == NULL)
            continue;
        caps = IMPL(ipp_capabilities_t, ifc, this_p);

        if (caps->http != NULL)
        {
        	ifprint((DBG_VERBOSE, "ippstatus_capabilities: _init(): http != NULL closing HTTP"));
            httpClose(caps->http);
        }

        int ippPortNumber = ippPort();
        ifprint((DBG_VERBOSE, "ippstatus_capabilities: _init(): ippPortNumber: %d", ippPortNumber));
        //http_uri_status_t http_assemble_status =
        		httpAssembleURIf(HTTP_URI_CODING_ALL, caps->printer_uri, sizeof(caps->printer_uri),
                         "ipp", NULL, printer_address, ippPortNumber, "/ipp/printer");
        caps->http = httpConnectEncrypt(printer_address, ippPortNumber, cupsEncryption());
        httpSetTimeout(caps->http, DEFAULT_IPP_TIMEOUT, NULL, 0);
        if (caps->http == NULL)
        {
        	ifprint((DBG_ERROR, "ippstatus_capabilities: _init(): http is NULL "));
//        	caps->http = httpConnectEncrypt(printer_address, ippPortNumber, cupsEncryption());
//        	        if (caps->http == NULL)
//        	        {
//        	        	ifprint((DBG_ERROR, "ippstatus_capabilities: _init(): http is still NULL "));
//        	        }
        }
    } while(0);
}

static int _get_capabilities(const ifc_printer_capabilities_t *this_p,
                             printer_capabilities_t *capabilities)
{
	ifprint((DBG_VERBOSE, "ippstatus_capabilities: _get_capabilities: Enter"));
    int result = -1;
    int service_unavailable_retry_count = 0;
    int bad_request_retry_count = 0;
    ipp_capabilities_t *caps;
    ipp_t           *request = NULL;     /* IPP request object */
    ipp_t           *response = NULL;    /* IPP response object */
    ipp_attribute_t *attr;               /* Current IPP attribute */
    ipp_attribute_t *attrptr;            /* Attribute pointer */
    ipp_version_state ipp_version_supported = new_request_sequence;
    int op = IPP_GET_PRINTER_ATTRIBUTES;

    ipp_status_t ipp_status = IPP_OK; /* Status of IPP request */
    printer_mediaSizes_t printer_mediaSizes;

    if (capabilities != NULL)
    {
        memset(capabilities, 0, sizeof(printer_capabilities_t));
    }

    do
    {
    	ipp_status = IPP_OK; //reset ipp_status
        if (this_p == NULL)
        {
        	break;
        }

        caps = IMPL(ipp_capabilities_t, ifc, this_p);
        if (caps->http == NULL)
        {
        	ifprint((DBG_VERBOSE, "ippstatus_capabilities: _get_capabilities: caps->http is NULL"));
        	break;
        }

        request = ippNewRequest(op);
        if(set_ipp_version(request, caps->printer_uri, caps->http, ipp_version_supported) !=0)
		{
        	ifprint((DBG_ERROR, "ippstatus_capabilities: _get_capabilities: set_ipp_version!=0, version not set"));
        	ipp_status = IPP_VERSION_NOT_SUPPORTED;

			// 20120302 - LK - added ippDelete(request) to clean up allocated request before
	    	// returning (resource leak reported by Coverity)
	        ippDelete(request);
        	break;
		}

		ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
		                     "printer-uri", NULL, caps->printer_uri);

		ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                      "requested-attributes",
                      sizeof(pattrs) / sizeof(pattrs[0]),
                      NULL, pattrs);


        ifprint((DBG_VERBOSE, "  IPP_GET_PRINTER_ATTRIBUTES %s request:", ippOpString(op)));
        for (attrptr = ippFirstAttribute(request); attrptr; attrptr = ippNextAttribute(request))
            print_attr(attrptr);

        if ((response = cupsDoRequest(caps->http, request, "/ipp/printer")) == NULL)
        {
            ipp_status = cupsLastError();
            ifprint((DBG_ERROR, "  ippstatus_capabilities: _get_capabilities:  response is null:  ipp_status %d %s",
                   ipp_status, ippErrorString(ipp_status) ));
            if((ipp_status == IPP_SERVICE_UNAVAILABLE) && (service_unavailable_retry_count<IPP_SERVICE_UNAVAILBLE_MAX_RETRIES))
            {
            	ifprint((DBG_ERROR, "1282 received, retrying %d of %d", service_unavailable_retry_count, IPP_SERVICE_UNAVAILBLE_MAX_RETRIES));
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
        	ifprint((DBG_VERBOSE, "ipp CUPS last ERROR: %d, %s", ipp_status, ippErrorString(ipp_status)));
        	if (ipp_status == IPP_BAD_REQUEST)
			{
        		ifprint((DBG_ERROR, "IPP_Status of IPP_BAD_REQUEST received. retry (%d) of (%d)", bad_request_retry_count, IPP_BAD_REQUEST_MAX_RETRIES));
				if(bad_request_retry_count>IPP_BAD_REQUEST_MAX_RETRIES)
					break;
				bad_request_retry_count++;
				ippDelete(response);
				continue;
			}
        	if(ipp_status == IPP_VERSION_NOT_SUPPORTED)
			{
				ipp_version_supported = ipp_version_was_not_supported;
				ippDelete(response);
				continue;
			}
        	ifprint((DBG_VERBOSE, "\n %s received  now call parse_printerAttributes:", ippOpString(op)));
        	parse_printerAttributes(response, capabilities, &printer_mediaSizes);
        	ifprint((DBG_VERBOSE, "\n\n %s received:", ippOpString(op)));
            for (attrptr = ippFirstAttribute(response); attrptr; attrptr = ippNextAttribute(response))
                print_attr(attrptr);
            ifprint((DBG_VERBOSE, ""));
            if ((attr = ippFindAttribute(response, "printer-state", IPP_TAG_ENUM)) == NULL)
            {
                ifprint((DBG_VERBOSE, "  printer-state: null"));
            } else {
                ifprint((DBG_LOG, "    printer-state %d:",(ipp_pstate_t)ippGetInteger(attr, 0)));
            }
        }

        if(response!=NULL)
        {
        	ippDelete(response);
        }

		result = ((strncmp(ippErrorString(ipp_status), ippErrorString(IPP_OK), strlen(ippErrorString(IPP_OK))) == 0) ? OK : ERROR);
        break;
    } while(1);

    ifprint((DBG_LOG, " ippstatus_capabilities: _get_capabilities: returning %d:",result));
    return(result);
}

static void _destroy(const ifc_printer_capabilities_t *this_p)
{
    ipp_capabilities_t *caps;
    ifprint((DBG_VERBOSE, "ippstatus_capabilities: _destroy(): enter"));
    do
    {
        if (this_p == NULL)
            continue;
        caps = IMPL(ipp_capabilities_t, ifc, this_p);

        if (caps->http != NULL)
            httpClose(caps->http);

         free(caps);
    } while(0);
}

