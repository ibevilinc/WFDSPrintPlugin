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
#include <math.h>
#include "lib_printable_area.h"
#include "wprint_debug.h"

void printable_area_get(wprint_job_params_t *job_params,
                        float               top_margin,
                        float               left_margin,
                        float               right_margin,
                        float               bottom_margin)
{
    if (job_params == NULL)
        return;

    job_params->printable_area_width = job_params->printable_area_height = 0.0f;
    job_params->width = job_params->height = 0.0f;
    job_params->page_top_margin = job_params->page_left_margin = 0.0f;
    job_params->page_right_margin = job_params->page_left_margin = 0.0f;
    switch(job_params->media_size)
    {
        case DF_MEDIA_SIZE_A:
            job_params->page_width  = 8.5f;
            job_params->page_height = 11.0f;
            break;
        case DF_MEDIA_SIZE_LEGAL:
            job_params->page_width  = 8.5f;
            job_params->page_height = 14.0f;
            break;
        case DF_MEDIA_SIZE_A4:
            job_params->page_width  = 8.27f;
            job_params->page_height = 11.69f;
            break;
        case DF_MEDIA_SIZE_HAGAKI:
            job_params->page_width  = 3.94f;
            job_params->page_height = 5.83f;
            break;
        case DF_MEDIA_SIZE_PHOTO_10X15:
        case DF_MEDIA_SIZE_PHOTO_4X6:
            job_params->page_width  = 4.0f;
            job_params->page_height = 6.0f;
            break;
        case DF_MEDIA_SIZE_PHOTO_L:
            job_params->page_width  = 3.5f;
            job_params->page_height = 5.0f;
            break;
        case DF_MEDIA_SIZE_PHOTO_5X7:
            job_params->page_width  = 5.0f;
            job_params->page_height = 7.0f;
            break;
        case DF_MEDIA_SIZE_A6:
            job_params->page_width  = 4.13f;
            job_params->page_height = 5.83f;
            break;
        case DF_MEDIA_SIZE_PHOTO_8X10:
            job_params->page_width  = 4.13f;
            job_params->page_height = 5.83f;
            break;
        case DF_MEDIA_SIZE_PHOTO_11X14:
            job_params->page_width  = 11.0f;
            job_params->page_height = 14.0f;
            break;
        case DF_MEDIA_SIZE_SUPER_B:
            job_params->page_width  = 13.0f;
            job_params->page_height = 19.0f;
            break;
        case DF_MEDIA_SIZE_A3:
            job_params->page_width  = 11.69f;
            job_params->page_height = 16.53f;
            break;
        case DF_MEDIA_SIZE_PHOTO_4X12:
            job_params->page_width  = 4.0f;
            job_params->page_height = 12.0f;
            break;
        case DF_MEDIA_SIZE_CD_3_5_INCH:
            job_params->page_width  = 3.3f;
            job_params->page_height = 3.3f;
            break;
        case DF_MEDIA_SIZE_CD_5_INCH:
            job_params->page_width  = 5.0f;
            job_params->page_height = 5.0f;
            break;
        case DF_MEDIA_SIZE_PHOTO_4X8:
            job_params->page_width  = 4.0f;
            job_params->page_height = 8.0f;
            break;
        case DF_MEDIA_SIZE_B_TABLOID:
            job_params->page_width  = 11.0f;
            job_params->page_height = 17.0f;
            break;
        default:
            job_params->page_width  = 0.0f;
            job_params->page_height = 0.0f;
            return;
    }

    // don't adjust for margins if job is borderless and PCLm.  dimensions of image will not match (will be bigger than)
    // the dimensions of the page size and a corrupt image will render in genPCLm
    job_params->printable_area_width  = floorf(((job_params->page_width - (left_margin + right_margin)) *
									 (float)job_params->pixel_units));
    job_params->printable_area_height = floorf(((job_params->page_height - (top_margin + bottom_margin)) *
									 (float)job_params->pixel_units));

    job_params->page_top_margin    = top_margin;
    job_params->page_left_margin   = left_margin;
    job_params->page_right_margin  = right_margin;
    job_params->page_bottom_margin = bottom_margin;

    if (!job_params->borderless) {
        if (job_params->job_top_margin > top_margin) {
            job_params->print_top_margin = floorf(((job_params->job_top_margin - top_margin) * (float)job_params->pixel_units));
        }
        if (job_params->job_left_margin > left_margin) {
            job_params->print_left_margin = floorf(((job_params->job_left_margin - left_margin) * (float)job_params->pixel_units));
        }
        if (job_params->job_right_margin > right_margin) {
            job_params->print_right_margin = floorf(((job_params->job_right_margin - right_margin) * (float)job_params->pixel_units));
        }
        if (job_params->job_bottom_margin > bottom_margin) {
            job_params->print_bottom_margin = floorf(((job_params->job_bottom_margin - bottom_margin) * (float)job_params->pixel_units));
        }
    }

    job_params->width  = (job_params->printable_area_width  - (job_params->print_left_margin + job_params->print_right_margin));
    job_params->height = (job_params->printable_area_height - (job_params->print_top_margin + job_params->print_bottom_margin));

    ifprint((DBG_VERBOSE, "printable_area_get(): page dimensions: %fx%f", job_params->page_width, job_params->page_height));
    ifprint((DBG_VERBOSE, "printable_area_get(): page_top_margin=%f, page_left_margin=%f, page_right_margin=%f, page_bottom_margin=%f",
            job_params->page_top_margin,
            job_params->page_left_margin,
            job_params->page_right_margin,
            job_params->page_bottom_margin));
    ifprint((DBG_VERBOSE, "printable_area_get(): job_top_margin=%f, job_left_margin=%f, job_right_margin=%f, job_bottom_margin=%f",
            job_params->job_top_margin,
            job_params->job_left_margin,
            job_params->job_right_margin,
            job_params->job_bottom_margin));
    ifprint((DBG_VERBOSE, "printable_area_get(): printable area %dx%d @ %d dpi", job_params->printable_area_width, job_params->printable_area_height, job_params->pixel_units));
    ifprint((DBG_VERBOSE, "printable_area_get(): usalbe printable area %dx%d @ %d dpi", job_params->width, job_params->height, job_params->pixel_units));
}

