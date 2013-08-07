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
#ifdef  __INCLUDE_MEDIA_H__   // should be included only from lib_pclm.c

#include "lib_wprint.h"

#define STANDARD_SCALE_FOR_PDF    72.0
#define _MI_TO_PIXELS(n,res)      ((n)*(res)+500)/1000.0
#define _MI_TO_POINTS(n)          _MI_TO_PIXELS(n, STANDARD_SCALE_FOR_PDF)

struct MediaSizeTableElement2
{
     media_size_t      media_size;
     const char*       PCL6Name;
     const char*       XPSName;
     const float       WidthInInches;
     const float       HeightInInches;
};

static struct MediaSizeTableElement2 MasterMediaSizeTable2[] = {
     {
          US_LETTER,
	  "LETTER",                	      //PCL6 Name				 
	  "NorthAmericaLetter",    	      //XPS Name				 
	  8500,                    	      //Width in thousands of an inch	 
	  11000                   	      //Height in thousands of an inch	 
     },	
     {	
          US_LEGAL,
	  "LEGAL",                 	      //PCL6 Name				 
	  "NorthAmericaLegal",     	      //XPS Name				 
	  8500,                    	      //Width in thousands of an inch	 
	  14000                    	      //Height in thousands of an inch	 
     },					      									 
     {	
          US_EXECUTIVE,
	  "EXEC",                  	      //PCL6 Name				 
	  "NorthAmericaExecutive", 	      //XPS Name				 
	  7250,                    	      //Width in thousands of an inch	 
	  10500                    	      //Height in thousands of an inch	 
     },

#if 0
     {	
          UNKNOWN_MEDIA_SIZE,
	  "STATEMENT",             	      //PCL6 Name				 
	  "NorthAmericaStatement", 	      //XPS Name				 
	  5500,                    	      //Width in thousands of an inch	 
	  8500                     	      //Height in thousands of an inch	 
     },
     {	
          UNKNOWN_MEDIA_SIZE,
	  "8.5X13",                	      //PCL6 Name				 
	  "???????????",           	      //XPS Name				 
	  8500,                    	      //Width in thousands of an inch	 
	  13000                    	      //Height in thousands of an inch	 
     },
     {	
          UNKNOWN_MEDIA_SIZE,
	  "12X18",                 	      //PCL6 Name				 
	  "NorthAmericaArchitectureBSheet",   //XPS Name				 
	  12000,                   	      //Width in thousands of an inch	 
	  18000                    	      //Height in thousands of an inch	 
     },
#endif

