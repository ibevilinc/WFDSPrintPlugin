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
#ifndef __INC__pe_media_types_h_
#define __INC__pe_media_types_h_

#include <stdint.h>

typedef enum {

  /* default (platform/tray) behaviour */
  PE_media_verify_default_action_e,
  
  /* ignore mismatch */
  PE_media_verify_ignore_e,
  
  /* cancel on mismatch */
  PE_media_verify_cancel_e,
  
  /* hold paper and notify user (event will be 
   * generated) */
  PE_media_verify_hold_and_notify_e,
  
  /* eject paper and notify user (event will be 
   * generated) */
  PE_media_verify_eject_and_notify_e,
  
} PE_media_verify_action_t;

typedef enum {

  /* default (platform/tray) behaviour */
  PE_media_verify_default_check_e,
  
  /* no verification */
  PE_media_verify_none_e,

  /* fast verification (if available) */
  PE_media_verify_fast_e,

  /* full verification (if available) */
  PE_media_verify_full_e,

} PE_media_verify_check_t;

typedef enum
{
    PT_MEDIA_UNKNOWN     = -1,
    PT_PLAIN_PAPER       = 0,
    PT_BOND_PAPER        = 1,
    PT_SPECIAL_PAPER     = 2,
    PT_GLOSSY_PAPER      = 3,
    PT_TRANSPARENCY      = 4,
    PT_FAST_GLOSSY       = 5,
    PT_FAST_TRANSPARENCY = 6,
    PT_PRINTABLE_CD      = 7,
    PT_BROCHURE          = 8,
    PT_NUM_MEDIA_TYPES   = 9  /* add new types above this line */
} PT_media_t;

typedef int PT_media_subtype_t;

/*
 * A numeration of the different media sizes known by the printing system.
 * The numeration of the media ID corresponds to the PCL numeration of the
 * media ID.  There is also a numeration for custom size (101).  This enum
 * contains all the values that are currently defined for media types.  The
 * product may choose to support any *subset* of these defined media types.
 */
