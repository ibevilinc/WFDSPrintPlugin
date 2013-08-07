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
#ifndef EXCLUDE_JPEG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wtypes.h"
#include "jpeglib.h"
#include "wprint_image.h"

static void _jpeg_error_exit(j_common_ptr cinfo)
{
    jpeg_error_handler_t *handler = (jpeg_error_handler_t*)cinfo->err;
    (handler->pub.output_message)(cinfo);
    longjmp(handler->env, 1);
}

static void _jpeg_output_message(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    jpeg_error_handler_t *handler = (jpeg_error_handler_t*)cinfo->err;
    (handler->pub.format_message)(cinfo, buffer);
    wprint_image_info_t* image_info = (wprint_image_info_t*)cinfo->client_data;
    image_info->wprint_ifc->debug(DBG_ERROR, buffer);
}

static void _jpeg_init( wprint_image_info_t   *image_info )
{
    image_info->decoder_data.jpg_info.buff = NULL;
}

static int _jpeg_get_hdr( wprint_image_info_t   *image_info )
{
   struct jpeg_decompress_struct *cinfo = &(image_info->decoder_data.jpg_info.cinfo);

   /*  initialize the JPEG decompression object and specify source file  */
   fseek(image_info->imgfile, 0, SEEK_SET);

   cinfo->err = jpeg_std_error(&image_info->decoder_data.jpg_info.jerr.pub);
   image_info->decoder_data.jpg_info.jerr.pub.error_exit = _jpeg_error_exit;
   image_info->decoder_data.jpg_info.jerr.pub.output_message = _jpeg_output_message;

   if (setjmp(image_info->decoder_data.jpg_info.jerr.env))
   {
       image_info->wprint_ifc->debug(DBG_ERROR, "ERROR: invalid or corrupt JPEG");
       return(ERROR);
   }

   jpeg_create_decompress(cinfo);
   cinfo->client_data = (void*)image_info;
   //cinfo->mem->max_memory_to_use = 30 * 1024 * 1024;
   jpeg_stdio_src(cinfo, image_info->imgfile);

   jpeg_read_header(cinfo, true);

   image_info->width = cinfo->image_width;
   image_info->height = cinfo->image_height;

   cinfo->scale_num = 1;
   cinfo->scale_denom = image_info->scaled_sample_size;

   jpeg_calc_output_dimensions(cinfo);

   image_info->wprint_ifc->debug(DBG_VERBOSE, 
          "_jpeg_get_hdr(): jpeg DIM %d x %d => %d x %d, #components = %d",
          image_info->width, image_info->height,
          cinfo->output_width, cinfo->output_height,
          cinfo->output_components);

   image_info->num_components = cinfo->output_components;
   cinfo->out_color_space = JCS_RGB;

   jpeg_start_decompress(cinfo);

   if (cinfo->output_components == BYTES_PER_PIXEL(1))
   {
       return(OK);
   }
   else
   {
       image_info->wprint_ifc->debug(DBG_ERROR, "ERROR: JPG format not supported. #components = %d",
               cinfo->output_components);
       return(ERROR);
   }
}  /*  _jpeg_get_hdr  */

static unsigned char* _jpeg_decode_row( wprint_image_info_t *image_info,
                                       int                 row )
{
int i, j, rows;
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
        if (image_info->decoder_data.jpg_info.buff == NULL)
        {
            rows_cached = wprint_image_compute_rows_to_cache(image_info);
            image_info->decoder_data.jpg_info.buff = (JSAMPARRAY*)malloc(sizeof(JSAMPARRAY) * rows_cached);
            for(i = 0; i < rows_cached; i++)
            {
                image_info->decoder_data.jpg_info.buff[i] = NULL;
                image_info->decoder_data.jpg_info.buff[i] = (JSAMPARRAY)
                    (*image_info->decoder_data.jpg_info.cinfo.mem->alloc_sarray) (
                          (j_common_ptr) &image_info->decoder_data.jpg_info.cinfo, 
                          JPOOL_IMAGE, 
                          BYTES_PER_PIXEL(image_info->decoder_data.jpg_info.cinfo.output_width),
                          1);
                if (image_info->decoder_data.jpg_info.buff[i] == NULL)
                    break;
            }
            rows_cached = MIN(i, rows_cached);
        }
        else if ((image_info->swath_start != -1) && (row < image_info->swath_start))
        {
            start_row = 0;
            jpeg_abort((void *) &image_info->decoder_data.jpg_info.cinfo);
            jpeg_destroy_decompress(&image_info->decoder_data.jpg_info.cinfo);
            if (_jpeg_get_hdr(image_info) == ERROR)
                return(NULL);
            for(i = 0; i < rows_cached; i++)
            {
                image_info->decoder_data.jpg_info.buff[i] = (JSAMPARRAY)
                    (*image_info->decoder_data.jpg_info.cinfo.mem->alloc_sarray) (
                          (j_common_ptr) &image_info->decoder_data.jpg_info.cinfo, 
                          JPOOL_IMAGE, 
                          BYTES_PER_PIXEL(image_info->decoder_data.jpg_info.cinfo.output_width),
                          1);
                if (image_info->decoder_data.jpg_info.buff[i] == NULL)
                    break;
            }
            rows_cached = MIN(i, rows_cached);
        }
        if (rows_cached == 0)
            return(NULL);
        if (setjmp(image_info->decoder_data.jpg_info.jerr.env))
        {
            image_info->wprint_ifc->debug(DBG_ERROR, "ERROR: JPEG read error or corrupt file");
            return(NULL);
        }
        image_info->swath_start = ((row / rows_cached) * rows_cached);
        for(i = start_row; i <= image_info->swath_start; i += rows_cached)
        {
            rows = MIN(rows_cached, (image_info->sampled_height - i));
            for(j = 0; j < rows; j++)
            {
                jpeg_read_scanlines(&image_info->decoder_data.jpg_info.cinfo,
                                        image_info->decoder_data.jpg_info.buff[j],
                                        1);
                if (image_info->decoder_data.jpg_info.jerr.pub.num_warnings != 0)
                {
                    image_info->wprint_ifc->debug(DBG_ERROR, "ERROR: JPEG scanline error");
                    return(NULL);
                }
            }
        }
    }
    
    return((unsigned char*)image_info->decoder_data.jpg_info.buff[row - image_info->swath_start][0]);
} /* _jpeg_decode_row */

static int _jpeg_cleanup( wprint_image_info_t   *image_info )
{
    jpeg_abort((void *) &image_info->decoder_data.jpg_info.cinfo);
    jpeg_destroy_decompress(&image_info->decoder_data.jpg_info.cinfo);
    if (image_info->decoder_data.jpg_info.buff != NULL)
        free(image_info->decoder_data.jpg_info.buff);
    image_info->decoder_data.jpg_info.buff = NULL;
    return(OK);

}  /*  _jpeg_cleanup  */

static int _jpeg_supports_subsampling(wprint_image_info_t *image_info)
{
    return(OK);
}

static const image_decode_ifc_t _jpg_decode_ifc = {
    &_jpeg_init,
    &_jpeg_get_hdr,
    &_jpeg_decode_row,
    &_jpeg_cleanup,
    &_jpeg_supports_subsampling,
};

const image_decode_ifc_t *wprint_jpg_decode_ifc = &_jpg_decode_ifc;

#endif /* EXCLUDE_JPEG */