     {	
          LEDGER,
	  "LEDGER",                	      //PCL6 Name				 
	  "???????????",           	      //XPS Name				 
	  11000,                   	      //Width in thousands of an inch	 
	  17000                    	      //Height in thousands of an inch	 
     },
     {	
          INDEX_CARD_3X5,
	  "3X5",                   	      //PCL6 Name				 
	  "??????????????????",               //XPS Name				 
	  3000,                   	      //Width in thousands of an inch	 
	  5000                     	      //Height in thousands of an inch	 
     },
     {		
          PHOTO_4X6,
	  "4X6",                   	      //PCL6 Name				 
	  "NorthAmerica4x6",                  //XPS Name				 
	  4000,                   	      //Width in thousands of an inch	 
	  6000                     	      //Height in thousands of an inch	 
     },
     {	
          INDEX_CARD_5X7,
	  "5X7",                   	      //PCL6 Name				 
	  "NorthAmerica5x7",                  //XPS Name				 
	  5000,                   	      //Width in thousands of an inch	 
	  7000                     	      //Height in thousands of an inch	 
     },
     {		
          INDEX_CARD_5X8,
	  "5X8",                   	      //PCL6 Name				 
	  "???????????????",                  //XPS Name				 
	  5000,                   	      //Width in thousands of an inch	 
	  8000                     	      //Height in thousands of an inch	 
     },
     {	
          ISO_AND_JIS_A3,
	  "A3",                    	      //PCL6 Name				 
	  "ISOA3",                            //XPS Name				 
	  11690,                  	      //Width in thousands of an inch	 
	  16540                    	      //Height in thousands of an inch	 
     },
     {	
          ISO_A4,
	  "A4",                    	      //PCL6 Name				 
	  "ISOA4",                            //XPS Name				 
	  8270,                   	      //Width in thousands of an inch	 
	  11690                    	      //Height in thousands of an inch	 
     },
     {	
          ISO_A5,
	  "A5",                    	      //PCL6 Name				 
	  "ISOA5",                 	      //XPS Name				 
	  5830,                    	      //Width in thousands of an inch	 
	  8270                     	      //Height in thousands of an inch	 
     },
     {	
          ISO_AND_JIS_A6,
	  "A6",                    	      //PCL6 Name				 
	  "ISOA5",                 	      //XPS Name				 
	  4130,                    	      //Width in thousands of an inch	 
	  5830                     	      //Height in thousands of an inch	 
     },
     {	
          RA3,
	  "RA3",                   	      //PCL6 Name				 
	  "????????????????",      	      //XPS Name				 
	  12010,                   	      //Width in thousands of an inch	 
	  16930                    	      //Height in thousands of an inch	 
     },
     {		
          SRA3,
	  "SRA3",                   	      //PCL6 Name				 
	  "ISOSRA3",               	      //XPS Name				 
	  12600,                   	      //Width in thousands of an inch	 
	  17720                    	      //Height in thousands of an inch	 
     },
     {		
          RA4,
	  "RA4",                    	      //PCL6 Name				 
	  "????????????????",      	      //XPS Name				 
	  8464,                    	      //Width in thousands of an inch	 
	  12007                    	      //Height in thousands of an inch	 
     },
     {	
          SRA4,
	  "SRA4",                   	      //PCL6 Name				 
	  "????????????????",      	      //XPS Name				 
	  8858,                    	      //Width in thousands of an inch	 
	  12598                    	      //Height in thousands of an inch	 
     },
     {		
          JIS_B4,
	  "JIS B4",                 	      //PCL6 Name				 
	  "JISB4",                    	      //XPS Name				 
	  10120,                   	      //Width in thousands of an inch	 
	  14330                    	      //Height in thousands of an inch	 
     },
     {	
          JIS_B5,
	  "JIS B5",                 	      //PCL6 Name				 
	  "JISB5",                    	      //XPS Name				 
	  7165,                    	      //Width in thousands of an inch	 
	  10118                    	      //Height in thousands of an inch	 
     },
     {	
          JIS_B6,
	  "JIS B6",                 	      //PCL6 Name				 
	  "JISB6",                    	      //XPS Name				 
	  5040,                    	      //Width in thousands of an inch	 
	  7170                     	      //Height in thousands of an inch	 
     },
     {	
          PHOTO_10X15,
	  "10x15 cm",               	      //PCL6 Name				 
	  "?????????????",            	      //XPS Name				 
	  4000,                    	      //Width in thousands of an inch	 
	  6000                     	      //Height in thousands of an inch	 
     },

#if 0
     {	
          UNKNOWN_MEDIA_SIZE,
	  "8K 270x390MM",           	      //PCL6 Name				 
	  "?????????????",            	      //XPS Name				 
	  10630,                   	      //Width in thousands of an inch	 
	  15350                    	      //Height in thousands of an inch	 
     },
     {	
          UNKNOWN_MEDIA_SIZE,
	  "16K 195X270MM",           	      //PCL6 Name				 
	  "?????????????",            	      //XPS Name				 
	  7680,                    	      //Width in thousands of an inch	 
	  10630                    	      //Height in thousands of an inch	 
     },
     {	
          UNKNOWN_MEDIA_SIZE,
	  "8K 260X368MM",           	      //PCL6 Name				 
	  "?????????????",            	      //XPS Name				 
	  10240,                   	      //Width in thousands of an inch	 
	  14490                    	      //Height in thousands of an inch	 
     },
     {	
          UNKNOWN_MEDIA_SIZE,
	  "16K 184X260MM",           	      //PCL6 Name				 
	  "?????????????",            	      //XPS Name				 
	  7240,                   	      //Width in thousands of an inch	 
	  10240                    	      //Height in thousands of an inch	 
     },
#endif

