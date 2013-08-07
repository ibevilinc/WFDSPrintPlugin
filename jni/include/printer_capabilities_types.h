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
#ifndef __PRINTER_CAPABILITIES_TYPES_H__
#define __PRINTER_CAPABILITIES_TYPES_H__

#define MAX_SIZES_SUPPORTED 30
#define MAX_MEDIA_TRAYS_SUPPORTED 10
#define MAX_MEDIA_TYPES_SUPPORTED 20
#include "wprint_df_types.h"

#define MAX_URI_LENGTH 256

typedef struct
{
    unsigned char canDuplex;
    unsigned char hasPhotoTray;
    unsigned char canPrintBorderless;
    unsigned char canPrintPDF;
    unsigned char canPrintJPEG;
    unsigned char canPrintPCLm;
	unsigned char canPrintPWG;
    unsigned char canPrintOther;				/* Some other format support; need to query to get it*/
    char		  name[256];					/* Printer or class name */
    char		  make[256];					/* Make and Model */
    unsigned char canPrintQualityDraft;		    /* Supports print quality settings, need to query to get them*/
    unsigned char canPrintQualityNormal;		/* Supports print quality settings, need to query to get them*/
    unsigned char canPrintQualityHigh;		    /* Supports print quality settings, need to query to get them*/
    unsigned char canRotateDuplexBackPage;	    /* Can we get this??  0 = can't do it*/
    unsigned char hasColor;					    /* Color supported */
    unsigned char hasFaceDownTray;
    DF_media_size_t supported_media_sizes[MAX_SIZES_SUPPORTED];
    unsigned int    num_supported_media_sizes;
    int           ippVersionMajor;    			/* IPP major version (0 not supported) */
    int           ippVersionMinor;    			/* IPP minor version (0 not supported) */
    int 		  ePCL_ipp_version; 			/* ePCL over ipp supported version */
    int           strip_height;					/* image strip height supported by device */

    unsigned long long  supported_input_mime_types;

    PT_media_src_t supported_media_trays[MAX_MEDIA_TRAYS_SUPPORTED];
    unsigned int num_supported_media_trays;

    DF_media_type_t supported_media_types[MAX_MEDIA_TYPES_SUPPORTED];
    unsigned int num_supported_media_types;

    unsigned char isSupported;
    unsigned char canCancel;

    unsigned char canGrayscale;

    unsigned int printerTopMargin;
    unsigned int printerBottomMargin;
    unsigned int printerLeftMargin;
    unsigned int printerRightMargin;

    unsigned char inkjet;
    unsigned char canPrint300DPI;
    unsigned char canPrint600DPI;

    char suppliesURI[MAX_URI_LENGTH];

} printer_capabilities_t;

#endif /* __PRINTER_CAPABILITIES_TYPES_H__ */
