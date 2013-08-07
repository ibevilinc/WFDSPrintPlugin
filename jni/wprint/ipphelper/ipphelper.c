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
#include "lib_wprint.h"
#include "config.h"
#include "cups.h"
#include <stdio.h>
#include "http-private.h"
#include <fcntl.h>
#include "http.h"
#include "ipp.h"
#include "ipphelper.h"
#include "ippstatus.h"
#include "wprint_status_types.h"
#include "printer_capabilities_types.h"
#include "wprint_debug.h"

#include "ippstatus_monitor.h"
#include "ippstatus_capabilities.h"
#include "ipp_print.h"
#include "wprint_io_plugin.h"

const ifc_wprint_t *ipp_wprint_ifc;
int determine_ipp_version(char *, http_t * );
int test_and_set_ipp_version(char  *, http_t * , int, int );
#undef ifprint
#define ifprint(statements) ipp_wprint_ifc->debug statements


//#ifndef O_BINARY
//#  define O_BINARY 0
//#endif /* !O_BINARY */

/* see pwg.org document PWG 5101.1 for these names */
ipp_pt_to_keyword_table_t pt_to_keyword_trans_table[] = {
    { US_LETTER, "na_letter_8.5x11in"},
    { US_EXECUTIVE, "na_executive_7.25x10.5in"},
    { US_LEGAL, "na_legal_8.5x14in"},
    { US_GOVERNMENT_LETTER, "na_govt-letter_8x10in"},
    { MINI, "na_invoice_5.5x8.5in"},
    { ISO_A6_CARD, "iso_a6_105x148mm"},
    { ISO_AND_JIS_A6, "iso_a6_105x148mm"},
    { ISO_A5, "iso_a5_148x210mm"},
    { ISO_A4, "iso_a4_210x297mm"},
    { ISO_B7, "iso_b7_88x125mm"},
    { ISO_B5, "iso_b5_176x250mm"},
    { JIS_B7, "jis_b7_91x128mm"},
    { JIS_B6, "jis_b6_128x182mm"},
    { JIS_B5, "jis_b5_182x257mm"},
    { JIS_B4, "jis_b4_257x364mm"},
    { JPN_HAGAKI_PC, "jpn_hagaki_100x148mm"},
    { JPN_OUFUKU_PC, "jpn_oufuku_148x200mm"},
    { INDEX_CARD_3X5, "na_index-3x5_3x5in"},
    { INDEX_CARD_4X6, "na_index-4x6_4x6in"},
    { INDEX_CARD_5X8, "na_index-5x8_5x8in"},
    { INDEX_CARD_5X7, "na_5x7_5x7in"},
    { PHOTO_L_SIZE_CARD, "oe_l_3.5x5in"},
    { PHOTO_10X15, "om_small-photo_100x150mm"},
    { NO_10_ENVELOPE, "na_number-10_4.125x9.5in"},
    { INT_DL_ENVELOPE, "iso_dl_110x220mm"},
    { JAPANESE_ENV_LONG_3, "jpn_chou3_120x235mm"},
    { JAPANESE_ENV_LONG_4, "jpn_chou4_90x205mm"},
    { INTERNATIONAL_C5, "iso_c5_162x229mm"},
    { INT_C6_ENVELOPE, "iso_c6_114x162mm"},
    { ENVELOPE_NO_6_75, "na_personal_3.625x6.5in"},
    { MONARCH, "na_monarch_3.875x7.5in"},
    { A2_ENVELOPE, "na_a2_4.375x5.75in"},
    { US_EDP, "na_edp_11x14in"},
    { LEDGER, "na_ledger_11x17in"},
    { B_TABLOID, "na_ledger_11x17in"},
    { SUPER_B, "na_super-b_13x19in"},
    { SUPER_B_PAPER, "na_arch-b_12x18in"},
    { ISO_AND_JIS_A3, "iso_a3_297x420mm"},
};

//
static int __ipp_version_major = 2;
static int __ipp_version_minor = 0;


int set_ipp_version( ipp_t * op_to_set, char  *printer_uri, http_t * http, ipp_version_state use_existing_version)
{
	ifprint((DBG_VERBOSE, "ipphelper: set_ipp_version(): Enter %d", use_existing_version));
	if (op_to_set == NULL)
	{
		return -1;
	}
	switch (use_existing_version)
	{
	case new_request_sequence:
		__ipp_version_major = 2;
		__ipp_version_minor = 0;
		break;
	case ipp_version_has_been_resolved:
		break;
	case ipp_version_was_not_supported:
		if	(determine_ipp_version(printer_uri, http) != 0)
		{
			return -1;
		}
		break;
	}
    ippSetVersion(op_to_set, __ipp_version_major, __ipp_version_minor);

	ifprint((DBG_VERBOSE, "ipphelper: set_ipp_version(): Done"));
	return 0;
}

static const char * __request_ipp_version[] =   /* Requested printer ipp-version-supported */
{
	 "ipp-versions-supported"
};

int determine_ipp_version(char  *printer_uri, http_t * http)
{
	ifprint((DBG_VERBOSE, "ipphelper: determine_ipp_version(): Enter printer_uri =  %s", printer_uri));


	if (http == NULL)
	{

		ifprint((DBG_ERROR, "ipphelper: determine_ipp_version(): http is NULL cannot continue"));
		return -1;
	}
	if ((test_and_set_ipp_version(printer_uri, http, 1, 1) == 0)
			|| (test_and_set_ipp_version(printer_uri, http, 1, 0) == 0)
			|| (test_and_set_ipp_version(printer_uri, http, 2, 0) == 0))
	{
		ifprint((DBG_VERBOSE, "successfully set ipp version."));
	}
	else
	{
		ifprint((DBG_VERBOSE, "could not get ipp version using any known ipp version."));
		return -1;
	}


	return 0;
}

int test_and_set_ipp_version(char  *printer_uri, http_t * http, int major, int minor)
{
	ifprint((DBG_VERBOSE, "ipphelper: test_and_set_ipp_version(): Enter %d - %d", major, minor ));
	int return_value = -1;
	int service_unavailable_retry_count = 0;
	int bad_request_retry_count = 0;
	ipp_t           *request = NULL;  /* IPP request object */
	ipp_t           *response; /* IPP response object */
	ipp_version_supported_t ippVersions;
	memset(&ippVersions, 0, sizeof(ipp_version_supported_t));

	int op = IPP_GET_PRINTER_ATTRIBUTES;

	do
	{
		request = ippNewRequest(op);
        ippSetVersion(request, major, minor);
		ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
		ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
				"requested-attributes",
				sizeof(__request_ipp_version) / sizeof(__request_ipp_version[0]),
				NULL, __request_ipp_version);
		if ((response = cupsDoRequest(http, request, "/ipp/printer")) == NULL)
		{
			ipp_status_t ipp_status = cupsLastError();
			ifprint((DBG_VERBOSE, "  test_and_set_ipp_version:  response is null:  ipp_status %d %s",ipp_status, ippErrorString(ipp_status)));
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
			return_value = -1;
		}
		else
		{
			ipp_status_t ipp_status = cupsLastError();
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

			parse_IPPVersions(response, &ippVersions);
			if (ippVersions.supportsIpp20)
			{
				__ipp_version_major = 2;
				__ipp_version_minor = 0;
				return_value = 0;
				ifprint((DBG_VERBOSE, "test_and_set_ipp_version(): ipp version set to %d,%d",__ipp_version_major, __ipp_version_minor));
			}
			else if (ippVersions.supportsIpp11)
			{
				__ipp_version_major = 1;
				__ipp_version_minor = 1;
				return_value = 0;
				ifprint((DBG_VERBOSE, "test_and_set_ipp_version(): ipp version set to %d,%d",__ipp_version_major, __ipp_version_minor));
			}
			else if (ippVersions.supportsIpp10)
			{
				__ipp_version_major = 1;
				__ipp_version_minor = 0;
				return_value = 0;
				ifprint((DBG_VERBOSE, "test_and_set_ipp_version(): ipp version set to %d,%d",__ipp_version_major, __ipp_version_minor));
			}
			else
			{
				ifprint((DBG_VERBOSE, "ipp version not found"));
				return_value = -1;
			}

		}
		if(response!=NULL) ippDelete(response);
		break;
	}while(1);

	return return_value;
}
//char    printer_uri[1024];
//char	server[HTTP_MAX_URI];	/* Server */
//int		port;			/* Port number */	
//const char *filename = "/home/craig/projects/cups/ippPalm/sample/testImage.jpg";



