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
#ifndef EXCLUDE_PNG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "png.h"

#include "wtypes.h"
#include "wprint_png.h"
#include "wprint_image.h"

static void _png_init( wprint_image_info_t   *image_info )
{
    image_info->decoder_data.png_info.rowp = NULL;
    image_info->decoder_data.png_info.png_ptr = NULL;
}

static int _png_get_hdr( wprint_image_info_t   *image_info )
{
unsigned char header[8];  
memset(header, 0,sizeof(header));
int bit_depth, pixel_depth, number_of_passes;

   image_info->decoder_data.png_info.png_ptr = NULL;
   image_info->decoder_data.png_info.info_ptr = NULL;

   fseek(image_info->imgfile, 0, SEEK_SET);
   int header_size = fread(header, 1, sizeof(header), image_info->imgfile);
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): header_size: %d", header_size);

   int pngsig = png_sig_cmp(header, 0, sizeof(header));
   if (pngsig)
   {
      image_info->wprint_ifc->debug(DBG_ERROR, "_png_get_hdr(): ERROR: not a PNG file");
      return(ERROR);
   }
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): png_sig_cmp() returned %d", pngsig);

   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): PNG_LIBPNG_VER_STRING: %s", PNG_LIBPNG_VER_STRING);
   /* initialize stuff */
   image_info->decoder_data.png_info.png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
                                                NULL, NULL, NULL);
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): png_create_read_struct() returned %d", image_info->decoder_data.png_info.png_ptr);

   if (!(image_info->decoder_data.png_info.png_ptr))
   {
      image_info->wprint_ifc->debug(DBG_ERROR, "_png_get_hdr(): ERROR: png_create_read_struct failed");
      return(ERROR);
   }

   image_info->decoder_data.png_info.info_ptr = png_create_info_struct(image_info->decoder_data.png_info.png_ptr);
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): png_create_info_struct() returned %d", image_info->decoder_data.png_info.info_ptr);
   if (!(image_info->decoder_data.png_info.info_ptr))
   {
      image_info->wprint_ifc->debug(DBG_ERROR, "_png_get_hdr(): ERROR: png_create_info_struct failed");
      return(ERROR);
   }

   if (setjmp(png_jmpbuf(image_info->decoder_data.png_info.png_ptr)))
   {
      image_info->wprint_ifc->debug(DBG_ERROR, "_png_get_hdr(): ERROR: invalid or corrupt PNG");
      return(ERROR);
   }

   png_init_io(image_info->decoder_data.png_info.png_ptr, image_info->imgfile);
   png_set_sig_bytes(image_info->decoder_data.png_info.png_ptr, sizeof(header));

   png_read_info(image_info->decoder_data.png_info.png_ptr, image_info->decoder_data.png_info.info_ptr);
   number_of_passes = png_set_interlace_handling(image_info->decoder_data.png_info.png_ptr);
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): png_set_interlace_handling() returned %d", number_of_passes);

   // setup background color as white
   png_color_16 bg_color;
   bg_color.index = 0;
   bg_color.red   = (png_uint_16)-1;
   bg_color.green = (png_uint_16)-1;
   bg_color.blue  = (png_uint_16)-1;
   bg_color.gray  = (png_uint_16)-1;
   // blend alpha using white background color instead of stripping alpha channel
   png_set_background(image_info->decoder_data.png_info.png_ptr, &bg_color, PNG_BACKGROUND_GAMMA_FILE, 0, 1.0);
//   png_set_strip_alpha(image_info->decoder_data.png_info.png_ptr);
   png_set_expand(image_info->decoder_data.png_info.png_ptr);
   png_set_strip_16(image_info->decoder_data.png_info.png_ptr);
   png_set_gray_to_rgb(image_info->decoder_data.png_info.png_ptr);
   png_read_update_info(image_info->decoder_data.png_info.png_ptr, image_info->decoder_data.png_info.info_ptr);

   image_info->width = png_get_image_width(image_info->decoder_data.png_info.png_ptr, 
                                           image_info->decoder_data.png_info.info_ptr);
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): png_get_image_width() returned %d", image_info->width);
   image_info->height = png_get_image_height(image_info->decoder_data.png_info.png_ptr, 
                                             image_info->decoder_data.png_info.info_ptr);
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): png_get_image_height() returned %d", image_info->height);

   bit_depth = png_get_bit_depth(image_info->decoder_data.png_info.png_ptr, 
                                 image_info->decoder_data.png_info.info_ptr);
   image_info->wprint_ifc->debug(DBG_VERBOSE, "_png_get_hdr(): png_get_bit_depth() returned %d", bit_depth);
   pixel_depth = image_info->decoder_data.png_info.info_ptr->pixel_depth;

   if ( (bit_depth == BITS_PER_CHANNEL) && (pixel_depth == BITS_PER_PIXEL) )
   {
      image_info->num_components = pixel_depth/bit_depth;
      image_info->wprint_ifc->debug(DBG_LOG, "_png_get_hdr(): PNG %d x %d (%d components)",
             image_info->width, image_info->height, 
             image_info->num_components);
      return(OK);
   }
   else
   {
      image_info->wprint_ifc->debug(DBG_ERROR, "_png_get_hdr(): ERROR: PNG format not supported. pix = %d, bit = %d",
              pixel_depth, bit_depth);
      return(ERROR);
   }
}  /*  static wprint_png_get_hdr  */

