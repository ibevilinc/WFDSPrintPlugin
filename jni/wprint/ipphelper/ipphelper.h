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
#ifndef _IPP_HELPER_H_
#  define _IPP_HELPER_H_

/*
 * Include necessary headers...
 */

#include "lib_wprint.h"
#include "ifc_printer_capabilities.h"
#include "ippstatus.h"
#include "ifc_status_monitor.h"
#include "http.h"
#include "ipp.h"
#include "ifc_wprint.h"

/*
 * Partial port of httpSetTimeout code from newer cups version
 * Originally was going to use timeout of 15 seconds, but switched to 30 to
 * match the values being used in IPP examples from cups source code
 */
#define DEFAULT_IPP_TIMEOUT 30.0

/*
 * C++ magic...
 */


#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */

typedef struct ipp_version_supported_s		/**** IPP Supported Versions ****/
{
  unsigned char supportsIpp10;   			/* IPP 1.0 */
  unsigned char supportsIpp11;				/* IPP 1.1 */
  unsigned char supportsIpp20;				/* IPP 2.0 */
  unsigned char supportsIpp21;				/* IPP 2.1 */
  unsigned char supportsIpp22;				/* IPP 2.2*/
} ipp_version_supported_t;

typedef enum
{
	new_request_sequence,
	ipp_version_has_been_resolved,
	ipp_version_was_not_supported,

} ipp_version_state;

#define IPP_SERVICE_UNAVAILBLE_MAX_RETRIES 3
#define IPP_BAD_REQUEST_MAX_RETRIES 2

typedef struct 
{
//	PT_size_t  pt_size;
    media_size_t  pt_size;
    char      *ipp_media_keyword;
} ipp_pt_to_keyword_table_t;

extern const ifc_wprint_t *ipp_wprint_ifc;

#if 0
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

#endif


/** What is the use model here? */
typedef struct printer_mediaSizes_s		/**** Do we need to split these by name and size? ****/
{
  unsigned char na_executive__7_25x10_5in;		
  unsigned char na_letter__8_5x11in;				
  unsigned char na_legal__8_5x14in;				
  unsigned char na_govt__letter_8x10in;		
  unsigned char na_invoice__5_5x8_5in;		
  unsigned char iso_a5__148x210mm;		
  unsigned char iso_a4__210x297mm;		 
  unsigned char jis_b5__182x257mm;					
  unsigned char jpn_hagaki__100x148mm;			
  unsigned char iso_a6__105x148mm;			
  unsigned char na_index4x6__4x6in;	
  unsigned char na_index5x8__5x8in;					
  unsigned char na_index3x5__3x5in;				
  unsigned char na_monarch__3_875x7_5in;				
  unsigned char na_number10__4_125x9_5in;    			
  unsigned char iso_dl__110x220mm;			
  unsigned char iso_c5__162x229mm;	
  unsigned char iso_c6__114x162mm;					
  unsigned char na_a2__4_375x5_75in;				
  unsigned char jpn_chou3__120x235mm;				
  unsigned char jpn_chou4__90x205mm; 
  unsigned char oe_l__3_5x5in;			
  unsigned char na_5x7__5x7in;					
  unsigned char na_personal__3_625x6_5in;		
  unsigned char jpn_oufuku__148x200mm;				
  unsigned char iso_b5__176x250mm;				
  unsigned char om_smallphoto__100x150mm; 
  } printer_mediaSizes_t;


  #if 0
typedef struct printer_capabilities_dyn_s		
{
  ipp_pstate_t		printer_state;		            /* User the submitted the job */
  print_status_t	printer_reasons[IPP_MAX_REASONS];  /* this may become another struct of some kind....*/
//  print_status_t    printer_temp;
} printer_capabilities_dyn_t;
#endif

typedef struct job_creation_attribute_s		/**** Job - may not want all of these****/
{
  int		id;			/* The job ID */
  char		*dest;			/* Printer or class name */
  char		*title;			/* Title/job name */
  char		*user;			/* User the submitted the job */
  char		*format;		/* Document format */
  ipp_jstate_t	state;			/* Job state */
  int		size;			/* Size in kilobytes */
  int		priority;		/* Priority (1-100) */
  time_t	completed_time;		/* Time the job was completed */
  time_t	creation_time;		/* Time the job was created */
  time_t	processing_time;	/* Time the job was processed */
} job_creation_attributes_t;