ipp_status_t
get_printerAttributes (http_t *http,
					   char  *printer_uri,		/* I - URI buffer */
					   int num_values, 
					   const char * const *pattrs) {

	ifprint((DBG_VERBOSE, "ipphelper: get_printerAttributes(): Enter"));
	int service_unavailable_retry_count = 0;
	int bad_request_retry_count = 0;
	ipp_t           *request = NULL;  /* IPP request object */
    ipp_t           *response = NULL; /* IPP response object */
    ipp_attribute_t *attr;     /* Current IPP attribute */
    ipp_attribute_t *attrptr;		/* Attribute pointer */

    ipp_version_state ipp_version_supported = ipp_version_has_been_resolved;
    ipp_status_t	ipp_status = IPP_OK;		/* Status of IPP request */
 //   ipp_attribute_t	*printer_name;	/* printer-name */
 //   ipp_attribute_t	*printer_state;	/* printer-state */

    int op = IPP_GET_PRINTER_ATTRIBUTES;

	do
	{
		ipp_status = IPP_OK; //reset ipp_status
		request = ippNewRequest(op);
		if (set_ipp_version(request, printer_uri, http, ipp_version_supported) != 0)
		{
			ifprint((DBG_ERROR, "ipphelper: get_printerAttributes(): set_ipp_version!=0, version not set"));
			ipp_status = IPP_VERSION_NOT_SUPPORTED;

			// 20120305 - LK - added ippDelete(request) to clean up allocated request before
	    	// returning (resource leak was NOT reported by Coverity for some reason)
	        ippDelete(request);
			break;
		}

		ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri",
						NULL, printer_uri);

		ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
				"requested-attributes", num_values, NULL, pattrs);


		ifprint((DBG_VERBOSE, "  IPP_GET_PRINTER_ATTRIBUTES %s request:", ippOpString(op)));
        for(attrptr = ippFirstAttribute(request); attrptr; attrptr = ippNextAttribute(request))
			print_attr(attrptr);

		if ((response = cupsDoRequest(http, request, "/ipp/printer")) == NULL)
		{
			ipp_status = cupsLastError();
			ifprint((DBG_VERBOSE, "  get_printerAttributes:  response is null:  ipp_status %d %s",ipp_status, ippErrorString(ipp_status) ));
			if ((ipp_status == IPP_SERVICE_UNAVAILABLE) && (service_unavailable_retry_count < IPP_SERVICE_UNAVAILBLE_MAX_RETRIES))
			{
				ifprint((DBG_ERROR, "1282 received, retrying %d of %d", service_unavailable_retry_count, IPP_SERVICE_UNAVAILBLE_MAX_RETRIES));
				service_unavailable_retry_count++;
				continue;
			}
			if (ipp_status == IPP_BAD_REQUEST)
			{
				ifprint((DBG_ERROR, "IPP_Status of IPP_BAD_REQUEST (%d) received. retry", IPP_BAD_REQUEST));
				service_unavailable_retry_count++;
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
			ifprint((DBG_VERBOSE, "\n %s received:", ippOpString(op)));
            for(attrptr = ippFirstAttribute(response); attrptr; attrptr = ippNextAttribute(response))
				print_attr(attrptr);
			//        printer_name  = ippFindAttribute(response, "printer-name", IPP_TAG_NAME);
			//        printer_state = ippFindAttribute(response, "printer-state", IPP_TAG_ENUM);

			//   ifprint((DBG_VERBOSE, "printer-state %s:",(ipp_pstate_t)printer_name->values[0].string.text));
			//   ifprint((DBG_VERBOSE, "printer-state %d:",(ipp_pstate_t)printer_state->values[0].integer));

			if ((attr = ippFindAttribute(response, "printer-state",
					IPP_TAG_ENUM)) == NULL)
			{
				ifprint((DBG_VERBOSE, "  printer-state null"));
			}
			else
			{
				ifprint((DBG_VERBOSE, "    printer-state %d:",(ipp_pstate_t)ippGetString(attrptr, 0, NULL)));
			}
		}
		break;
	} while (1);
	if(response!=NULL) ippDelete(response);
	return(ipp_status);
}


ipp_status_t
get_PrinterState (http_t *http, 
				  char  *printer_uri,		/* I - URI buffer */
				  printer_state_dyn_t *printer_state_dyn,
				  ipp_pstate_t *printer_state) {

	ifprint((DBG_VERBOSE, "ipphelper: get_PrinterState(): Enter"));
    static const char *pattrs[] =   /* Requested printer attributes */
	{
		"printer-make-and-model",
		"printer-state",
		"printer-state-message",
		"printer-state-reasons"
	};

//   http_t          *http;     /* HTTP object */
	ipp_t *request; /* IPP request object */
	// 20120305 - LK - initializing response to NULL to remove Coverity issue
	ipp_t *response = NULL; /* IPP response object */
	//  ipp_attribute_t *attr;     /* Current IPP attribute */
//   ipp_attribute_t *attrptr;		/* Attribute pointer */
//   int              idx = 0 ;
	int service_unavailable_retry_count = 0;
	int bad_request_retry_count = 0;
	ipp_status_t ipp_status = IPP_OK; /* Status of IPP request */
	int op = IPP_GET_PRINTER_ATTRIBUTES;
	ipp_version_state ipp_version_supported = ipp_version_has_been_resolved;
//   ifprint((DBG_VERBOSE, "  get_PrinterState entry: httpStatus fd  zero out printer_capabilities_dyn %d", http->fd));

	if (printer_state_dyn == NULL)
	{
		ifprint(
				(DBG_ERROR, "ipphelper: get_PrinterState():   ERROR:  get_PrinterState entry: printer_state_dyn is null"));
		return ipp_status;
	}
 //  printer_capabilities_dyn_t   printer_capabilities_dyn;  

//   printer_capabilities_dyn.printer_state = PRINT_STATUS_INITIALIZING;
//   for ( idx = 0; idx < IPP_MAX_REASONS; idx ++) {
 //      printer_capabilities_dyn.printer_reasons[idx] = PRINT_STATUS_INITIALIZING;
//   }

	if (printer_state)
		*printer_state = IPP_PRINTER_STOPPED;
	else
	{
		ifprint((DBG_ERROR, "ipphelper: get_PrinterState():   get_PrinterState: printer_state is null"));
	}
	do
	{
		ipp_status = IPP_OK; //reset ipp_status
		request = ippNewRequest(op);
		if (set_ipp_version(request, printer_uri, http, ipp_version_supported) != 0)
		{
			ifprint((DBG_ERROR, "ipphelper: get_PrinterState(): set_ipp_version!=0, version not set"));
			ipp_status = IPP_VERSION_NOT_SUPPORTED;
			ippDelete(request);
			break;
		}

		ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri",
				NULL, printer_uri);

		ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
				"requested-attributes", sizeof(pattrs) / sizeof(pattrs[0]),
				NULL, pattrs);

		if ((response = cupsDoRequest(http, request, "/ipp/printer")) == NULL)
		{
			ipp_status = cupsLastError();
			ifprint((DBG_ERROR, "ipphelper: get_PrinterState():   response is null:  ipp_status %d",ipp_status ));
			if ((ipp_status == IPP_SERVICE_UNAVAILABLE) && (service_unavailable_retry_count < IPP_SERVICE_UNAVAILBLE_MAX_RETRIES))
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
			printer_state_dyn->printer_status = PRINT_STATUS_UNABLE_TO_CONNECT;
			printer_state_dyn->printer_reasons[0] = PRINT_STATUS_UNABLE_TO_CONNECT;

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
			get_PrinterStateReason(response, printer_state, printer_state_dyn);
//		  debuglist_printerStatus(printer_state_dyn);
			ifprint((DBG_VERBOSE, "ipphelper: get_PrinterState():  get_PrinterState printer_state_dyn->printer_status  %d", printer_state_dyn->printer_status));
		}

		ifprint((DBG_VERBOSE, "ipphelper: get_PrinterState():   get_PrinterState exit http->fd %d ipp_status %d  printer_state %d", http->fd, ipp_status, printer_state_dyn->printer_status));
		//    ippDelete(request);
		break;
	} while (1);
	if(response!=NULL) ippDelete(response);
	ifprint((DBG_VERBOSE, "ipphelper: get_PrinterState(): Exit"));
    return ipp_status;

}