static unsigned char* _png_decode_row( wprint_image_info_t *image_info,
                                       int                 row )
{
int i, rows;
int start_row;
int rows_cached;
    if ((row < 0) || (row >= image_info->sampled_height))
        return(NULL);

    rows_cached = wprint_image_input_rows_cached(image_info);
    if ((image_info->swath_start == -1) ||
        (row < image_info->swath_start) ||
        (row >= (image_info->swath_start + rows_cached)))
    {
        start_row = ((image_info->swath_start < 0) ? 0 : (image_info->swath_start + rows_cached));
        if (image_info->decoder_data.png_info.rowp == NULL)
        {
            rows_cached = wprint_image_compute_rows_to_cache(image_info);
            image_info->decoder_data.png_info.rowp = (png_bytep*)malloc(sizeof(png_bytep) * rows_cached);
            for(i = 0; i < rows_cached; i++)
            {
               image_info->decoder_data.png_info.rowp[i] = (png_bytep)malloc(png_get_rowbytes(image_info->decoder_data.png_info.png_ptr,image_info->decoder_data.png_info.info_ptr));
               if (image_info->decoder_data.png_info.rowp[i] == NULL)
                   break;
	    }
            rows_cached = MIN(i, rows_cached);
        }
        else if ((image_info->swath_start != -1) && (row < image_info->swath_start))
        {
            start_row = 0;
            png_destroy_read_struct(&image_info->decoder_data.png_info.png_ptr, &image_info->decoder_data.png_info.info_ptr, (png_infopp) NULL);
            if (_png_get_hdr(image_info) == ERROR)
                return(NULL);
        }
        if (rows_cached == 0)
            return(NULL);
        image_info->swath_start = ((row / rows_cached) * rows_cached);
        if (setjmp(png_jmpbuf(image_info->decoder_data.png_info.png_ptr)))
        {
           image_info->wprint_ifc->debug(DBG_ERROR, "ERROR: PNG read error or corrupt file");
           return(NULL);
        }
        for(i = start_row; i <= image_info->swath_start; i += rows_cached)
        {
            rows = MIN(rows_cached, (image_info->height - i));
            png_read_rows(image_info->decoder_data.png_info.png_ptr, NULL, image_info->decoder_data.png_info.rowp, rows);
        }
    }
    return(image_info->decoder_data.png_info.rowp[row - image_info->swath_start]);
} /* _png_decode_row */

static int _png_cleanup( wprint_image_info_t    *image_info )
{
    int i, rows_cached;
    if (image_info->decoder_data.png_info.rowp != NULL)
    {
        rows_cached = wprint_image_input_rows_cached(image_info);
        for (i = 0; (i < rows_cached) && (image_info->decoder_data.png_info.rowp[i] != NULL); i++)
            free(image_info->decoder_data.png_info.rowp[i]);
        free(image_info->decoder_data.png_info.rowp);
    }
    image_info->decoder_data.png_info.rowp = NULL;
    //png_read_end(image_info->png_ptr, image_info->info_ptr);
    if (image_info->decoder_data.png_info.png_ptr != NULL)
        png_destroy_read_struct(&(image_info->decoder_data.png_info.png_ptr), 
                                &(image_info->decoder_data.png_info.info_ptr),
                                (png_infopp) NULL);
    image_info->decoder_data.png_info.png_ptr = NULL;
    return(OK);

}  /*  _png_cleanup  */

static int _png_supports_subsampling(wprint_image_info_t *image_info)
{
    return(ERROR);
}

static const image_decode_ifc_t _png_decode_ifc = {
    &_png_init,
    &_png_get_hdr,
    &_png_decode_row,
    &_png_cleanup,
    &_png_supports_subsampling,
};

const image_decode_ifc_t *wprint_png_decode_ifc = &_png_decode_ifc;

#endif /* EXCLUDE_PNG */
