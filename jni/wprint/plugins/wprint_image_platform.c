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

#include <string.h>
#include "wprint_image.h"
#include "wprint_ppm.h"
#include "wprint_jpeg.h"
#include "wprint_png.h"
#include "wprint_osimg.h"
#include "wprint_hpimage.h"
#include "mime_types.h"

const image_decode_ifc_t* wprint_image_get_decode_ifc(wprint_image_info_t *image_info)
{
    if ((image_info != NULL) && (image_info->mime_type != NULL))
    {
        if (strcasecmp(image_info->mime_type, MIME_TYPE_PPM) == 0)
            return (wprint_ppm_decode_ifc);
#ifndef EXCLUDE_JPEG
        else if (strcasecmp(image_info->mime_type, MIME_TYPE_JPEG) == 0)
            return (wprint_jpg_decode_ifc);
#endif /* EXCLUDE_JPEG */
#ifndef EXCLUDE_PNG
        else if (strcasecmp(image_info->mime_type, MIME_TYPE_PNG) == 0)
            return (wprint_png_decode_ifc);
#endif /* EXCLUDE_PNG */
        else if (strcasecmp(image_info->mime_type, MIME_TYPE_GIF) == 0)
            return (wprint_osimg_decode_ifc);
        else if (strcasecmp(image_info->mime_type, MIME_TYPE_BMP) == 0)
            return (wprint_osimg_decode_ifc);
        else if (strcasecmp(image_info->mime_type, MIME_TYPE_HPIMAGE) == 0)
            return (wprint_hpimage_decode_ifc);
        else if (strncasecmp(image_info->mime_type, MIME_TYPE_IMAGE, strlen(MIME_TYPE_IMAGE)) == 0)
            return (wprint_osimg_decode_ifc);
    }
    return(NULL);
}

int wprint_image_init( wprint_image_info_t *image_info) {
    const image_decode_ifc_t *decode_ifc = wprint_image_get_decode_ifc(image_info);

    if ((decode_ifc != NULL) && (decode_ifc->init != NULL))
    {
        decode_ifc->init(image_info);
        image_info->decode_ifc = decode_ifc;
        return(OK);
    }
    return(ERROR);
}