void get_PrinterStateReason( ipp_t *response, ipp_pstate_t *printer_state, printer_state_dyn_t *printer_state_dyn){

	ifprint((DBG_VERBOSE, "ipphelper: get_PrinterStateReason(): Enter"));
   ipp_attribute_t *attrptr;		/* Attribute pointer */
    int reason_idx = 0;
    int idx = 0;
	ipp_pstate_t printer_ippstate = PRINT_STATUS_IDLE; 

    if ((attrptr = ippFindAttribute(response, "printer-state", IPP_TAG_ENUM)) == NULL) {
        ifprint((DBG_ERROR, " get_PrinterStateReason  printer-state null"));
		printer_state_dyn->printer_status = PRINT_STATUS_UNABLE_TO_CONNECT;
		printer_state_dyn->printer_reasons[0] = PRINT_STATUS_UNABLE_TO_CONNECT;
    } else {
		printer_ippstate = (ipp_pstate_t)ippGetInteger(attrptr, 0);
        *printer_state = printer_ippstate;
 //       printer_state_dyn->printer_state = (ipp_pstate_t)attrptr->values[0].integer;
		// set the printer_status; they may be modified based on the status reasons below.
		switch (printer_ippstate) {
		case IPP_PRINTER_IDLE:
			printer_state_dyn->printer_status = PRINT_STATUS_IDLE;
			break;
		case IPP_PRINTER_PROCESSING:
			printer_state_dyn->printer_status = PRINT_STATUS_PRINTING;
			break;
		case IPP_PRINTER_STOPPED:
			printer_state_dyn->printer_status = PRINT_STATUS_SVC_REQUEST;
			break;
		}
//		ifprint((DBG_VERBOSE, "  get_PrinterStateReason printer_ippstate %d printer_state_dyn->printer_status  %d", 
//			   printer_ippstate, printer_state_dyn->printer_status));
    }
    if ((attrptr = ippFindAttribute(response, "printer-state-reasons", IPP_TAG_KEYWORD)) == NULL) {
      ifprint((DBG_ERROR, " get_PrinterStateReason printer-state reason null"));
	  printer_state_dyn->printer_status = PRINT_STATUS_UNABLE_TO_CONNECT;
	  printer_state_dyn->printer_reasons[0] = PRINT_STATUS_UNABLE_TO_CONNECT;
    } else {
 //       ifprint((DBG_VERBOSE, "  get_PrinterStateReason  (%d) printer-state-reasons :",ippGetCount(attrptr)));
        for (idx = 0; idx < ippGetCount(attrptr); idx ++) {
 //           ifprint((DBG_VERBOSE, "  get_PrinterStateReason printer-state-reasons (%d) %s", idx, attrptr->values[idx].string.text));

        	//According to RFC2911, any of these can have
        	//-error, -warning, or -report appended to end
			if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_NONE, strlen(IPP_PRNT_STATE_NONE)) == 0) {

				switch (printer_ippstate) {
				case IPP_PRINTER_IDLE:
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_IDLE;
					break;
				case IPP_PRINTER_PROCESSING:
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_PRINTING; 
					break;
				case IPP_PRINTER_STOPPED:
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_UNKNOWN;   // should this be PRINT_STATUS_SVC_REQUEST
					break;
				}				 
			} else if (strncmp(ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_SPOOL_FULL, strlen(IPP_PRNT_STATE_SPOOL_FULL)) == 0) { 
				switch (printer_ippstate) {
				case IPP_PRINTER_IDLE:
//					printer_state_dyn->printer_status = PRINT_STATUS_IDLE;   // Unclear is this shold be PRINT_STATUS_SVC_REQUEST, but may never be reached.
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_UNKNOWN;  
					break;
				case IPP_PRINTER_PROCESSING:
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_PRINTING; 
					break;
				case IPP_PRINTER_STOPPED:
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_UNKNOWN;   	// should this be PRINT_STATUS_SVC_REQUEST
					break;
				}
			}   // not blocking conditions -- may need to modify printer_state_dyn->printer_status
            else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_MARKER_SUPPLY_LOW, strlen(IPP_PRNT_STATE_MARKER_SUPPLY_LOW)) == 0) {
				printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_LOW_ON_INK;
            } else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_TONER_LOW, strlen(IPP_PRNT_STATE_TONER_LOW)) == 0) {
				printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_LOW_ON_TONER;
            } else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_OTHER_WARN, strlen(IPP_PRNT_STATE_OTHER_WARN)) == 0) {
				printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_UNKNOWN;            // may need to modify
            } else {
				// check blocking cases
				printer_state_dyn->printer_status = PRINT_STATUS_SVC_REQUEST;

				if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_MEDIA_NEEDED, strlen(IPP_PRNT_STATE_MEDIA_NEEDED)) == 0) {
					 printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_OUT_OF_PAPER;
				} else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_MEDIA_EMPTY, strlen(IPP_PRNT_STATE_MEDIA_EMPTY)) == 0) {
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_OUT_OF_PAPER;
				} else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_TONER_EMPTY, strlen(IPP_PRNT_STATE_TONER_EMPTY)) == 0) {
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_OUT_OF_TONER;
				} else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_MARKER_SUPPLY_EMPTY, strlen(IPP_PRNT_STATE_MARKER_SUPPLY_EMPTY)) == 0) {
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_OUT_OF_INK;
			    } else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_DOOR_OPEN, strlen(IPP_PRNT_STATE_DOOR_OPEN)) == 0) {
				   printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_DOOR_OPEN;
				} else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_MEDIA_JAM, strlen(IPP_PRNT_STATE_MEDIA_JAM)) == 0) {
				   printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_JAMMED;
				} else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_SHUTDOWN, strlen(IPP_PRNT_SHUTDOWN)) == 0) {
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_SHUTTING_DOWN;
				} else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_STATE_OTHER_ERR, strlen(IPP_PRNT_STATE_OTHER_ERR)) == 0) {
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_SVC_REQUEST;                       // may need to modify
				} else if (strncmp (ippGetString(attrptr, idx, NULL), IPP_PRNT_PAUSED, strlen(IPP_PRNT_PAUSED)) == 0) {
					printer_state_dyn->printer_reasons[reason_idx++] = PRINT_STATUS_UNKNOWN;                           // may need to modify
				} 
			}
       }  // end of reasons loop
    }

}


ipp_status_t
get_JobStatus (http_t *http, 
			   char  *printer_uri,		/* I - URI buffer */
			   int job_id, 
			   ipp_jstate_t *job_state) {

	ifprint((DBG_VERBOSE, "ipphelper: get_JobStatus(): Enter"));
	   static const char * const jattrs[] =
	   {			/* Job attributes we want */
		  "job-id",
		  "job-printer-uri",
		  "job-originating-user-name",
		  "job-name",
		  "job-state",
		  "job-state-reasons"
	   };
	   int service_unavailable_retry_count = 0;
	   int bad_request_retry_count = 0;
	   int op = IPP_GET_JOB_ATTRIBUTES;
	   ipp_t           *request;  /* IPP request object */
	   ipp_t           *response; /* IPP response object */
	   ipp_attribute_t *attr;     /* Current IPP attribute */
	   ipp_attribute_t *attrptr;		/* Attribute pointer */
	   ipp_status_t	ipp_status = IPP_OK;		/* Status of IPP request */
	   ipp_version_state ipp_version_supported = ipp_version_has_been_resolved;

	   if (job_state != NULL)
	   {
		   *job_state = IPP_JOB_COMPLETED;
	   }
	   else
	   {
		   ifprint((DBG_ERROR, "  get_JobStatus: printer_state is null"));
		   //return error...
	   }

 //   int version = major * 10 + minor;
	ifprint((DBG_VERBOSE, " get_JobStatus IPP_GET_JOB_ATTRIBUTES  http->fd %d", http->fd));
	do
	{
		ipp_status = IPP_OK; //reset ipp_status

        request = ippNewRequest(op);
		if (set_ipp_version(request, printer_uri, http, ipp_version_supported) != 0)
		{
			ifprint((DBG_ERROR, "ipphelper: get_JobStatus(): set_ipp_version!=0, version not set"));
			ipp_status = IPP_VERSION_NOT_SUPPORTED;

			// 20120302 - LK - added ippDelete(request) to clean up allocated request before
	    	// returning (resource leak reported by Coverity)
	        ippDelete(request);
			break;
		}

		ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri",
				NULL, printer_uri);
		ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id",
				job_id);

		ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
				"requested-attributes", sizeof(jattrs) / sizeof(jattrs[0]),
				NULL, jattrs);

        for(attrptr = ippFirstAttribute(request); attrptr; attrptr = ippNextAttribute(request))
			print_attr(attrptr);

		if ((response = cupsDoRequest(http, request, "/ipp/printer")) == NULL)
		{
			ipp_status = cupsLastError();
			ifprint((DBG_ERROR, "  get_JobStatus:  response is null:  ipp_status %d",ipp_status ));
			if (ipp_status == IPP_SERVICE_UNAVAILABLE && (service_unavailable_retry_count < IPP_SERVICE_UNAVAILBLE_MAX_RETRIES))
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
			ifprint((DBG_VERBOSE, "  get_JobStatus:  response!=null:  ipp_status %d",ipp_status ));
			for (attrptr = ippFirstAttribute(response); attrptr; attrptr = ippNextAttribute(response))
				print_attr(attrptr);
		}

		if ((attr = ippFindAttribute(response, "job-state", IPP_TAG_ENUM))== NULL)
		{
			ifprint((DBG_ERROR, "  job-state null"));
		}
		else
		{
		   // 20120302 - LK - check explicitly for job_state != NULL to avoid Coverity issue
		   if (job_state != NULL)
		   {
			   // 20120302 - LK - changed cast from ipp_pstate_t to ipp_jstate_t to fix
			   // coverity (and possibly other) issue(s)
			   *job_state = (ipp_jstate_t) ippGetInteger(attr, 0);
		   }
		}

		if(response!=NULL) ippDelete(response);

		break;
	}while(1);

	 // 20120302 - LK - check explicitly for job_state != NULL to avoid Coverity issue
	 if (job_state != NULL)
	 {
		 ifprint((DBG_VERBOSE, "  get_JobStatus exit  ipp_status %d, job_state %d",ipp_status, *job_state));
	 }
	 else
	 {
		 ifprint((DBG_ERROR, "  get_JobStatus exit  ipp_status %d, job_state IS NULL AND THIS IS A BAD THING!!!",ipp_status));
	 }

     return ipp_status;
}

