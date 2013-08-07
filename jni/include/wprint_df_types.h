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
#ifndef __WPRINT_DF_TYPES_H__
#define __WPRINT_DF_TYPES_H__

#include "pe_media_types.h"
#include "print_media_tray_types.h"


typedef enum {
    DF_COLOR_SPACE_MONO,       //  MONO job
    DF_COLOR_SPACE_COLOR,      //  sRGB COLOR job
    DF_COLOR_SPACE_ADOBE_RGB   //  supported in pclm only

} DF_color_space_t;


typedef enum {
   DF_DUPLEX_MODE_NONE,   /* no dpulex */
   DF_DUPLEX_MODE_BOOK,   /* long edge binding */
   DF_DUPLEX_MODE_TABLET, /* short edge binding */

} DF_duplex_t;


/* Paper sizes used for photo card slot, PictBridge and Bluetooth prints,
   and for copy, fax and reprint functions.  This list does not include paper
   sizes that are only used for PCL driver printing. */

typedef enum
{
   DF_MEDIA_SIZE_A           = US_LETTER,
   DF_MEDIA_SIZE_LEGAL       = US_LEGAL,
   DF_MEDIA_SIZE_A4          = ISO_AND_JIS_A4,
   DF_MEDIA_SIZE_HAGAKI      = JPN_HAGAKI_PC,
   DF_MEDIA_SIZE_PHOTO_4X6   = INDEX_CARD_4X6,
   DF_MEDIA_SIZE_PHOTO_L     = PHOTO_L_SIZE_CARD,
   DF_MEDIA_SIZE_PHOTO_5X7   = INDEX_CARD_5X7,
   DF_MEDIA_SIZE_A6          = ISO_AND_JIS_A6,
   DF_MEDIA_SIZE_PHOTO_8X10  = US_GOVERNMENT_LETTER,
   DF_MEDIA_SIZE_PHOTO_11X14 = US_EDP,
   DF_MEDIA_SIZE_SUPER_B     = SUPER_B,
   DF_MEDIA_SIZE_A3          = ISO_AND_JIS_A3,
   DF_MEDIA_SIZE_PHOTO_4X12  = PHOTO_4X12,
   DF_MEDIA_SIZE_PHOTO_10X15 = PHOTO_10X15,
   DF_MEDIA_SIZE_CD_3_5_INCH = PRINTABLE_CD_3_5_INCH,
   DF_MEDIA_SIZE_CD_5_INCH   = PRINTABLE_CD_5_INCH,
   DF_MEDIA_SIZE_PHOTO_4X8   = PHOTO_4X8,
   DF_MEDIA_SIZE_B_TABLOID   = B_TABLOID,
   DF_MEDIA_SIZE_PHOTO_5x7_MAIN_TRAY = PHOTO_5X7_MAIN_TRAY,
   
   /* add here */
   DF_MEDIA_SIZE_UNKNOWN   =  UNKNOWN_MEDIA_SIZE

} DF_media_size_t;

/* Borderless mask is used to differentiate 
 * borderless media sizes in the UI.  The
 * values below must not include this bit! */ 
#define BORDERLESS_MASK (0x8000UL)

/* Get the base size value with mask removed */
#define DF_MEDIA_SIZE_BASE_VALUE(_s)((_s)&(~BORDERLESS_MASK))
/* Macro to check if the size is borderless */
#define DF_MEDIA_SIZE_IS_BORDERLESS(_s)(((_s)&BORDERLESS_MASK)==BORDERLESS_MASK)


typedef enum
{
  DF_MEDIA_PLAIN,              
  DF_MEDIA_SPECIAL,             
  DF_MEDIA_PHOTO,            
  DF_MEDIA_TRANSPARENCY,           
  DF_MEDIA_IRON_ON,                   
  DF_MEDIA_IRON_ON_MIRROR,
  DF_MEDIA_ADVANCED_PHOTO,
  DF_MEDIA_FAST_TRANSPARENCY,
  DF_MEDIA_BROCHURE_GLOSSY ,       
  DF_MEDIA_BROCHURE_MATTE,        
  DF_MEDIA_PHOTO_GLOSSY,         
  DF_MEDIA_PHOTO_MATTE,         
  DF_MEDIA_PREMIUM_PHOTO,         
  DF_MEDIA_OTHER_PHOTO,
  DF_MEDIA_PRINTABLE_CD,
  DF_MEDIA_PREMIUM_PRESENTATION,
  
  /* add new types here */

  DF_MEDIA_UNKNOWN = 99

} DF_media_type_t;


typedef enum {
   DF_QUALITY_FAST, 
   DF_QUALITY_NORMAL, 
   DF_QUALITY_BEST,
   DF_QUALITY_MAXDPI,
   DF_QUALITY_AUTO

} DF_quality_t;


#endif /* __WPRINT_DF_TYPES_H__ */