typedef struct	mapping_media_s			/**** Attribute mapping data ****/
{
  PT_size_t	media_size;		/* Option has multiple values? */
  const char	*media_name;			/* Option/attribute name */
//  ipp_tag_t	value_tag;		/* Value tag for this attribute */
//  ipp_tag_t	group_tag;		/* Group tag for this attribute */
} mapping_media_t;

#define PAGE_STATUS_MAX 200		//maybe a lot less...

typedef struct media_supported_s
{                                                      
  int maximum_width;                               // Widest paper          
  media_size_t media_size[PAGE_STATUS_MAX];  // all supported papesizes
  int idxKeywordTranTable[PAGE_STATUS_MAX];					 // index to entry in keyword trans table.
} media_supported_t;

#if 0
// see _ipp_option_t ipp_options in encode.c
static const mapping_media_t mapping_media[] =
{
	{US_EXECUTIVE, "na_executive_7.25x10.5in"},
	{US_LETTER, "na_letter_8.5x11in"},
	{US_LEGAL, "na_legal_8.5x14in"},
	{US_GOVERNMENT_LETTER, "na_govt-letter_8x10in"},
};
#endif


#if 0
typedef struct print_job_params_s		/**** Job - may not want all of these****/
{
   PT_size_t              media_size;
   DF_media_type_t        media_type;
   DF_quality_t           print_quality;
   DF_duplex_t            duplex;
//   duplex_dry_time_t      dry_time;
   DF_color_space_t       color_space;
   PT_media_src_t         media_tray;
   int                    page_num;
   int                    num_copies;
 //  bool              borderless;
 //  bool              cancelled;
//   bool              last_page;
} print_job_params_t;
#endif
/*
 * Prototypes...
 */

extern ipp_status_t get_PrinterState (http_t *http, char *printer_uri,printer_state_dyn_t  *printer_state_dyn, ipp_pstate_t *printer_state);
extern void get_PrinterStateReason( ipp_t *response, ipp_pstate_t *printer_state, printer_state_dyn_t *printer_state_dyn);
extern ipp_status_t get_printerAttributes (http_t *http, char *printer_uri,int num_values, const char * const *pattrs);
extern ipp_status_t get_JobStatus (http_t *http, char *printer_uri, int job_id, ipp_jstate_t   *job_state); 
//extern ipp_status_t validatePrintJob(http_t *http, char *printer_uri);
//extern ipp_status_t get_printerCapabilities (http_t *http, char *printer_uri,  printer_capabilities_t *capabilities ,
//								ipp_version_supported_t *ippVersions, printer_mediaSizes_t *printer_mediaSizes);

extern void parse_printerAttributes(ipp_t *response, printer_capabilities_t *capabilities, 
									printer_mediaSizes_t *printer_mediaSizes);
extern void parse_IPPVersions(ipp_t *response, ipp_version_supported_t *ippVersions);	
extern int set_ipp_version(ipp_t *, char  *, http_t *, ipp_version_state);

//extern void parse_getMediaSupported(ipp_t *response, printer_mediaSizes_t *printer_mediaSizes);
extern void parse_getMediaSupported(ipp_t *response, printer_mediaSizes_t *printer_mediaSizes, media_supported_t *media_supported);

//extern ipp_status_t do_PrintJobStart( http_t *http, char *printer_uri,	const char *server);			

extern void  debuglist_printerCapabilities(printer_capabilities_t *capabilities,
					printer_mediaSizes_t *printer_mediaSizes);
extern bool shouldSkipDocFormat(ipp_t *response, ipp_attribute_t *attrptr);
extern void debuglist_printerStatus(printer_state_dyn_t   *printer_state_dyn);	
extern void debuglist_printParameters(const wprint_job_params_t   *job_params);
extern void print_col(ipp_t *col);	
extern void	print_attr(ipp_attribute_t *attr);

extern ipp_quality_t ippMapPQ( DF_quality_t  print_quality);
extern media_size_t ipp_find_pt_type(char *ipp_media_keyword);
extern int ipp_find_media_size(const char *ipp_media_keyword, media_size_t *media_size);

extern char* mapDFMediaToIPPKeyword(DF_media_size_t media_size);

/*
 * C++ magic...
 */

#  ifdef __cplusplus
#  endif /* __cplusplus */
#endif /* !_IPP_HELPER_H_ */

/*
 * End of "$Id: ipp.h 9120 2010-04-23 18:56:34Z mike $".
 */