void print_col(ipp_t *col) /* I - Collection attribute to print */
{
	int i; /* Looping var */
	ipp_attribute_t *attr; /* Current attribute in collection */

	ifprint((DBG_VERBOSE, "{"));
	for (attr = ippFirstAttribute(col); attr; attr = ippNextAttribute(col))
	{
		switch (ippGetValueTag(attr))
		{
		case IPP_TAG_INTEGER:
		case IPP_TAG_ENUM:
			for (i = 0; i < ippGetCount(attr); i++)
				ifprint((DBG_VERBOSE, "    %s(%s%s)= %d ",
						ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr)), ippGetInteger(attr, i)));
			break;

		case IPP_TAG_BOOLEAN:
			for (i = 0; i < ippGetCount(attr); i++)
				if (ippGetBoolean(attr, i))
					ifprint((DBG_VERBOSE, "    %s(%s%s)= true ",
							ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
							ippTagString(ippGetValueTag(attr))));
				else
					ifprint((DBG_VERBOSE, "    %s(%s%s)= false ",
							ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
							ippTagString(ippGetValueTag(attr))));
			break;

		case IPP_TAG_NOVALUE:
			ifprint((DBG_VERBOSE, "    %s(%s%s)= novalue",
					ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
					ippTagString(ippGetValueTag(attr))));
			break;

		case IPP_TAG_RANGE:
			for (i = 0; i < ippGetCount(attr); i++) {
                int lower, upper;
                lower = ippGetRange(attr, i, &upper);
				ifprint((DBG_VERBOSE, "    %s(%s%s)= %d-%d ",
						ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr)),
                        lower, upper));
            }
			break;

		case IPP_TAG_RESOLUTION:
			for (i = 0; i < ippGetCount(attr); i++) {
                ipp_res_t units;
                int xres, yres;
                xres = ippGetResolution(attr, i, &yres, &units);
				ifprint((DBG_VERBOSE, "    %s(%s%s)= %dx%d%s ",
						ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr)),
                        xres, yres,
						units == IPP_RES_PER_INCH ? "dpi" : "dpc"));
            }
			break;

		case IPP_TAG_STRING:
		case IPP_TAG_TEXT:
		case IPP_TAG_NAME:
		case IPP_TAG_KEYWORD:
		case IPP_TAG_CHARSET:
		case IPP_TAG_URI:
		case IPP_TAG_MIMETYPE:
		case IPP_TAG_LANGUAGE:
			for (i = 0; i < ippGetCount(attr); i++)
				ifprint((DBG_VERBOSE, "    %s(%s%s)= \"%s\" ",
						ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr)),
						ippGetString(attr, i, NULL)));
			break;

		case IPP_TAG_TEXTLANG:
		case IPP_TAG_NAMELANG:
			for (i = 0; i < ippGetCount(attr); i++) {
                const char *charset;
                const char *text;
                text = ippGetString(attr, i, &charset);
				ifprint((DBG_VERBOSE, "    %s(%s%s)= \"%s\",%s ",
						ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr)),
						text, charset));
            }
			break;

		case IPP_TAG_BEGIN_COLLECTION:
			for (i = 0; i < ippGetCount(attr); i++)
			{
				print_col(ippGetCollection(attr, i));
			}
			break;

		default:
			break; /* anti-compiler-warning-code */
		}
	}

	ifprint((DBG_VERBOSE, "}"));
}

void print_attr(ipp_attribute_t *attr) /* I - Attribute to print */
{
	int i; /* Looping var */

	if (ippGetName(attr) == NULL)
	{
		return;
	}

	switch (ippGetValueTag(attr))
	{
	case IPP_TAG_INTEGER:
	case IPP_TAG_ENUM:
		for (i = 0; i < ippGetCount(attr); i++)
			ifprint((DBG_VERBOSE, "%s (%s%s) = %d ",
					ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
					ippTagString(ippGetValueTag(attr)), ippGetInteger(attr, i)));
		break;

	case IPP_TAG_BOOLEAN:
		for (i = 0; i < ippGetCount(attr); i++)
			if (ippGetBoolean(attr, i))
				ifprint((DBG_VERBOSE, "%s (%s%s) = true ",
						ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr))));
			else
				ifprint((DBG_VERBOSE, "%s (%s%s) = false ",
						ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr))));
		break;

	case IPP_TAG_NOVALUE:
		ifprint((DBG_VERBOSE, "%s (%s%s) = novalue",
				ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
						ippTagString(ippGetValueTag(attr))));
		break;

	case IPP_TAG_RANGE:
		for (i = 0; i < ippGetCount(attr); i++) {
            int lower, upper;
            lower = ippGetRange(attr, i, &upper);
			ifprint((DBG_VERBOSE, "%s (%s%s) = %d-%d ",
					ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
					ippTagString(ippGetValueTag(attr)),
                    lower, upper));
        }
		break;

	case IPP_TAG_RESOLUTION:
		for (i = 0; i < ippGetCount(attr); i++) {
            ipp_res_t units;
            int xres, yres;
            xres = ippGetResolution(attr, i, &yres, &units);
			ifprint((DBG_VERBOSE, "%s (%s%s) = %dx%d%s ",
					ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
					ippTagString(ippGetValueTag(attr)),
					xres, yres,
					units == IPP_RES_PER_INCH ? "dpi" : "dpc"));
        }
		break;

	case IPP_TAG_STRING:
	case IPP_TAG_TEXT:
	case IPP_TAG_NAME:
	case IPP_TAG_KEYWORD:
	case IPP_TAG_CHARSET:
	case IPP_TAG_URI:
	case IPP_TAG_MIMETYPE:
	case IPP_TAG_LANGUAGE:
		for (i = 0; i < ippGetCount(attr); i++)
			ifprint((DBG_VERBOSE, "%s (%s%s) = \"%s\" ",
					ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
					ippTagString(ippGetValueTag(attr)),
					ippGetString(attr, i, NULL)));
		break;

	case IPP_TAG_TEXTLANG:
	case IPP_TAG_NAMELANG:
		for (i = 0; i < ippGetCount(attr); i++) {
            const char *charset;
            const char *text;
            text = ippGetString(attr, i, &charset);
			ifprint((DBG_VERBOSE, "%s (%s%s) = \"%s\",%s ",
					ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
					ippTagString(ippGetValueTag(attr)), text,
					charset));
        }
		break;

	case IPP_TAG_BEGIN_COLLECTION:
		for (i = 0; i < ippGetCount(attr); i++)
		{
			ifprint((DBG_VERBOSE, "%s (%s%s): IPP_TAG_BEGIN_COLLECTION",
					ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
					ippTagString(ippGetValueTag(attr))));
			print_col(ippGetCollection(attr, i));
		}
		ifprint((DBG_VERBOSE, "%s (%s%s): IPP_TAG_END_COLLECTION",
				ippGetName(attr), ippGetCount(attr) > 1 ? "1setOf " : "",
				ippTagString(ippGetValueTag(attr))));
		break;

	default:
		break; /* anti-compiler-warning-code */
	}

}

void
parse_IPPVersions(ipp_t *response, ipp_version_supported_t *ippVersions)	/* I - Supported IPP Versions */
{
  int		i;					/* Looping var */
  ipp_attribute_t *attrptr;		/* Attribute pointer */
  char ipp10[] = "1.0";
  char ipp11[] = "1.1";
  char ipp20[] = "2.0";
  ifprint((DBG_VERBOSE, " Entered IPPVersions"));
  if (ippVersions != NULL) {
	  memset(ippVersions, 0, sizeof(ipp_version_supported_t));
	
	  ifprint((DBG_VERBOSE, " in get_supportedIPPVersions"));
	  if ((attrptr = ippFindAttribute(response, "ipp-versions-supported", IPP_TAG_KEYWORD)) != NULL) {
		  ifprint((DBG_VERBOSE, " in get_supportedIPPVersions: %d", ippGetCount(attrptr)));
		  for (i = 0; i < ippGetCount(attrptr); i ++) {
			if (strcmp (ipp10,ippGetString(attrptr, i, NULL)) == 0)
			{
				ippVersions->supportsIpp10 = 1;
			} else if (strcmp (ipp11,ippGetString(attrptr, i, NULL)) == 0) {
				ippVersions->supportsIpp11 = 1;
			} else if (strcmp (ipp20,ippGetString(attrptr, i, NULL)) == 0) {
				ippVersions->supportsIpp20 = 1;
			} else {
				ifprint((DBG_VERBOSE, "found another ipp version. %s", ippGetString(attrptr, i, NULL)));
			}
		  } 
	  }
	}

}
#define MEDIA_MAX 40;

char *mapDFMediaToIPPKeyword(DF_media_size_t media_size)
{
    int i;
    size_t mediaArraySize = (sizeof pt_to_keyword_trans_table)/(sizeof pt_to_keyword_trans_table[0]);
    for(i = 0; i < mediaArraySize; i++)
    {
        if (pt_to_keyword_trans_table[i].pt_size == (media_size_t)media_size)
            return(pt_to_keyword_trans_table[i].ipp_media_keyword);
    }
    return(pt_to_keyword_trans_table[0].ipp_media_keyword);
}


int ipp_find_media_size(const char *ipp_media_keyword, media_size_t *media_size)
{
  int		i;
	ifprint((DBG_VERBOSE, "\n ** ipp_find_media_size entry is %s", ipp_media_keyword));

	size_t mediaArraySize = (sizeof pt_to_keyword_trans_table)/(sizeof pt_to_keyword_trans_table[0]);
//	ifprint((DBG_VERBOSE,  " mediaArraySize is %d", mediaArraySize));

    for (i = 0; i < mediaArraySize; i ++) {
		if (strcmp (pt_to_keyword_trans_table[i].ipp_media_keyword,ipp_media_keyword) == 0) 
//		if (strcmp (pt_to_keyword_trans_table[i].ipp_media_keyword,attr->values[i].string.text) == 0) 
 //       if (pt_to_keyword_trans_table[i].pt_size == size)
        {
			ifprint((DBG_VERBOSE, " mediaArraySize: match string  %s  PT_size: %d", 
					pt_to_keyword_trans_table[i].ipp_media_keyword, pt_to_keyword_trans_table[i].pt_size));
            break;
        }
    }
    if (i < mediaArraySize) {
		*media_size = pt_to_keyword_trans_table[i].pt_size;
        return i;

		} else
    {
        return 0;			
		}
	return 0;
	    }


