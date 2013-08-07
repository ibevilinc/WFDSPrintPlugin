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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wtypes.h"
#include "wprint_ppm.h"


#define _PPM_IDENTIFIER     "P6\n"
#define _Px_STRLEN           3
#define PPM_MAX_HDR_SIZE     128


/**________________________________________________________________________
 **
 **  _get_char() :  get the next available character from the input stream
 **_______________________________________________________________________*/


unsigned char _get_char(wprint_image_info_t *image_info)
{
    unsigned char val;
    fread(&val, 1, 1, image_info->imgfile);
    return(val);
}


/**________________________________________________________________________
 **
 **  _scan_int() :  parse the PPM input stream for an unsigned integer 
 **_______________________________________________________________________*/

static unsigned int  _scan_int(wprint_image_info_t *image_info)
{
unsigned int    value = 0;
char   c;

      /*  pass over comment lines */

      while((c = _get_char(image_info)) == '#')
      {
         do 
	 {
            c = _get_char(image_info);

         } while (c != '\n');
      }

      /*  skip whitespace  */

      while(c == ' ' || c == '\t' || c == '\n' || c == '\r')
      {
         c = _get_char(image_info);
      }
      
      /*  scan the integer  */

      do
      {
	 value = value * 10 + (c - '0');
         c = _get_char(image_info);

      } while (c >= '0' && c <= '9');

      return(value);

}  /*  _scan_int  */

static void _ppm_init( wprint_image_info_t *image_info )
{
    image_info->decoder_data.ppm_info.ppm_data = NULL;
}

static int _ppm_get_hdr( wprint_image_info_t *image_info )
{
char header[_Px_STRLEN];
int found = 0;
unsigned int  max_val;

  /* Look for the PPM identifier :   P6  */
  fseek(image_info->imgfile, 0, SEEK_SET);
  fread(header, 1, _Px_STRLEN, image_info->imgfile);
  if ( strncmp((const char *) header, _PPM_IDENTIFIER, _Px_STRLEN) == 0 )
  {
      image_info->width = _scan_int(image_info);
      image_info->height = _scan_int(image_info);
      max_val = _scan_int(image_info);

      image_info->decoder_data.ppm_info.data_offset = ftell(image_info->imgfile);
                                                   /* prevent overflows */
      if (image_info->decoder_data.ppm_info.data_offset < PPM_MAX_HDR_SIZE)
      {
         found = ((image_info->width > 0)  && (image_info->height > 0) && (max_val == 255));
      }
  }	  

  if (found)
  {
     image_info->wprint_ifc->debug(DBG_LOG, "_ppm_get_hdr() : w = %d, ht = %d, offset = %d",
             image_info->width, image_info->height, image_info->decoder_data.ppm_info.data_offset );
     return(OK);
  }
  else
  {
     image_info->wprint_ifc->debug(DBG_ERROR, "ERROR: _ppm_get_hdr() : PPM not found or invalid");
     return(ERROR);
  }

}  /*  _ppm_get_hdr  */

static unsigned char* _ppm_decode_row( wprint_image_info_t *image_info,
                                      int row)
{
int rows_cached;
    if ((row < 0) || (row >= image_info->height))
        return(NULL);
    rows_cached = wprint_image_input_rows_cached(image_info);
    if ((image_info->swath_start == -1) ||
        (row < image_info->swath_start) ||
        (row >= image_info->swath_start + rows_cached))
    {
        int i, rows, position;
        if (image_info->decoder_data.ppm_info.ppm_data == NULL)
        {
            rows_cached = wprint_image_compute_rows_to_cache(image_info);
            image_info->decoder_data.ppm_info.ppm_data = (unsigned char**)malloc(sizeof(unsigned char*) * rows_cached);
            for(i = 0; i < rows_cached; i++)
            {
                image_info->decoder_data.ppm_info.ppm_data[i] = (unsigned char*)malloc(image_info->width * 3);
                if (image_info->decoder_data.ppm_info.ppm_data[i] == NULL)
                    break;
            }
            rows_cached = MIN(i, rows_cached);
        }

        image_info->swath_start = ((row / rows_cached) * rows_cached);
        rows = MIN(rows_cached, (image_info->height - image_info->swath_start));

        position = (image_info->decoder_data.ppm_info.data_offset + BYTES_PER_PIXEL(image_info->swath_start * image_info->width));
        if ((fseek(image_info->imgfile, position, SEEK_SET) == ERROR) ||
            (position != ftell(image_info->imgfile)))
            return(NULL);
        for(i = 0; i < rows; i++)
        {
            if (fread(image_info->decoder_data.ppm_info.ppm_data[i], 1, BYTES_PER_PIXEL(image_info->width), image_info->imgfile) <
                BYTES_PER_PIXEL(image_info->width))
                return(NULL);
        }
    }
    return(image_info->decoder_data.ppm_info.ppm_data[row - image_info->swath_start]);
} /* _ppm_decode_row */

static int _ppm_cleanup( wprint_image_info_t *image_info )
{
int rows_cached = wprint_image_input_rows_cached(image_info);
    if (image_info->decoder_data.ppm_info.ppm_data != NULL)
    {
        int i;
        for(i = 0; (i < rows_cached) && (image_info->decoder_data.ppm_info.ppm_data[i] != NULL); i++)
            free(image_info->decoder_data.ppm_info.ppm_data[i]);
        free(image_info->decoder_data.ppm_info.ppm_data);
        image_info->decoder_data.ppm_info.ppm_data = NULL;
    }
    return(OK);
}

static int _ppm_supports_subsampling(wprint_image_info_t *image_info)
{
    return(ERROR);
}

static const image_decode_ifc_t _ppm_decode_ifc = {
    &_ppm_init,
    &_ppm_get_hdr,
    &_ppm_decode_row,
    &_ppm_cleanup,
    &_ppm_supports_subsampling,
};

const image_decode_ifc_t *wprint_ppm_decode_ifc = &_ppm_decode_ifc;
