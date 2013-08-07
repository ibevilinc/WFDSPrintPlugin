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
#include "wprint_hpimage.h"
#include "wprint_jpeg.h"
#include "wprint_png.h"
#include "wprint_ppm.h"
#include "wprint_osimg.h"

static void _hpimage_init( wprint_image_info_t *image_info )
{
    image_info->hpimage_info = NULL;
}

static int _hpimage_get_hdr( wprint_image_info_t *image_info )
{
    const image_decode_ifc_t *decode_ifc = NULL;
    int result = ERROR;
    do
    {
        if (image_info == NULL)
            continue;

#ifndef EXCLUDE_JPEG
        decode_ifc = wprint_jpg_decode_ifc;
        if (decode_ifc != NULL)
        {

            if (decode_ifc->init != NULL)
                decode_ifc->init(image_info);
            if (decode_ifc->get_hdr != NULL)
                result = decode_ifc->get_hdr(image_info);
            if (result == OK)
                continue;
            if (decode_ifc->cleanup != NULL)
                decode_ifc->cleanup(image_info);
        }
#endif /* EXCLUDE_JPEG */

#ifndef EXCLUDE_PNG
        decode_ifc = wprint_png_decode_ifc;
        if (decode_ifc != NULL)
        {
            if (decode_ifc->init != NULL)
                decode_ifc->init(image_info);
            if (decode_ifc->get_hdr != NULL)
                result = decode_ifc->get_hdr(image_info);
            if (result == OK)
                continue;
            if (decode_ifc->cleanup != NULL)
                decode_ifc->cleanup(image_info);
        }
#endif /* EXCLUDE_PNG */

        decode_ifc = wprint_ppm_decode_ifc;
        if (decode_ifc != NULL)
        {
            if (decode_ifc->init != NULL)
                decode_ifc->init(image_info);
            if (decode_ifc->get_hdr != NULL)
                result = decode_ifc->get_hdr(image_info);
            if (result == OK)
                continue;
            if (decode_ifc->cleanup != NULL)
                decode_ifc->cleanup(image_info);
        }

        decode_ifc = wprint_osimg_decode_ifc;
        if (decode_ifc != NULL)
        {
            if (decode_ifc->init != NULL)
                decode_ifc->init(image_info);
            if (decode_ifc->get_hdr != NULL)
                result = decode_ifc->get_hdr(image_info);
            if (result == OK)
                continue;
            if (decode_ifc->cleanup != NULL)
                decode_ifc->cleanup(image_info);
        }

        decode_ifc = NULL;

    } while(0);

    if (image_info != NULL)
        image_info->hpimage_info = (void*)decode_ifc;
    
    return(result);
}

static unsigned char* _hpimage_decode_row( wprint_image_info_t *image_info,
                                      int row)
{
    const image_decode_ifc_t *decode_ifc = (const image_decode_ifc_t*)image_info->hpimage_info;

    if ((decode_ifc == NULL) ||
        (decode_ifc->decode_row == NULL))
        return(NULL);
    
    return(decode_ifc->decode_row(image_info, row));
} /* _ppm_decode_row */

static int _hpimage_cleanup( wprint_image_info_t *image_info )
{
    const image_decode_ifc_t *decode_ifc = (const image_decode_ifc_t*)image_info->hpimage_info;

    if ((decode_ifc == NULL) ||
        (decode_ifc->cleanup == NULL))
        return(ERROR);

    image_info->hpimage_info = NULL;
    return(decode_ifc->cleanup(image_info));
}

static int _hpimage_supports_subsampling(wprint_image_info_t *image_info)
{
    const image_decode_ifc_t *decode_ifc = (const image_decode_ifc_t*)image_info->hpimage_info;

    if ((decode_ifc == NULL) ||
        (decode_ifc->supports_subsampling == NULL))
        return(ERROR);

    return(decode_ifc->supports_subsampling(image_info));
}

static const image_decode_ifc_t _hpimage_decode_ifc = {
    &_hpimage_init,
    &_hpimage_get_hdr,
    &_hpimage_decode_row,
    &_hpimage_cleanup,
    &_hpimage_supports_subsampling,
};

const image_decode_ifc_t *wprint_hpimage_decode_ifc = &_hpimage_decode_ifc;