typedef enum
{
    US_EXECUTIVE                    = 1,
    US_LETTER                       = 2,
    US_LEGAL                        = 3,
    US_EDP                          = 4,
    EUROPEAN_EDP                    = 5,
    B_TABLOID                       = 6,
    US_GOVERNMENT_LETTER            = 7,
    US_GOVERNMENT_LEGAL             = 8,
    FOLIO                           = 9,
    FOOLSCAP                        = 10,
    LEDGER                          = 11,
    C_SIZE                          = 12,
    D_SIZE                          = 13,
    E_SIZE                          = 14,
    MINI                            = 15,
    SUPER_B                         = 16,
    ROC16K                          = 17,
    ROC8K                           = 19,
    ISO_AND_JIS_A10                 = 20,
    ISO_AND_JIS_A9                  = 21,
    ISO_AND_JIS_A8                  = 22,
    ISO_AND_JIS_A7                  = 23,
    ISO_AND_JIS_A6                  = 24,
    ISO_AND_JIS_A5                  = 25,
    ISO_A5                          = 25,   /* for compatibility; revisit */
    ISO_AND_JIS_A4                  = 26,
    ISO_A4                          = 26,   /* for compatibility; revisit */
    ISO_AND_JIS_A3                  = 27,
    ISO_AND_JIS_A2                  = 28,
    ISO_AND_JIS_A1                  = 29,
    ISO_AND_JIS_A0                  = 30,
    ISO_AND_JIS_2A0                 = 31,
    ISO_AND_JIS_4A0                 = 32,
    K8_270X390MM                    = 33,
    K16_195X270MM                   = 34,
    K8_260X368MM                    = 35,
    RA4                             = 36,
    SRA4                            = 37, 
    SRA3                            = 38,
    RA3                             = 39,
    JIS_B10                         = 40,
    JIS_B9                          = 41,
    JIS_B8                          = 42,
    JIS_B7                          = 43,
    JIS_B6                          = 44,
    JIS_B5                          = 45,
    JIS_B4                          = 46,
    JIS_B3                          = 47,
    JIS_B2                          = 48,
    JIS_B1                          = 49,
    JIS_B0                          = 50,
    ISO_B10                         = 60,
    ISO_B9                          = 61,
    ISO_B8                          = 62,
    ISO_B7                          = 63,
    ISO_B6                          = 64,
    ISO_B5                          = 65,
    ISO_B4                          = 66,
    ISO_B3                          = 67,
    ISO_B2                          = 68,
    ISO_B1                          = 69,
    ISO_B0                          = 70,
    JAPANESE_POSTCARD_SINGLE        = 71,
    JPN_HAGAKI_PC                   = 71,   /* for compatibility; revisit */
    JAPANESE_POSTCARD_DOUBLE        = 72,
    JPN_OUFUKU_PC                   = 72,   /* for compatibility; revisit */
    ISO_A6_POSTCARD                 = 73,
    ISO_A6_CARD                     = 73,   /* for compatibility; revisit */
    INDEX_CARD_4X6                  = 74,
    US_SMALL_IDX                    = 74,   /* for compatibility; revisit */
    INDEX_CARD_5X8                  = 75,
    US_LARGE_IDX                    = 75,   /* for compatibility; revisit */
    PHOTO_4X6                       = 76,
    JAPANESE_POSTCARD_WITH_TAB      = 77,
    INDEX_CARD_3X5                  = 78,
    MONARCH                         = 80,
    COMMERCIAL_10                   = 81,
    NO_10_ENVELOPE                  = 81,   /* for compatibility; revisit */
    CATALOG_1                       = 82,
    ENVELOPE_NO_6_75                = 83,
    K16_184X260MM                   = 89,
    INTERNATIONAL_DL                = 90,
    INT_DL_ENVELOPE                 = 90,   /* for compatibility; revisit */
    INTERNATIONAL_C5                = 91,

/* CODE_CHANGE - VTUNG Glacier matched INT_C6_ENVELOPE to an incorrect value
 *		 of 91.
 */
    INT_C6_ENVELOPE                 = 92,   /* for compatibility; revisit */
    INTERNATIONAL_C6                = 92,
    INTERNATIONAL_C4                = 93,
    PRINTABLE_CD_3_5_INCH           = 98,
    PRINTABLE_CD_5_INCH             = 99,
    INTERNATIONAL_B5                = 100,
    CUSTOM                          = 101,
    COMMERCIAL_9                    = 102,
    CUSTOM_CARD                     = 108,
    US_ENVELOPE_A2                  = 109,
    A2_ENVELOPE                     = 109,  /* for compatibility; revisit */
    JAPANESE_ENV_LONG_3             = 110,
    NEC_L3_ENVELOPE                 = 110,  /* for compatibility; revisit */
    JAPANESE_ENV_LONG_4             = 111,
    NEC_L4_ENVELOPE                 = 111,  /* for compatibility; revisit */
    JAPANESE_ENV_2                  = 112,
    HP_GREETING_CARD_ENVELOPE       = 114,
    US_PHOTO_9X12                   = 116,
    US_PHOTO_ALBUM_12X12            = 117,
    PHOTO_10X15                     = 118,
    PHOTO_CABINET                   = 119,
    SUPER_B_PAPER                   = 120,
    PHOTO_L_SIZE_CARD               = 121, 
    LSIZE_CARD                      = 121,
    INDEX_CARD_5X7                  = 122,
    PHOTO_E_SIZE_CARD               = 123, 
    PHOTO_KG_SIZE_CARD              = 124, 
    PHOTO_2E_SIZE_CARD              = 125, 
    PHOTO_2L_SIZE_CARD              = 126, 
/* Rotated Media (add 256 to the unrotated value) */
    US_EXECUTIVE_ROTATED            = 257,
    US_LETTER_ROTATED               = 258,
    ISO_AND_JIS_A5_ROTATED          = 281,
    ISO_AND_JIS_A4_ROTATED          = 282,
    JIS_B5_ROTATED                  = 301,

/* Need a media size for products that want to
 * reject any media that doesn't have an exact match.
 * UNKNOWN_MEDIA_SIZE can't be used because it is used on
 * other (most) products that don't want this behavior.
 * lcanaday 10-05-2007
 * */
   UNDEFINED_MEDIA_SIZE               = 29999,


/* Adding this for PIP-South.
 * I need to go through the "official" process of selecting real
 * numbers for these values, but for now will use 30000 and 30001.
 * dwills 5-19-2004
 */
    PHOTO_4X12                      = 30000,
    PHOTO_4X8                       = 30001,
    PHOTO_5X7_MAIN_TRAY             = 30002,
/* Other Media */
    CUSTOM_ROLL                     = 32766,
    UNKNOWN_MEDIA_SIZE              = 32767,
} PT_size_t;

