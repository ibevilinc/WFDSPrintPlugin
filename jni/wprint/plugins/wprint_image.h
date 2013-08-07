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
#ifndef __WPRINT_IMAGE__
#define __WPRINT_IMAGE__

#include <stdio.h>
#include "mime_types.h"
#include "wprint_scaler.h"
#include "wprint_debug.h"
#include "ifc_wprint.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BYTES_PER_RGBA(X)   ((X)*4)
#define BYTES_PER_PIXEL(X)  ((X)*3)

#define BITS_PER_CHANNEL  8
#define BITS_PER_RGBA    32
#define BITS_PER_PIXEL   24

typedef enum
{
	ROT_0 = 0,
    ROT_90,
    ROT_180,
    ROT_270,
} wprint_rotation_t;

typedef enum
{
    PAD_TOP    = (1 << 0),
    PAD_LEFT   = (1 << 1),
    PAD_RIGHT  = (1 << 2),
    PAD_BOTTOM = (1 << 3),
} wprint_padding_t;

#define PAD_NONE  (0)
#define PAD_PRINT (PAD_TOP | PAD_LEFT)
#define PAD_ALL   (PAD_TOP | PAD_LEFT | PAD_RIGHT | PAD_BOTTOM)

#define __DEFINE_WPRINT_PLATFORM_TYPES__
#include "wprint_image_platform.h"
#undef __DEFINE_WPRINT_PLATFORM_TYPES__
typedef struct
{
    // file information
	const char                          *mime_type;
	FILE                                *imgfile;

    // interfaces
    const struct _image_decode_ifc_st   *decode_ifc;
	const ifc_wprint_t                  *wprint_ifc;

    // image dimentions
	unsigned int                        width;
	unsigned int                        height;
	unsigned int                        sampled_width;
	unsigned int                        sampled_height;

    // printable area information
	unsigned int                        printable_width;
	unsigned int                        printable_height;
	unsigned int                        render_flags;

    // output information
	unsigned int                        output_width;
	unsigned int                        output_height;
	int                                 num_components;

    // memory optimization parameters
	unsigned int                        stripe_height;
	unsigned int                        concurrent_stripes;
	unsigned int                        output_rows;

    // scaling parameters
	unsigned int                        scaled_sample_size;
	unsigned int                        scaled_width;
	unsigned int                        scaled_height;
	int                                 unscaled_start_row;
	int                                 unscaled_end_row;
	unsigned int                        unscaled_rows_needed;
	unsigned char                       *unscaled_rows;
	unsigned int                        mixed_memory_needed;
	unsigned char                       *mixed_memory;
	scaler_config_t                     scaler_config;

    // padding parameters
	unsigned int                        output_padding_top;
	unsigned int                        output_padding_bottom;
	unsigned int                        output_padding_left;
	unsigned int                        output_padding_right;
    unsigned int                        padding_options;

    // decoding information
	wprint_rotation_t                   rotation;
	unsigned int                        row_offset;
	unsigned int                        col_offset;
	int                                 swath_start;
	int                                 rows_cached;
	unsigned char                       **output_cache;
	int                                 output_swath_start;
	void                                *hpimage_info;
    decoder_data_t                      decoder_data;

} wprint_image_info_t;

typedef struct _image_decode_ifc_st
{
	void (*init)(wprint_image_info_t *image_info);
	int (*get_hdr)(wprint_image_info_t *image_info);
	unsigned char* (*decode_row)(wprint_image_info_t*, int row);
	int (*cleanup)(wprint_image_info_t *image_info);
	int (*supports_subsampling)(wprint_image_info_t *image_info);
} image_decode_ifc_t;

const image_decode_ifc_t* wprint_image_get_decode_ifc(wprint_image_info_t *image_info);

void wprint_image_setup(wprint_image_info_t *image_info, const char *mime_type, const ifc_wprint_t *wprint_ifc);

int wprint_image_get_info(FILE *imgfile, wprint_image_info_t *image_info);

int wprint_image_set_output_properties(
        wprint_image_info_t *image_info,
		wprint_rotation_t rotation,
        unsigned int output_width,
		unsigned int output_height,
        unsigned int top_margin,
		unsigned int left_margin,
        unsigned int right_margin,
		unsigned int bottom_margin,
        unsigned int render_flags,
		unsigned int max_decode_stripe,
        unsigned int concurrent_stripes,
        unsigned int padding_options);

int wprint_image_is_landscape(wprint_image_info_t *image_info);

int wprint_image_get_output_buff_size(wprint_image_info_t *image_info);

int wprint_image_get_width(wprint_image_info_t *image_info);
int wprint_image_get_height(wprint_image_info_t *image_info);

int wprint_image_decode_stripe(wprint_image_info_t *image_info, int start_row,
		int *height, unsigned char *rgb_pixels);

int wprint_image_compute_rows_to_cache(wprint_image_info_t *image_info);

int wprint_image_cleanup(wprint_image_info_t *image_info);

int wprint_image_input_rows_cached(wprint_image_info_t *image_info);

#ifdef __cplusplus
}
#endif

#define __DEFINE_WPRINT_PLATFORM_METHODS__
#include "wprint_image_platform.h"
#undef __DEFINE_WPRINT_PLATFORM_METHODS__

#endif  /*  __WPRINT_IMAGE__ */