//char *ipp_find_pt_type(PT_size_t size, ipp_attribute_t *attr)
media_size_t ipp_find_pt_type(char *ipp_media_keyword) {
    int i;
	ifprint((DBG_VERBOSE, " ipp_find_pt_type entry is %s", ipp_media_keyword));

	size_t mediaArraySize = (sizeof pt_to_keyword_trans_table)/(sizeof pt_to_keyword_trans_table[0]);
//	ifprint((DBG_VERBOSE, " mediaArraySize is %d", mediaArraySize));

    for (i = 0; i < mediaArraySize; i++) {
		if (strcmp (pt_to_keyword_trans_table[i].ipp_media_keyword,ipp_media_keyword) == 0) 
//		if (strcmp (pt_to_keyword_trans_table[i].ipp_media_keyword,attr->values[i].string.text) == 0) 
 //       if (pt_to_keyword_trans_table[i].pt_size == size)
        {
			ifprint((DBG_VERBOSE, " mediaArraySize: match string  %s  PT_size: %d", 
					pt_to_keyword_trans_table[i].ipp_media_keyword, pt_to_keyword_trans_table[i].pt_size));
            break;
		}
    }
    if (i < mediaArraySize) {
        return pt_to_keyword_trans_table[i].pt_size;			   		
		} else
    {
        return 0;			
		}
	return 0;

		}


#if 0
char *ipp_find_pt_type(PT_size_t size)
{
    int i;
    
    for (i = 0; i < ARRAY_SIZE(pt_to_keyword_trans_table); i++)
    {
        if (pt_to_keyword_trans_table[i].pt_size == size)
        {
            break;
        }
    }
    if (i < ARRAY_SIZE(pt_to_keyword_trans_table))
    {
        return pt_to_keyword_trans_table[i].ipp_media_keyword;
    }
    else
    {
        return NULL;
    }
}
#endif

