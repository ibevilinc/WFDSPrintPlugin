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
#include "wprint_osimg.h"

static void _osimg_init( wprint_image_info_t *image_info )
{
    image_info->decoder_data.osimg_info.param    = NULL;
    image_info->decoder_data.osimg_info.data_ptr = NULL;
}

static int _osimg_get_hdr( wprint_image_info_t *image_info )
{
    int result = ERROR;
    const ifc_osimg_t* osimg_ifc = image_info->wprint_ifc->get_osimg_ifc();
    do {
        if (osimg_ifc == NULL) {
            continue;
        }

        int width, height;
        width = height = -1;
        result = osimg_ifc->decode_bounds(image_info->imgfile, &width, &height);

        if ((result == OK) && (width > 0) && (height > 0)) {
            image_info->width  = width;
            image_info->height = height;
            image_info->num_components = 3;
        } else {
            result = ERROR;
        }
        
    } while(0);
    return(result);
}

static unsigned char* _osimg_decode_row( wprint_image_info_t *image_info, int row)
{
    const ifc_osimg_t* osimg_ifc = image_info->wprint_ifc->get_osimg_ifc();
    int rows_cached = wprint_image_input_rows_cached(image_info);
    if (osimg_ifc == NULL) {
        return NULL;
    }

    int i, rows;
    if ((image_info->swath_start == -1) ||
        (row < image_info->swath_start) ||
        (row >= image_info->swath_start + rows_cached)) {

        if (image_info->decoder_data.osimg_info.data_ptr == NULL)
        {
            if (image_info->decoder_data.osimg_info.param == NULL) {
                image_info->decoder_data.osimg_info.param = osimg_ifc->decode_start(image_info->imgfile, image_info->scaled_sample_size);
                if (image_info->decoder_data.osimg_info.param == NULL) {
                    return NULL;
                }
            }

            rows_cached = wprint_image_compute_rows_to_cache(image_info);
            image_info->decoder_data.osimg_info.data_ptr = (unsigned char**)malloc(sizeof(unsigned char*) * rows_cached);
            for(i = 0; i < rows_cached; i++)
            {
               image_info->decoder_data.osimg_info.data_ptr[i] = (unsigned char*)malloc(3 * osimg_ifc->get_width(image_info->decoder_data.osimg_info.param));
                if (image_info->decoder_data.osimg_info.data_ptr[i] == NULL) {
                    break;
                }
            }

            rows_cached = MIN(i, rows_cached);
        }
        if (rows_cached == 0)
        {
            return NULL;
        }

        image_info->swath_start = ((row / rows_cached) * rows_cached);
        rows = MIN(rows_cached, (image_info->height - image_info->swath_start));

        for(i = 0; i < rows; i++) {
            if (osimg_ifc->decode_row(image_info->decoder_data.osimg_info.param, image_info->swath_start + i, image_info->decoder_data.osimg_info.data_ptr[i]) != OK) {
                return NULL;
            }
        }
    }

    return(image_info->decoder_data.osimg_info.data_ptr[row - image_info->swath_start]);
} /* _osimg_decode_row */

static int _osimg_cleanup( wprint_image_info_t *image_info )
{
    const ifc_osimg_t* osimg_ifc = image_info->wprint_ifc->get_osimg_ifc();
    do {
        if (osimg_ifc == NULL) {
            continue;
        }

        if (image_info->decoder_data.osimg_info.data_ptr != NULL) {
            int i;
            int rows_cached = wprint_image_input_rows_cached(image_info);
            for(i = 0; i < rows_cached; i++)
                free(image_info->decoder_data.osimg_info.data_ptr[i]);
            free(image_info->decoder_data.osimg_info.data_ptr);
        }

        osimg_ifc->decode_end(image_info->decoder_data.osimg_info.param);
        image_info->decoder_data.osimg_info.param = NULL;
        image_info->decoder_data.osimg_info.data_ptr = NULL;
    } while(0);

    return(OK);
}

static int _osimg_supports_subsampling(wprint_image_info_t *image_info)
{
    int result = ERROR;
    const ifc_osimg_t* osimg_ifc = image_info->wprint_ifc->get_osimg_ifc();
    do {
        if (osimg_ifc == NULL) {
            continue;
        }

        result = osimg_ifc->can_subsample();

    } while(0);
    return(result);
}

static const image_decode_ifc_t _osimg_decode_ifc = {
    &_osimg_init,
    &_osimg_get_hdr,
    &_osimg_decode_row,
    &_osimg_cleanup,
    &_osimg_supports_subsampling,
};

const image_decode_ifc_t *wprint_osimg_decode_ifc = &_osimg_decode_ifc;
