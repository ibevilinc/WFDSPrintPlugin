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
#ifndef __MIME_TYPES_H__
#define __MIME_TYPES_H__


/**  input MIME types  **/

#define MIME_TYPE_WORD    "application/msword"
#define MIME_TYPE_PPT     "application/powerpoint"
#define MIME_TYPE_EXCEL   "application/excel"
#define MIME_TYPE_PDF     "application/pdf"
#define MIME_TYPE_PS      "application/postscript"
#define MIME_TYPE_PCLM    "application/PCLm"

#define MIME_TYPE_TXT     "text/plain"
#define MIME_TYPE_HTML    "text/html"

#define MIME_TYPE_GIF     "image/gif"
#define MIME_TYPE_BMP     "image/bmp"
#define MIME_TYPE_JPEG    "image/jpeg"
#define MIME_TYPE_TIFF    "image/tiff"
#define MIME_TYPE_PNG     "image/png"
#define MIME_TYPE_PPM     "image/x-portable-pixmap"
#define MIME_TYPE_HPIMAGE "image/hpimage"
#define MIME_TYPE_IMAGE   "image/"
#define MIME_TYPE_PWG     "image/pwg-raster"
#define MIME_TYPE_IMAGE_ALL "image/*"
typedef enum
{
    INPUT_MIME_TYPE_PPM = 0,
    INPUT_MIME_TYPE_JPEG,
    INPUT_MIME_TYPE_PNG,
    INPUT_MIME_TYPE_PDF,
    INPUT_MIME_TYPE_WORD,
    INPUT_MIME_TYPE_PPT,
    INPUT_MIME_TYPE_EXCEL,
    INPUT_MIME_TYPE_PCLM,

    INPUT_MIME_TYPE_TEXT,
    INPUT_MIME_TYPE_HTML,
    INPUT_MIME_TYPE_TIFF,
    INPUT_MIME_TYPE_GIF,
    INPUT_MIME_TYPE_BMP,
    INPUT_MIME_TYPE_HPIMAGE,
    INPUT_MIME_TYPE_IMAGE_GENERIC,
} input_mime_types_t;


/**  output print data formats  **/


#define PRINT_FORMAT_PCLM       MIME_TYPE_PCLM
#define PRINT_FORMAT_PDF        MIME_TYPE_PDF
#define PRINT_FORMAT_JPEG       MIME_TYPE_JPEG
#define PRINT_FORMAT_PS         MIME_TYPE_PS
#define PRINT_FORMAT_AUTO        "application/octet-stream"
#define PRINT_FORMAT_PWG	MIME_TYPE_PWG

#endif   /* __MIME_TYPES_H__ */
