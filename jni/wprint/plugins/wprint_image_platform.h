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
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __DEFINE_WPRINT_PLATFORM_TYPES__
#ifndef EXCLUDE_JPEG
#include "jpeglib.h"
#endif /* EXCLUDE_JPEG */
#ifndef EXCLUDE_PNG
#include "png.h"
#endif /* EXCLUDE_PNG */
#include <setjmp.h>

#ifndef EXCLUDE_JPEG
typedef struct
{
    struct jpeg_error_mgr pub;
    jmp_buf env;
} jpeg_error_handler_t;
#endif /* EXCLUDE_JPEG */

typedef struct
{
    union {
        /* PPM data */
        struct
        {
            unsigned int data_offset;
            unsigned char **ppm_data;
        } ppm_info;

#ifndef EXCLUDE_JPEG
        /* JPEG data */
        struct
        {
            struct jpeg_decompress_struct cinfo;
            jpeg_error_handler_t jerr;
            JSAMPARRAY *buff;
        } jpg_info;
#endif /* EXCLUDE_JPEG */

#ifndef EXCLUDE_PNG
        /* PNG data */
        struct
        {
            png_structp png_ptr;
            png_infop info_ptr;
            png_bytep *rowp;
        } png_info;
#endif /* EXCLUDE_PNG */

        struct
        {
            void *param;
            unsigned char **data_ptr;
        } osimg_info;
    };

} decoder_data_t;
#endif /* __DEFINE_WPRINT_PLATFORM_TYPES__ */

#ifdef __DEFINE_WPRINT_PLATFORM_METHODS__
int wprint_image_init(wprint_image_info_t *image_info);
#endif /* __DEFINE_WPRINT_PLATFORM_METHODS__ */

#ifdef __cplusplus
}
#endif