typedef uint32_t PE_media_props_t;

#define PE_MEDIA_PROP_BBC 0x1

typedef struct
{
  PT_size_t		size;
  PT_media_t		type;
  PT_media_subtype_t	subtype;
  PE_media_props_t      props;
} PE_media_info_t;

/* backwards compatible name */
#define PE_loaded_media_t PE_media_info_t

/* smaller classes MUST come before large classes;
 * PE internals depend upon it (pe_metrics, pe_jobdb) 
 */
typedef enum {
  PE_media_width_class_narrow_e,
  PE_media_width_class_standard_e,
  PE_media_width_class_wide_e,
  PE_media_width_class_CNT
} PE_media_width_class_t;

typedef int32_t PT_page_units_t;

typedef struct
{
    PT_size_t       media_size;
    PT_page_units_t page_width;
    PT_page_units_t page_height;
    PT_page_units_t top_margin;
    PT_page_units_t bottom_margin;
    PT_page_units_t left_margin;
    PT_page_units_t right_margin;
    PT_page_units_t inner_radius;
    PT_page_units_t outer_radius;
    int32_t         width_in_inches;
    int32_t         height_in_inches;
    int32_t         width_in_points;
    int32_t         height_in_points;
    int32_t         width_in_micrometers;
    int32_t         height_in_micrometers;
    int16_t         originX_from_tl_in_micrometers;
    int16_t         originY_from_tl_in_micrometers;
} PT_page_spec_t;

typedef enum {
  PE_media_check_none_e, /* no media check requested */
  PE_media_check_fast_e, /* fast (if available) media check requested; e.g. bbc */
  PE_media_check_full_e, /* full (if available) media check requested, e.g. bbc (if it fails then spot) */
} PE_media_check_t;

typedef uint32_t PE_media_detect_capabilities_t;

/* if set it means the width class is used to determine size/type */
#define PE_MEDIA_DETECT_WIDTH_CLASS 0x1

/* if set it means that measurements is used to determine size/type */
#define PE_MEDIA_DETECT_MEASURE     0x2

/* if set if means that some form of media identity (e.g. back-side-barcode)
 * is used to identify the media */
#define PE_MEDIA_DETECT_IDENTITY    0x4

typedef enum {
  /* left has to be first as it's the default */
  PE_media_datum_left_e,
  PE_media_datum_right_e,
} PE_media_datum_t;

typedef enum {
  PE_media_load_not_attempted_e,
  PE_media_load_success_e,
  PE_media_load_out_of_media_e,
  PE_media_load_holder_not_found_e,
  PE_media_load_holder_slipped_e,
  PE_media_load_upside_down_e,
  PE_media_load_too_short_e,
} PE_media_load_status_t;


/* jam location bit-field definitions */
typedef enum {

  /* First, one special value: Use if media jam loction reporting is
     supported, but is not known in this situation. */
  PE_media_jam_location_unknown            =  0,

  // Media jam bit-field locations - originally populated for mimas,
  // Feb. 2010
  PE_media_jam_location_input_tray         =  0x1,
  PE_media_jam_location_turn_roller        =  0x2,
  PE_media_jam_location_platen_area        =  0x4,
  PE_media_jam_location_duplexer           =  0x8,
  PE_media_jam_location_output_tray        =  0x10
} PE_media_jam_location_t;

typedef struct {
      /* Indicates if jam location identification is supported.  If
         this is false, the location field below is NOT meaningful. */
      int   location_ident_supported;

      /* bitfield, using one or more values from the enum
         PE_media_jam_location_t */
      uint32_t location;
} PE_media_jam_info_t;

#endif /* __INC__pe_media_types_h_ */
