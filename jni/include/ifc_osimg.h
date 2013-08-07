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
#ifndef __IFC_OSIMG_H__
#define __IFC_OSIMG_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ifc_osimg_st
{
    int (*decode_bounds)(FILE *imgfile, int *width, int *height);
    int (*can_subsample)(void);
    void* (*decode_start)(FILE *imgfile, int subsample);
    int (*get_width)(void *decodeParam);
    int (*get_height)(void *decodeParam);
    int (*decode_row)(void *decodeParam, int row, unsigned char *rowBuf);
    void (*decode_end)(void* decodeParam);
} ifc_osimg_t;

#ifdef __cplusplus
}
#endif

#endif /* __IFC_OSIMG_H__ */