void printable_area_get_default_margins(wprint_job_params_t *job_params,
                                        const printer_capabilities_t *printer_cap,
                                        float *top_margin,
                                        float *left_margin,
                                        float *right_margin,
                                        float *bottom_margin)
{
    if ((job_params == NULL) || (printer_cap == NULL)) {
        return;
    }

    bool useDefaultMargins = true;

    if (job_params->borderless)
    {
        useDefaultMargins = false;
    	switch(job_params->pcl_type) {
            case PCLm:
            case PCLPWG:
        		*top_margin    = 0.0f;
	        	*left_margin   = 0.0f;
        		*right_margin  = 0.0f;
	        	*bottom_margin = 0.00f;
                break;
            default:
        		*top_margin    = -0.065f;
	        	*left_margin   = -0.10f;
        		*right_margin  = -0.118f;
	        	*bottom_margin = -0.10f;
                break;
        }
    } else {
        switch(job_params->pcl_type) {
            case PCLm:
                *top_margin    = (float)printer_cap->printerTopMargin / 2540;
                *bottom_margin = (float)printer_cap->printerBottomMargin / 2540;
                *left_margin   = (float)printer_cap->printerLeftMargin / 2540;
                *right_margin  = (float)printer_cap->printerRightMargin / 2540;
                useDefaultMargins = false;
                break;
	    case PCLPWG:
        	*top_margin    = 0.0f;
	        *left_margin   = 0.0f;
        	*right_margin  = 0.0f;
	        *bottom_margin = 0.00f;
		 useDefaultMargins = false;
                break;
            default:
                break;
        }
    }

    if (useDefaultMargins) {
        if (!printer_cap->inkjet) {
            // default laser margins
		    *top_margin = 0.2f;
   			*left_margin = 0.25f;
   			*right_margin = 0.25f;
   			*bottom_margin = 0.2f;
        } else {
            // default inkjet margins
            switch(job_params->media_size)
            {
                case DF_MEDIA_SIZE_CD_3_5_INCH:
                case DF_MEDIA_SIZE_CD_5_INCH:
                    *top_margin    = 0.06f;
                    *left_margin   = 0.06f;
                    *right_margin  = 0.06f;
                    *bottom_margin = 0.06f;
                    break;
                case DF_MEDIA_SIZE_SUPER_B:
                    *top_margin    = 0.125f;
                    *left_margin   = 0.125f;
                    *right_margin  = 0.5f;
                    *bottom_margin = 0.75f;
                    break;
                default:
                    *top_margin    = 0.125f;
                    *left_margin   = 0.125f;
                    *right_margin  = 0.125f;
                    if ((job_params->duplex != DF_DUPLEX_MODE_NONE) || !printer_cap->canPrintBorderless) {
                        *bottom_margin = 0.5f;
                    } else {
                        *bottom_margin = 0.125f;
                    }
                    break;
            }
        }
    }

    ifprint((DBG_VERBOSE, "printable_area_get_default_margins(): top_margin=%f, left_margin=%f, right_margin=%f, bottom_margin=%f", *top_margin, *left_margin, *right_margin, *bottom_margin));

}