void
parse_getMediaSupported(ipp_t *response, printer_mediaSizes_t *printer_mediaSizes, media_supported_t *media_supported) {
  int		i;					/* Looping var */
  ipp_attribute_t *attrptr;		/* Attribute pointer */
  int sizes_idx = 0;
  ifprint((DBG_VERBOSE, " Entered getMediaSupported"));
  
  #if 0
  char na_executive[] 	= "na_executive_7.25x10.5in";		
  char na_letter[] 		= "na_letter_8.5x11in";				
  char na_legal[] 		= "na_legal_8.5x14in";				
  char na_govtletter[] = "na_govt-letter_8x10in";		
  char na_invoice_[] 	= "na_invoice_5.5x8.5in";		
  char iso_a5[] 		= "iso_a5_148x210mm";		
  char iso_a4[] 		= "iso_a4_210x297mm";		 
  char jis_b5[] 		= "jis_b5_182x257mm";					
  char jpn_hagaki[] 	= "jpn_hagaki_100x148mm";			
  char iso_a6[] 		= "iso_a6_105x148mm";			
  char na_index4x6[] 	= "na_index-4x6_4x6in";
  char na_index5x8[] 	= "na_index-5x8_5x8in";					
  char na_index3x5[] 	= "na_index-3x5_3x5in";				
  char na_monarch[] 	= "na_monarch_3.875x7.5in";				
  char na_number10[] 	= "na_number-10_4.125x9.5in";    			
  char iso_dl[] 		= "iso_dl_110x220mm";			
  char iso_c5[]    		= "iso_c5_162x229mm";	
  char iso_c6[]    		= "iso_c6_114x162mm";					
  char na_a2[]     		= "na_a2_4.375x5.75in";			
  char jpn_chou3[] 		= "jpn_chou3_120x235mm";				
  char jpn_chou4[] 		= "jpn_chou4_90x205mm"; 
  char oe_l[]      		= "oe_l_3.5x5in";			
  char na_5x7[]    		= "na_5x7_5x7in";					
  char na_personal[] 	= "na_personal_3.625x6.5in";					
  char jpn_oufuku[] 	= "jpn_oufuku_148x200mm";				
  char iso_b5[]     	= "iso_b5_176x250mm";				
  char om_smallphoto[] = "om_small-photo_100x150mm";
#endif

  if (printer_mediaSizes != NULL) {
	  memset(printer_mediaSizes, 0, sizeof(printer_mediaSizes_t));

  media_size_t media_sizeTemp;
  int idx = 0;

  if ((attrptr = ippFindAttribute(response, "media-supported", IPP_TAG_KEYWORD)) != NULL) {
	 ifprint((DBG_VERBOSE, "media-supported  found; number of values %d",ippGetCount(attrptr)));
	 for (i = 0; i < ippGetCount(attrptr); i ++) {
		 idx = ipp_find_media_size(ippGetString(attrptr, i, NULL), &media_sizeTemp);
		 ifprint((DBG_VERBOSE, " Temp - i: %d  idx %d keyword: %s PT_size %d", i ,idx, ippGetString(attrptr, i, NULL),media_sizeTemp ));


//		 media_sizeTemp = ipp_find_pt_type(attrptr->values[i].string.text);
//		 ifprint((DBG_VERBOSE, " Temp - i: %d  keyword: %s PT_size %d", i , attrptr->values[i].string.text,media_sizeTemp ));
		 media_supported->media_size[sizes_idx] = media_sizeTemp;
		 media_supported->idxKeywordTranTable[sizes_idx] = idx;
		 sizes_idx++;
	 }
		 #if 0
		if (strcmp (na_executive,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = US_EXECUTIVE;
			printer_mediaSizes->na_executive__7_25x10_5in = 1;
		} else if (strcmp (na_letter,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = US_LETTER;
			printer_mediaSizes->na_letter__8_5x11in = 1;
		} else if (strcmp (na_legal,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = US_LEGAL;
			printer_mediaSizes->na_legal__8_5x14in = 1;
 		} else if (strcmp (na_govtletter,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = US_GOVERNMENT_LETTER;
			printer_mediaSizes->na_govt__letter_8x10in = 1;
		} else if (strcmp (na_invoice_,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = MINI;   
			printer_mediaSizes->na_invoice__5_5x8_5in = 1;

		} else if (strcmp (iso_a5,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = ISO_A5;
			printer_mediaSizes->iso_a5__148x210mm = 1;			
		} else if (strcmp (iso_a4,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = ISO_A4;
			printer_mediaSizes->iso_a4__210x297mm = 1;
	    } else if (strcmp (jis_b5,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = JIS_B5;
			printer_mediaSizes->jis_b5__182x257mm = 1;
	    } else if (strcmp (jpn_hagaki,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = JPN_HAGAKI_PC;
		   printer_mediaSizes->jpn_hagaki__100x148mm = 1;			
		} else if (strcmp (iso_a6,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = ISO_AND_JIS_A6;
			printer_mediaSizes->iso_a6__105x148mm = 1;	
					
		} else if (strcmp (na_index4x6,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = INDEX_CARD_4X6;
			printer_mediaSizes->na_index4x6__4x6in = 1;
		} else if (strcmp (na_index5x8,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = INDEX_CARD_5X8;
			printer_mediaSizes->na_index5x8__5x8in = 1;
		} else if (strcmp (na_index3x5,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = INDEX_CARD_3X5;
			printer_mediaSizes->na_index3x5__3x5in = 1;			
		} else if (strcmp (na_monarch,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = MONARCH;
			printer_mediaSizes->na_monarch__3_875x7_5in = 1;
		} else if (strcmp (na_number10,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = NO_10_ENVELOPE;  
		   printer_mediaSizes->na_number10__4_125x9_5in = 1;

		} else if (strcmp (iso_dl,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = INTERNATIONAL_DL;  
		   printer_mediaSizes->iso_dl__110x220mm = 1;			   		
		} else if (strcmp (iso_c5,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = INTERNATIONAL_C5;  
			printer_mediaSizes->iso_c5__162x229mm = 1;			
		} else if (strcmp (iso_c6,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = INTERNATIONAL_C6;  
		   printer_mediaSizes->iso_c6__114x162mm = 1;

		} else if (strcmp (na_a2,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = A2_ENVELOPE;  
		   printer_mediaSizes->na_a2__4_375x5_75in = 1;
		} else if (strcmp (jpn_chou3,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = JAPANESE_ENV_LONG_3;  
		   printer_mediaSizes->jpn_chou3__120x235mm = 1;			
		} else if (strcmp (jpn_chou4,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = JAPANESE_ENV_LONG_4;  
			printer_mediaSizes->jpn_chou4__90x205mm = 1;						
		} else if (strcmp (oe_l,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = PHOTO_L_SIZE_CARD;  
		   printer_mediaSizes->oe_l__3_5x5in = 1;

		} else if (strcmp (na_5x7,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = INDEX_CARD_5X7;  
		   printer_mediaSizes->na_5x7__5x7in = 1;
		} else if (strcmp (na_personal,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = ENVELOPE_NO_6_75;  
		   printer_mediaSizes->na_personal__3_625x6_5in = 1;
		   			
		} else if (strcmp (jpn_oufuku,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = JPN_OUFUKU_PC;  
		    printer_mediaSizes->jpn_oufuku__148x200mm = 1;
		} else if (strcmp (iso_b5,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = ISO_B5;  
		    printer_mediaSizes->iso_b5__176x250mm = 1;
		} else if (strcmp (om_smallphoto,attrptr->values[i].string.text) == 0) {
			media_supported->media_size[sizes_idx++] = PHOTO_10X15;  
		   printer_mediaSizes-> om_smallphoto__100x150mm = 1;						 
		} else {
			//ifprint((DBG_VERBOSE, "found one sided %s", attrptr->values[i].string.text));
		}
	  }
	  #endif
    }
	else {
		ifprint((DBG_VERBOSE, "media-supported not found"));
	}

  }
}

ipp_quality_t ippMapPQ( DF_quality_t  print_quality)
{
	switch (print_quality)
	{
	case DF_QUALITY_FAST:
		return (IPP_QUALITY_DRAFT);
		break;
	case DF_QUALITY_NORMAL:
		return (IPP_QUALITY_NORMAL);
		break;
	case DF_QUALITY_BEST:
		return (IPP_QUALITY_HIGH);
		break;
	case DF_QUALITY_MAXDPI:
		return (IPP_QUALITY_DRAFT);
		break;
	case DF_QUALITY_AUTO:
		return (IPP_QUALITY_HIGH);
		break;
	default:
		return (IPP_QUALITY_NORMAL);
		break;
	}
	return (IPP_QUALITY_NORMAL);
}

void
parse_printerAttributes(ipp_t *response, printer_capabilities_t *capabilities, 
                        printer_mediaSizes_t *printer_mediaSizes)	
{
  int		i,j;			/* Looping vars */
  ipp_attribute_t *attrptr;		/* Attribute pointer */


  ifprint((DBG_VERBOSE, " Entered parse_printerAttributes"));

  media_supported_t 		media_supported;
  media_supported.maximum_width = 0;
  for(i = 0; i <= PAGE_STATUS_MAX -1; i++)
	  media_supported.media_size[i] = 0;
 // parse_IPPVersions(response, ippVersions);
  parse_getMediaSupported(response, printer_mediaSizes, &media_supported);

  ifprint((DBG_VERBOSE, "\n *****  Media Supported   *****"));
  int idx = 0;
  capabilities->num_supported_media_sizes = 0;
  for(i = 0; i <= PAGE_STATUS_MAX -1; i++) {
	  if (media_supported.media_size[i] != 0) {
                  capabilities->supported_media_sizes[capabilities->num_supported_media_sizes++] = media_supported.media_size[i];

		  idx = media_supported.idxKeywordTranTable[i];

		   ifprint((DBG_VERBOSE, "     i %d, \tPT_Size: %d  \tidx %d \tKeyword: %s",
				i, media_supported.media_size[i], idx, pt_to_keyword_trans_table[idx].ipp_media_keyword));
	  }
  }
  ifprint((DBG_VERBOSE, ""));

  if ((attrptr = ippFindAttribute(response, "printer-name", IPP_TAG_NAME)) != NULL) {
	  strlcpy(capabilities->name, ippGetString(attrptr, 0, NULL),  sizeof(capabilities->name));
  }

  if ((attrptr = ippFindAttribute(response, "printer-make-and-model", IPP_TAG_TEXT)) != NULL) {
	  strlcpy(capabilities->make, ippGetString(attrptr, 0, NULL),  sizeof(capabilities->make));
  }

  if ((attrptr = ippFindAttribute(response, "color-supported", IPP_TAG_BOOLEAN)) != NULL) {
	  if (ippGetBoolean(attrptr, 0)) {
		  capabilities->hasColor = 1;
	  }
  }

    if ((attrptr = ippFindAttribute(response, "printer-supply-info-uri", IPP_TAG_URI)) != NULL) {
        strlcpy(capabilities->suppliesURI, ippGetString(attrptr, 0, NULL), sizeof(capabilities->suppliesURI));
    }

  char imagejpg[] = "image/jpeg";
  char imagePCL[] = "application/vnd.hp-PCL";
  char imagePCLXL[] = "application/vnd.hp-PCLXL";
  char imagePCLm[] = "application/PCLm";
  char imagePDF[] = "image/pdf";
  char imagePWG[] = "image/pwg-raster";
  char applicationPDF[] = "application/pdf";

  if ((attrptr = ippFindAttribute(response, "document-format-supported",
			IPP_TAG_MIMETYPE)) != NULL)
	{
		unsigned char has_pclxl = 0;
		for (i = 0; i < ippGetCount(attrptr); i++)
		{
			if (strcmp(imagejpg, ippGetString(attrptr, i, NULL)) == 0)
			{
				capabilities->canPrintJPEG = 1;
			}
			else if (strcmp(imagePDF, ippGetString(attrptr, i, NULL)) == 0)
			{
				capabilities->canPrintPDF = 1;
			}
			else if (strcmp(applicationPDF, ippGetString(attrptr, i, NULL)) == 0)
			{
				capabilities->canPrintPDF = 1;
			}
			else if (strcmp(imagePCLXL, ippGetString(attrptr, i, NULL)) == 0)
			{
				has_pclxl = 1;
			}
			else if (strcmp(imagePCLm, ippGetString(attrptr, i, NULL)) == 0)
			{
				capabilities->canPrintPCLm = 1;
			}
			else if (strcmp(imagePWG, ippGetString(attrptr, i, NULL)) == 0)
			{
				capabilities->canPrintPWG = 1;
			}
			else
			{
				capabilities->canPrintOther = 1;
			}
		}
	}

  if ((attrptr = ippFindAttribute(response, "sides-supported", IPP_TAG_KEYWORD)) != NULL) {
	 for (i = 0; i < ippGetCount(attrptr); i ++) {
		if (strcmp (IPP_SIDES_TWO_SIDED_SHORT_EDGE, ippGetString(attrptr, i, NULL)) == 0)
		{
//			capabilities->canDuplexShortEdge = 1;
			capabilities->canDuplex = 1;
		} else if (strcmp (IPP_SIDES_TWO_SIDED_LONG_EDGE, ippGetString(attrptr, i, NULL)) == 0)
		{
//			capabilities->canDuplexLongEdge = 1;
			capabilities->canDuplex = 1;
		} else {
			//ifprint((DBG_VERBOSE, "found one sided %s", attrptr->values[i].string.text));
		}
	  } 
  }


  // There may be others; need to try this on a laser  with multiple trays
  char mediaMainTray[] = "main";
  char mediaPhotoTray[] = "photo";

  if ((attrptr = ippFindAttribute(response, "media-source-supported", IPP_TAG_KEYWORD)) != NULL) {
	 for (i = 0; i < ippGetCount(attrptr); i ++) {
 		if (strcmp (mediaMainTray,ippGetString(attrptr, i, NULL)) == 0)
		{
//			capabilities->hasMainTray = 1;
		} else if (strcmp (mediaPhotoTray,ippGetString(attrptr, i, NULL)) == 0) {
			capabilities->hasPhotoTray = 1;
		} else {
			ifprint((DBG_VERBOSE, "found another tray %s", ippGetString(attrptr, i, NULL)));
		}
	  } 
  }

  ipp_quality_t pQuality;

  if ((attrptr = ippFindAttribute(response, "print-quality-supported", IPP_TAG_ENUM)) != NULL) {
	 for (i = 0; i < ippGetCount(attrptr); i ++) {
		pQuality = (ipp_quality_t)ippGetInteger(attrptr, i);
		switch (pQuality) 
		{
		case IPP_QUALITY_DRAFT:
			capabilities->canPrintQualityDraft = 1;
			break;
		case IPP_QUALITY_NORMAL:
			capabilities->canPrintQualityNormal = 1;
			break;
		case IPP_QUALITY_HIGH:
			capabilities->canPrintQualityHigh = 1;
			break;
		}
	  } 
  } 

    // only appears that SMM supports the pclm-source-resolution-supported attribute
    // if that is not present, use the printer-resolution-supported attribute to determine if 300DPI is supported
    capabilities->canPrint600DPI = 1;
    if ((attrptr = ippFindAttribute(response, "pclm-source-resolution-supported", IPP_TAG_RESOLUTION)) != NULL)
    {
        for (i = 0; i < ippGetCount(attrptr); i++) {
            ipp_res_t units;
            int xres, yres;
            xres = ippGetResolution(attrptr, i, &yres, &units);
            if (units == IPP_RES_PER_INCH) {
                if ((xres == 300) && (yres == 300)) {
                    capabilities->canPrint300DPI = 1;
                }
            }
        }
    }
    else if ((attrptr = ippFindAttribute(response, "printer-resolution-supported", IPP_TAG_RESOLUTION)) != NULL)
    {
        for (i = 0; i < ippGetCount(attrptr); i++) {
            ipp_res_t units;
            int xres, yres;
            xres = ippGetResolution(attrptr, i, &yres, &units);
            if (units == IPP_RES_PER_INCH) {
                if ((xres == 300) && (yres == 300)) {
                    capabilities->canPrint300DPI = 1;
                }
            }
        }
    }

  char ipp10[] = "1.0";
  char ipp11[] = "1.1";
  char ipp20[] = "2.0";

  if ((attrptr = ippFindAttribute(response, "ipp-versions-supported", IPP_TAG_KEYWORD)) != NULL) {
      unsigned char supportsIpp20 = 0;
      unsigned char supportsIpp11 = 0;
      unsigned char supportsIpp10 = 0;

	  for (i = 0; i < ippGetCount(attrptr); i ++) {
          if (strcmp (ipp10,ippGetString(attrptr, i, NULL)) == 0)
            {
                supportsIpp10 = 1;
            } else if (strcmp (ipp11,ippGetString(attrptr, i, NULL)) == 0) {
                supportsIpp11 = 1;
            } else if (strcmp (ipp20,ippGetString(attrptr, i, NULL)) == 0) {
                supportsIpp20 = 1;
            } else {
                ifprint((DBG_VERBOSE, "found another ipp version. %s", ippGetString(attrptr, i, NULL)));
            }
            if (supportsIpp20) {
                capabilities->ippVersionMajor = 2;
                capabilities->ippVersionMinor = 0;
            }else if (supportsIpp11) {
                capabilities->ippVersionMajor = 1;
                capabilities->ippVersionMinor = 1;
            }else if (supportsIpp10) {
                capabilities->ippVersionMajor = 1;
                capabilities->ippVersionMinor = 0;
            }
            else {
            	// default to 1.0
                capabilities->ippVersionMajor = 1;
                capabilities->ippVersionMinor = 0;
            }
      } 
  }

  char epcl10[] = "1.0";

  if ((attrptr = ippFindAttribute(response, "epcl-version-supported",
			IPP_TAG_KEYWORD)) != NULL)
	{
		for (i = 0; i < ippGetCount(attrptr); i++)
		{
			ifprint(
					(DBG_VERBOSE, "setting epcl_ipp_version (KEYWORD) %s", ippGetString(attrptr, i, NULL)));

			if (strstr(ippGetString(attrptr, i, NULL), epcl10) != NULL) //substring match because different devices implemented spec differently
			{
				ifprint((DBG_VERBOSE, "setting epcl_ipp_version = 1"));
				capabilities->ePCL_ipp_version = 1;
			}
		}
	}

  if ((attrptr = ippFindAttribute(response, "epcl-version-supported",
			IPP_TAG_TEXT)) != NULL)
	{
		for (i = 0; i < ippGetCount(attrptr); i++)
		{
			ifprint(
					(DBG_VERBOSE, "setting epcl_ipp_verion (TEXT) %s", ippGetString(attrptr, i, NULL)));

			if (strstr(ippGetString(attrptr, i, NULL), epcl10) != NULL) //substring match because different devices implemented spec differently
			{
				ifprint((DBG_VERBOSE, "setting epcl_ipp_verion = 1"));
				capabilities->ePCL_ipp_version = 1;
			}
		}
	}

  if ((attrptr = ippFindAttribute(response, "media-col-default",
		  IPP_TAG_BEGIN_COLLECTION)) != NULL)
  	{
  		for (i = 0; i < ippGetCount(attrptr); i++)
  		{
			ifprint(
					(DBG_VERBOSE, "Gathering margins supported"));

            ipp_t * collection = ippGetCollection(attrptr, i);

            for(j = 0, attrptr = ippFirstAttribute(collection); (j < 4) && (attrptr != NULL); attrptr = ippNextAttribute(collection))
  			{
  				if (strcmp("media-top-margin", ippGetName(attrptr)) == 0)
				{
  					capabilities->printerTopMargin = ippGetInteger(attrptr, 0);
				}
				else if (strcmp("media-bottom-margin", ippGetName(attrptr)) == 0)
				{
					capabilities->printerBottomMargin = ippGetInteger(attrptr, 0);
				}
				else if (strcmp("media-left-margin", ippGetName(attrptr)) == 0)
				{
					capabilities->printerLeftMargin = ippGetInteger(attrptr, 0);
				}
				else if (strcmp("media-right-margin", ippGetName(attrptr)) == 0)
				{
					capabilities->printerRightMargin = ippGetInteger(attrptr, 0);
				}
  			}
  		}
  	}

  // is strip length supported? if so, stored in capabilities
  if((attrptr = ippFindAttribute(response, "pclm-strip-height-preferred",
		  IPP_TAG_INTEGER)) != NULL)
  {
	  ifprint((DBG_VERBOSE, "pclm-strip-height-preferred=%d", ippGetInteger(attrptr, 0)));

	  // if the strip height is 0, the device wants us to send the entire page in one band (according to ePCL spec).
	  // since our code doesn't currently support generating an entire page in one band,
	  // set the strip height to the default value every device *should* support
	  // also, for some reason our code crashes when it attempts to generate strips at 512 or above.
	  // therefore, limiting the upper bound strip height to 256
	  if(ippGetInteger(attrptr, 0) == 0 ||
		 ippGetInteger(attrptr, 0) > 256)
	  {
		  capabilities->strip_height = STRIPE_HEIGHT;
	  }
	  else
	  {
		  capabilities->strip_height = ippGetInteger(attrptr, 0);
	  }
  }
  else
  {
	  capabilities->strip_height = STRIPE_HEIGHT;
  }

  // what is the preferred compression method - jpeg, flate, rle
  if((attrptr = ippFindAttribute(response, "pclm-compression-method-preferred",
		  IPP_TAG_KEYWORD)) != NULL)
  {
	  ifprint((DBG_VERBOSE, "pclm-compression-method-preferred=%s", ippGetString(attrptr, 0, NULL)));
	  //capabilities->pclm_compression = ippGetString(attrptr, 0, NULL);
  }
  else
  {
	  //capabilities->pclm_compression = jpeg;
  }

  // is device able to rotate back page for duplex jobs?
  if((attrptr = ippFindAttribute(response, "pclm-raster-back-side",
		  IPP_TAG_KEYWORD)) != NULL)
  {
	  ifprint((DBG_VERBOSE, "pclm-raster-back-side=%s", ippGetString(attrptr, 0, NULL)));
	  if(strcmp(ippGetString(attrptr, 0, NULL), "rotated") == 0)
	  {
		  capabilities->canRotateDuplexBackPage = 0;
		  ifprint((DBG_VERBOSE, "Device cannot rotate back page for duplex jobs."));
	  }
	  else
	  {
		  capabilities->canRotateDuplexBackPage = 1;
	  }
  }

  // look for full-bleed supported by looking for 0 on all margins
  bool topsupported = false, bottomsupported = false, rightsupported = false, leftsupported = false;
  ifprint((DBG_VERBOSE, "Looking for full-bleed support"));
  if((attrptr = ippFindAttribute(response, "media-top-margin-supported",
		  IPP_TAG_INTEGER)) != NULL)
  {
		for (i = 0; i < ippGetCount(attrptr); i++)
		{
			if(ippGetInteger(attrptr, i) == 0)
			{
				ifprint((DBG_VERBOSE, "   Top Margin Supported"));
				topsupported = true;
				break;
			}
		}
  }
  if((attrptr = ippFindAttribute(response, "media-bottom-margin-supported",
		  IPP_TAG_INTEGER)) != NULL)
  {
		for (i = 0; i < ippGetCount(attrptr); i++)
		{
			if(ippGetInteger(attrptr, i) == 0)
			{
				ifprint((DBG_VERBOSE, "   Bottom Margin Supported"));
				bottomsupported = true;
				break;
			}
		}
  }
  if((attrptr = ippFindAttribute(response, "media-right-margin-supported",
		  IPP_TAG_INTEGER)) != NULL)
  {
		for (i = 0; i < ippGetCount(attrptr); i++)
		{
			if(ippGetInteger(attrptr, i) == 0)
			{
				ifprint((DBG_VERBOSE, "   Right Margin Supported"));
				rightsupported = true;
				break;
			}
		}
  }
  if((attrptr = ippFindAttribute(response, "media-left-margin-supported",
		  IPP_TAG_INTEGER)) != NULL)
  {
		for (i = 0; i < ippGetCount(attrptr); i++)
		{
			if(ippGetInteger(attrptr, i) == 0)
			{
				ifprint((DBG_VERBOSE, "   Left Margin Supported"));
				leftsupported = true;
				break;
			}
		}
  }

  if(topsupported &&
     bottomsupported &&
     rightsupported &&
     leftsupported)
  {
	  ifprint((DBG_VERBOSE, "      full-bleed is supported"));
	  capabilities->canPrintBorderless = 1;
  }
  else
  {
	  ifprint((DBG_VERBOSE, "      full-bleed is NOT supported"));
  }

  if ((attrptr = ippFindAttribute(response, "printer-device-id", IPP_TAG_TEXT)) != NULL)
  {
	  if(strstr(ippGetString(attrptr, 0, NULL), "PCL3GUI") != NULL)
	  {
		  capabilities->inkjet = 1;
	  }
  }
  else if (capabilities->canPrintBorderless == 1)
  {
	  capabilities->inkjet = 1;
  }

  // determine if device prints pages face-down
  if ((attrptr = ippFindAttribute(response, "output-bin-supported", IPP_TAG_KEYWORD)) != NULL)
  {
	  if(strstr(ippGetString(attrptr, 0, NULL), "face-down") != NULL)
	  {
		  capabilities->hasFaceDownTray = 1;
	  }
  }

  debuglist_printerCapabilities(capabilities, printer_mediaSizes);
}

void 
debuglist_printerCapabilities(printer_capabilities_t *capabilities,
							  printer_mediaSizes_t *printer_mediaSizes)
{

  ifprint((DBG_VERBOSE, "printer name\t\t\t: %s", capabilities->name));
  ifprint((DBG_VERBOSE, "printer make\t\t\t: %s", capabilities->make));
 // ifprint((DBG_VERBOSE, "ippSupported\t\t\t: %d", capabilities->ippSupported));
 // ifprint((DBG_VERBOSE, "ipp1.0Support\t\t\t: %d", ippVersions->supportsIpp10)); 
 // ifprint((DBG_VERBOSE, "ipp1.1Support\t\t\t: %d", ippVersions->supportsIpp11)); 
 // ifprint((DBG_VERBOSE, "ipp2.0Support\t\t\t: %d", ippVersions->supportsIpp20)); 
  ifprint((DBG_VERBOSE, "canPrintPDF\t\t\t: %d", capabilities->canPrintPDF));
  ifprint((DBG_VERBOSE, "canPrintJPG\t\t\t: %d", capabilities->canPrintJPEG));
  ifprint((DBG_VERBOSE, "canPrintOther\t\t\t: %d", capabilities->canPrintOther));
  ifprint((DBG_VERBOSE, "canPrintQualityDraft\t\t: %d", capabilities->canPrintQualityDraft));
  ifprint((DBG_VERBOSE, "canPrintQualityNormal\t\t: %d", capabilities->canPrintQualityNormal));
  ifprint((DBG_VERBOSE, "canPrintQualityHigh\t\t: %d", capabilities->canPrintQualityHigh));
  ifprint((DBG_VERBOSE, "canDuplex\t\t\t\t: %d", capabilities->canDuplex));
 // ifprint((DBG_VERBOSE, "canDuplexShortEdge\t\t: %d", capabilities->canDuplexShortEdge));
 // ifprint((DBG_VERBOSE, "canDuplexLongEdge\t\t: %d", capabilities->canDuplexLongEdge));
  ifprint((DBG_VERBOSE, "canRotateDuplexBackPage\t\t: %d", capabilities->canRotateDuplexBackPage));
  ifprint((DBG_VERBOSE, "hasColor\t\t\t\t: %d", capabilities->hasColor));
  //ifprint((DBG_VERBOSE, "hasMainTray\t\t\t: %d", capabilities->hasMainTray));
  ifprint((DBG_VERBOSE, "hasPhotoTray\t\t\t: %d", capabilities->hasPhotoTray));
  ifprint((DBG_VERBOSE, "ippVersionMajor\t\t\t: %d", capabilities->ippVersionMajor));
  ifprint((DBG_VERBOSE, "ippVersionMinor\t\t\t: %d", capabilities->ippVersionMinor));
  ifprint((DBG_VERBOSE, "strip height\t\t\t: %d", capabilities->strip_height));
  ifprint((DBG_VERBOSE, "hasFaceDownTray\t\t\t: %d", capabilities->hasFaceDownTray));
  ifprint((DBG_VERBOSE, "canPrint300DPI\t\t\t: %d", capabilities->canPrint300DPI));
  ifprint((DBG_VERBOSE, "canPrint600DPI\t\t\t: %d", capabilities->canPrint600DPI));
  ifprint((DBG_VERBOSE, ""));

  #if 0
  ifprint((DBG_VERBOSE, "na_executive_7.25x10.5in\t: %d", printer_mediaSizes->na_executive__7_25x10_5in));
  ifprint((DBG_VERBOSE, "na_letter_8.5x11in\t\t: %d", printer_mediaSizes->na_letter__8_5x11in));
  ifprint((DBG_VERBOSE, "na_legal_8.5x14in\t\t: %d", printer_mediaSizes->na_legal__8_5x14in));
  ifprint((DBG_VERBOSE, "na_govt-letter_8x10in\t\t: %d", printer_mediaSizes->na_govt__letter_8x10in));
  ifprint((DBG_VERBOSE, "na_invoice_5.5x8.5in\t\t: %d", printer_mediaSizes->na_invoice__5_5x8_5in));
  ifprint((DBG_VERBOSE, "iso_a5_148x210mm\t\t: %d", printer_mediaSizes->iso_a5__148x210mm));
  ifprint((DBG_VERBOSE, "iso_a4_210x297mm\t\t: %d", printer_mediaSizes->iso_a4__210x297mm));
  ifprint((DBG_VERBOSE, "jis_b5_182x257mm\t\t: %d", printer_mediaSizes->jis_b5__182x257mm));
  ifprint((DBG_VERBOSE, "jpn_hagaki_100x148mm\t\t: %d", printer_mediaSizes->jpn_hagaki__100x148mm));
  ifprint((DBG_VERBOSE, "iso_a6_105x148mm\t\t: %d", printer_mediaSizes->iso_a6__105x148mm));
  ifprint((DBG_VERBOSE, "na_index-4x6_4x6in\t\t: %d", printer_mediaSizes->na_index4x6__4x6in));
  ifprint((DBG_VERBOSE, "na_index-5x8_5x8in\t\t: %d", printer_mediaSizes->na_index5x8__5x8in));
  ifprint((DBG_VERBOSE, "na_index-3x5_3x5in\t\t: %d", printer_mediaSizes->na_index3x5__3x5in));
  ifprint((DBG_VERBOSE, "na_monarch_3.875x7.5in\t\t: %d", printer_mediaSizes->na_monarch__3_875x7_5in));
  ifprint((DBG_VERBOSE, "na_number-10_4.125x9.5in\t: %d", printer_mediaSizes->na_number10__4_125x9_5in));
  ifprint((DBG_VERBOSE, "iso_dl_110x220mm\t\t: %d", printer_mediaSizes->iso_dl__110x220mm));
  ifprint((DBG_VERBOSE, "iso_c5_162x229mm\t\t: %d", printer_mediaSizes->iso_c5__162x229mm));
  ifprint((DBG_VERBOSE, "iso_c6_114x162mm\t\t: %d", printer_mediaSizes->iso_c6__114x162mm));
  ifprint((DBG_VERBOSE, "na_a2_4.375x5.75in\t\t: %d", printer_mediaSizes->na_a2__4_375x5_75in));
  ifprint((DBG_VERBOSE, "jpn_chou3_120x235mm\t\t: %d", printer_mediaSizes->jpn_chou3__120x235mm));
  ifprint((DBG_VERBOSE, "jpn_chou4_90x205mm\t\t: %d", printer_mediaSizes->jpn_chou4__90x205mm));
  ifprint((DBG_VERBOSE, "oe_l_3.5x5in\t\t\t: %d", printer_mediaSizes->oe_l__3_5x5in));
  ifprint((DBG_VERBOSE, "na_5x7_5x7in\t\t\t: %d", printer_mediaSizes->na_5x7__5x7in));
  ifprint((DBG_VERBOSE, "na_personal_3.625x6.5in\t\t: %d", printer_mediaSizes->na_personal__3_625x6_5in));
  ifprint((DBG_VERBOSE, "jpn_oufuku_148x200mm\t\t: %d", printer_mediaSizes->jpn_oufuku__148x200mm));
  ifprint((DBG_VERBOSE, "iso_b5_176x250mm\t\t: %d", printer_mediaSizes->iso_b5__176x250mm));
  ifprint((DBG_VERBOSE, "om_small-photo_100x150mm\t: %d", printer_mediaSizes->om_smallphoto__100x150mm));
#endif

}

void 
debuglist_printerStatus(printer_state_dyn_t   *printer_state_dyn)
{
	if(printer_state_dyn->printer_status == PRINT_STATUS_INITIALIZING)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Initializing)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_SHUTTING_DOWN)
			ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Shutting Down)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_UNABLE_TO_CONNECT)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Unable To Connect)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_UNKNOWN)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Unknown)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_OFFLINE)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Offline)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_IDLE)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (IDLE)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_PRINTING)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Printing)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_OUT_OF_PAPER)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Out Of Paper)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_OUT_OF_INK)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Out Of Ink)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_JAMMED)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Jammed)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_DOOR_OPEN)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Door Open)", printer_state_dyn->printer_status));
	else if(printer_state_dyn->printer_status == PRINT_STATUS_SVC_REQUEST)
		ifprint((DBG_VERBOSE, "ipphelper: printer status: %d (Service Request)", printer_state_dyn->printer_status));

    int              idx = 0 ;
    for (idx = 0; idx < (PRINT_STATUS_MAX_STATE + 1) ;idx++) {
        if (PRINT_STATUS_MAX_STATE != printer_state_dyn->printer_reasons[idx]) {
            ifprint((DBG_VERBOSE, "   ipphelper: printer_reasons (%d)\t: %d", idx,printer_state_dyn->printer_reasons[idx]));
        }
    }
}


void 
debuglist_printParameters(const wprint_job_params_t   *job_params)	{
	ifprint((DBG_VERBOSE, ""));
	ifprint((DBG_VERBOSE, "  %d : media_size: (Letter %d, a4 %d, 4 x 6 %d)", job_params->media_size,
		   DF_MEDIA_SIZE_A, DF_MEDIA_SIZE_A4, DF_MEDIA_SIZE_PHOTO_4X6));
	ifprint((DBG_VERBOSE, "  %d : media_type: (Plain %d, photo %d)", job_params->media_type, 
		   DF_MEDIA_PLAIN, DF_MEDIA_PHOTO));
	ifprint((DBG_VERBOSE, "  %d : print_quality: (Fast %d, normal %d, best %d)", job_params->print_quality,
		   DF_QUALITY_FAST, DF_QUALITY_NORMAL, DF_QUALITY_BEST));
	ifprint((DBG_VERBOSE, "  %d : duplex: (Single %d, Duplex >= %d)", job_params->duplex, 
		   DF_DUPLEX_MODE_NONE, DF_DUPLEX_MODE_BOOK));
	ifprint((DBG_VERBOSE, "  %d : color_space: (Black = %d Color = %d)", job_params->color_space, 
		   DF_COLOR_SPACE_MONO, DF_COLOR_SPACE_COLOR));
	ifprint((DBG_VERBOSE, "  %d : media_tray: (Main %d, Photo %d)", job_params->media_tray,
		   PT_SOURCE_TRAY_1, PT_SOURCE_PHOTO_TRAY));
	ifprint((DBG_VERBOSE, "  %d : num_copies:", job_params->num_copies));
	ifprint((DBG_VERBOSE, "  %s : print format:", job_params->print_format));
	

}

wprint_io_plugin_t *libwipp_io_reg(void)
{
static const wprint_io_plugin_t _ipp_io_plugin =
  {
      .version      = WPRINT_PLUGIN_VERSION(0),
      .port_num     = PORT_IPP,
      .getCapsIFC   = ipp_status_get_capabilities_ifc,
      .getStatusIFC = ipp_status_get_monitor_ifc,
      .getPrintIFC  = ipp_get_print_ifc,
  };
  return((wprint_io_plugin_t*)&_ipp_io_plugin);
}