     {		
          ROC8K,
	  "ROC8K",                   	      //PCL6 Name				 
	  "?????????????",            	      //XPS Name				 
	  10750,                  	      //Width in thousands of an inch	 
	  15500                    	      //Height in thousands of an inch	 
     },
     {	
          ROC16K,
	  "ROC16K",                  	      //PCL6 Name				 
	  "?????????????",            	      //XPS Name				 
	  7750,                   	      //Width in thousands of an inch	 
	  10750                    	      //Height in thousands of an inch	 
     },
     {	
          JAPANESE_POSTCARD_SINGLE,
	  "JPOST",                   	      //PCL6 Name				 
	  "JapanHagakiPostCard",      	      //XPS Name				 
	  3940,                   	      //Width in thousands of an inch	 
	  5830                     	      //Height in thousands of an inch	 
     },
     {	
          JAPANESE_POSTCARD_DOUBLE,
	  "JPOSTD",                  	      //PCL6 Name				 
	  "?????????????????",        	      //XPS Name				 
	  5830,                   	      //Width in thousands of an inch	 
	  7870                     	      //Height in thousands of an inch	 
     },
     {	
          COMMERCIAL_9,
	  "COM9",                    	      //PCL6 Name				 
	  "NorthAmericaNumber9Envelope",      //XPS Name				 
	  3875,                   	      //Width in thousands of an inch	 
	  8875                     	      //Height in thousands of an inch	 
     },
     {		
          COMMERCIAL_10,
	  "COM10",                   	      //PCL6 Name				 
	  "NorthAmericaNumber10Envelope",     //XPS Name				 
	  4125,                   	      //Width in thousands of an inch	 
	  9500                     	      //Height in thousands of an inch	 
     },
     {	
          MONARCH,
	  "MONARCH",                 	      //PCL6 Name				 
	  "NorthAmericaMonarchEnvelope",      //XPS Name				 
	  3875,                   	      //Width in thousands of an inch	 
	  7500                     	      //Height in thousands of an inch	 
     },
     {	
          ISO_B5,
	  "B5 ENV",                  	      //PCL6 Name				 
	  "ISOB5Envelope",                    //XPS Name				 
	  6930,                   	      //Width in thousands of an inch	 
	  9840                     	      //Height in thousands of an inch	 
     },
     {	
          INTERNATIONAL_C5,
	  "C5",                      	      //PCL6 Name				 
	  "ISOC5Envelope",                    //XPS Name				 
	  6380,                   	      //Width in thousands of an inch	 
	  9020                     	      //Height in thousands of an inch	 
     },
     {	
          INTERNATIONAL_C6,
	  "C6",                      	      //PCL6 Name				 
	  "ISOC6Envelope",                    //XPS Name				 
	  4490,                   	      //Width in thousands of an inch	 
	  6380                     	      //Height in thousands of an inch	 
     },
     {	
          INTERNATIONAL_DL,
	  "DL",                      	      //PCL6 Name				 
	  "ISODLEnvelope",                    //XPS Name				 
	  4330,                   	      //Width in thousands of an inch	 
	  8660                     	      //Height in thousands of an inch	 
     },
     // 20120326 - LK - added support for existing types in wprint_df_types.h
     // that were missing
     {
    	  DF_MEDIA_SIZE_PHOTO_5x7_MAIN_TRAY,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  5000,                   	      //Width in thousands of an inch
    	  7000                    	      //Height in thousands of an inch
     },
     {
    	  B_TABLOID,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  11000,                   	      //Width in thousands of an inch
    	  17000                    	      //Height in thousands of an inch
     },
     {
    	  PHOTO_4X8,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  4000,                   	      //Width in thousands of an inch
    	  8000                    	      //Height in thousands of an inch
     },
     {
    	  PRINTABLE_CD_5_INCH,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  5000,                   	      //Width in thousands of an inch
    	  5000                    	      //Height in thousands of an inch
     },
     {
    	  PRINTABLE_CD_3_5_INCH,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  3300,                   	      //Width in thousands of an inch
    	  3300                    	      //Height in thousands of an inch
     },
     {
    	  PHOTO_4X12,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  4000,                   	      //Width in thousands of an inch
    	  12000                    	      //Height in thousands of an inch
     },
     {
    	  SUPER_B,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  13000,                   	      //Width in thousands of an inch
    	  19000                    	      //Height in thousands of an inch
     },
     {
    	  US_EDP,
    	  "?????????????",                //PCL6 Name
    	  "?????????????",                //XPS Name
    	  11000,                   	      //Width in thousands of an inch
    	  14000                    	      //Height in thousands of an inch
     },
     {
    	  US_GOVERNMENT_LETTER,
    	  "?????????????",         	      //PCL6 Name
    	  "?????????????",                //XPS Name
    	  4130,                   	      //Width in thousands of an inch
    	  5830                     	      //Height in thousands of an inch
     },
     {
    	  PHOTO_L_SIZE_CARD,
    	  "?????????????",                //PCL6 Name
    	  "Japan2LPhoto",                 //XPS Name
    	  3500,                   	      //Width in thousands of an inch
    	  5000                     	      //Height in thousands of an inch
     },
     {
    	  INDEX_CARD_4X6,
    	  "4x6",                      	  //PCL6 Name
    	  "NorthAmerica4x6",              //XPS Name
    	  4000,                   	      //Width in thousands of an inch
    	  6000                     	      //Height in thousands of an inch
     },
     {
    	  JPN_HAGAKI_PC,
    	  "?????????????",                //PCL6 Name
    	  "JapanHagakiPostcard",          //XPS Name
    	  3940,                   	      //Width in thousands of an inch
    	  5830                    	      //Height in thousands of an inch
     },
     {
    	  ISO_AND_JIS_A4,
    	  "A4",                    	      //PCL6 Name
    	  "ISOA4",                        //XPS Name
    	  8270,                   	      //Width in thousands of an inch
    	  11690                    	      //Height in thousands of an inch
     }
};

#endif
