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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>

#include "ifc_wprint.h"
#include "wprint_df_types.h"
#include "wprint_debug.h"
#include "mime_types.h"
#include "lib_wprint.h"
#include "lib_printable_area.h"
#include "wprint_image.h"

#ifndef EXCLUDE_PCLM
#include "pclm_wrapper_api.h"

#define  __INCLUDE_MEDIA_H__
#include "media2.h"
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
#include <cups/raster.h>
#endif /* NO_CUPS */

#define PPM_COLOR_IDENTIFIER "P6"
#define PPM_GRAYSCALE_IDENTIFIER "P5"
#define PPM_HEADER_LENGTH 128

#define SP_GRAY(Yr,Cbg,Crb) (((Yr<<6) + (Cbg*160) + (Crb<<5)) >> 8)

enum {
    OPTION_START = 500,
    OPTION_ROTATION,
    OPTION_MEDIA_SIZE,
    OPTION_EXTRA_MARGIN_LEFT,
    OPTION_EXTRA_MARGIN_RIGHT,
    OPTION_EXTRA_MARGIN_TOP,
    OPTION_EXTRA_MARGIN_BOTTOM,
    OPTION_MONOCHROME,
};

typedef enum {
    OUTPUT_PPM,
    OUTPUT_PDF,
    OUTPUT_PWG,
} output_format_t;


#define DEBUG_MSG_SIZE (2048)
static void _debug(int cond, char *fmt, ...)
{
    va_list args;
    char *buff = malloc(DEBUG_MSG_SIZE);
    if (buff != NULL)
    {
        va_start(args, fmt);
        vsnprintf(buff, DEBUG_MSG_SIZE, fmt, args);
        va_end(args);
        ifprint((cond, buff));
        free(buff);
    }
}

static const ifc_osimg_t* getOSIFC() {
    return NULL;
}

static const ifc_wprint_debug_stream_t* getDebugStreamIFC(wJob_t handle) {
    return NULL;
}

static const ifc_wprint_t _wprint_ifc =
{
    .debug                = _debug,
    .msgQCreate           = msgQCreate,
    .msgQDelete           = msgQDelete,
    .msgQSend             = msgQSend,
    .msgQReceive          = msgQReceive,
    .msgQNumMsgs          = msgQNumMsgs,
    .get_osimg_ifc        = getOSIFC, 
    .get_debug_stream_ifc = getDebugStreamIFC,
};

#ifndef EXCLUDE_PCLM
static void _get_pclm_media_size_name(media_size_t media_size, char *media_name)
{
	int mediaNumEntries = sizeof(MasterMediaSizeTable2)
			/ sizeof(struct MediaSizeTableElement2);
	int i = 0;

	for (i = 0; i < mediaNumEntries; i++)
	{
		if (media_size == MasterMediaSizeTable2[i].media_size)
		{
		    strncpy(media_name, MasterMediaSizeTable2[i].PCL6Name,
            						sizeof(media_name) - 1);
			break;  // we found a match, so break out of loop
		}
	}

	if (i == mediaNumEntries)
	{
		// media size not found, defaulting to letter
		ifprint(
				(DBG_VERBOSE, "   _get_pclm_media_size_name(): media size, %d, NOT FOUND, setting to letter", media_size));
		_get_pclm_media_size_name(US_LETTER, media_name);
	}

}
#endif /* EXCLUDE_PCLM */


static void _print_usage(char *argv0)
{
    char *ptr;
    if ( (ptr = strrchr(argv0, '/')) != NULL ) {
        printf("\nUsage : %s <options>\n\n", ptr+1);
    }
    else {
       printf("\nUsage : %s <options>\n\n", argv0);
    }

    printf("          Option      \t\t      Description\n");
    printf("  ===========================================================================================\n\n");
    printf("       -i  --input=PATHNAME\t\t|  Input pathname\n");
    printf("       -o  --output=PATHNAME\t\t|  Output pathname\n");
    printf("       -d  --debug=#\t\t\t|  debug level mask\n");
    printf("       -l  --log-file=PATHNAME\t\t|  Redirect debug log\n");
    printf("       -b  --borderless\t\t\t|  enable single borderless JPEG print\n");
    printf("       -f  --render-flags=# \t\t|  rendering flag requests\n");
    printf("                            \t\t|  1: AUTO_ROTATE     ,  2: CENTER_VERT         ,  4: CENTER_HORZ\n");
    printf("                            \t\t|  8: ROTATE_BACK_PAGE, 16: BACK_PAGE_PREROTATED, 32: AUTO_SCALE\n");
    printf("                            \t\t|  64: AUTO_FIT, 128: PORTRAIT, 256: LANDSCAPE\n");
    printf("                            \t\t|  512: CENTER_ON_ORIENTATION\n");
    printf("                            \t\t|  (AUTO_SCALE 1+2+4+32 = 39)\n");
    printf("                            \t\t|  (AUTO_FIT 1+64+512 = 577)\n");
    printf("       -m  --media-size=# \t\t|  2: LETTER, 3: LEGAL, 26: A4, 4x6: 74. 5x7:122, HAGAKI: 71, ...\n");
    printf("       -w  --printable-width=#\t\t|  Printable width to render to\n");
    printf("       -h  --printable-height=#\t\t|  Printable height to render to\n");
    printf("       -s  --stripe-height=#\t\t|  Number of rows to render at once\n");
    printf("       -c  --concurrent-stripes#\t|  Number of rows sets we want to render at once\n");
    printf("       -r  --rotation=#\t\t\t|  Specify rotation to use\n");
    printf("       -2  --duplex=#\t\t\t|  Duplex mode to use\n");
    printf("       -p  --pcl-type=#\t\t\t|  PCL Type to use, used in conjunction with media-size\n");
    printf("                            \t\t|  0: PPM\n");
#ifndef EXCLUDE_PCLM
    printf("                            \t\t|  1: PDF\n");
#endif /* EXCLUDE_PCLM */
#ifndef NO_CUPS
    printf("                            \t\t|  2: PWG\n");
#endif /* NO_CUPS */
    printf("       -z  --laser-jet\t\t\t|  Assume printer is a LaserJet, used in conjunction with media-size\n");
    printf("       -x  --resolution=#\t\t|  Print resolution to use\n");
    printf("       -u  --page-backside\t\t|  Used to render as if printing the backside of a page when no rotation is specified\n");
    printf("           --extra-margin-top=#\t\t|  Additional top margin in inches to apply\n");
    printf("           --extra-margin-left=#\t|  Additional left margin in inches to apply\n");
    printf("           --extra-margin-right=#\t|  Additional right margin in inches to apply\n");
    printf("           --extra-margin-bottom=#\t|  Additional bottom margin in inches to apply\n");
    printf("       -e  --white-padding=#\t\t|  White padding to put around image\n");
    printf("                            \t\t|  1: TOP, 2: LEFT, 4: RIGHT, 8: BOTTOM\n");
    printf("                            \t\t|  (NONE   0          )\n");
    printf("                            \t\t|  (PRINT  3 = 1+2    )\n");
    printf("                            \t\t|  (ALL   15 = 1+2+4+8)\n");
    printf("           --mono\t\t\t|  Monochrome\n");
#ifndef EXCLUDE_PCLM
    printf("       -v  --pclm-test-mode\t\t|  PCLm test mode\n");
#endif /* EXCLUDE_PCLM */
    printf("       -?  --help\t\t\t|  print help text\n\n");
}  /*  _print_usage  */

static void _write_header(FILE *outputfile, int width, int height, bool grayscale) {
    // add 1 for terminating null
    char ppm_header[PPM_HEADER_LENGTH + 1];
    if (outputfile != NULL) {
        fseek(outputfile, 0, SEEK_SET);
        int length = snprintf(ppm_header, sizeof(ppm_header), "%s\n#%*c\n%d %d\n%d\n",
                              grayscale ? PPM_GRAYSCALE_IDENTIFIER : PPM_COLOR_IDENTIFIER,
                              0,
                              ' ',
                              width,
                              height,
                              255);
        int padding = sizeof(ppm_header) - length;

        length = snprintf(ppm_header, sizeof(ppm_header), "%s\n#%*c\n%d %d\n%d\n",
                          grayscale ? PPM_GRAYSCALE_IDENTIFIER : PPM_COLOR_IDENTIFIER,
                          padding,
                          ' ',
                          width,
                          height,
                          255);
        fwrite(ppm_header, 1, PPM_HEADER_LENGTH, outputfile);
        fseek(outputfile, 0, SEEK_END);
    }
    
}

#ifndef NO_CUPS
static void _write_header_pwg(cups_raster_t *outputfile, int print_resolution, wprint_image_info_t image_info, cups_page_header2_t *h, bool monochrome) {

    if (outputfile != NULL) {
        strcpy(h->MediaClass, "PwgRaster");
        strcpy(h->MediaColor, "");
        strcpy(h->MediaType, "");
        strcpy(h->OutputType, "");
        h->AdvanceDistance = 0;
        h->AdvanceMedia = CUPS_ADVANCE_FILE;
        h->Collate = CUPS_FALSE;
        h->CutMedia = CUPS_CUT_NONE;
        h->Duplex = CUPS_FALSE;
        h->HWResolution[0] = print_resolution;
        h->HWResolution[1] = print_resolution;

        h->cupsPageSize[0] = (float) ( ( wprint_image_get_width(&image_info) / h->HWResolution[0]) * 72);
        h->cupsPageSize[1] =  (float) ( ( wprint_image_get_height(&image_info)  / h->HWResolution[1]) * 72);

        h->ImagingBoundingBox[0] = 0;
        h->ImagingBoundingBox[1] = 0;
        h->ImagingBoundingBox[2] = h->cupsPageSize[0];
        h->ImagingBoundingBox[3] = h->cupsPageSize[1];
        h->cupsBorderlessScalingFactor = 1.0;
        h->InsertSheet = CUPS_FALSE;
        h->Jog = CUPS_JOG_NONE;
        h->LeadingEdge = CUPS_EDGE_TOP;
        h->Margins[0] = 0;
        h->Margins[1] = 0;
        h->ManualFeed =CUPS_TRUE;
        h->MediaPosition = 0;
        h->MediaWeight = 0;
        h->MirrorPrint = CUPS_FALSE;
        h->NegativePrint = CUPS_FALSE;
        h->NumCopies = 1;
        h->Orientation = image_info.rotation;
        h->OutputFaceUp = CUPS_FALSE;
        h->PageSize[0] = (int)h->cupsPageSize[0];
        h->PageSize[1] = (int)h->cupsPageSize[1];
        h->Separations = CUPS_TRUE;
        h->TraySwitch = CUPS_TRUE;
        h->Tumble = CUPS_TRUE;
        h->cupsWidth = (int) wprint_image_get_width(&image_info) ;
        h->cupsHeight =(int) wprint_image_get_height(&image_info);
        h->cupsMediaType = 2;
        h->cupsBitsPerPixel = (monochrome ? 8 : 24);
        h->cupsBitsPerColor = 8;
        h->cupsBytesPerLine = (h->cupsBitsPerPixel * wprint_image_get_width(&image_info) + 7 ) / 8;
        h->cupsColorOrder = CUPS_ORDER_CHUNKED;
        h->cupsColorSpace = (monochrome ? CUPS_CSPACE_SW : CUPS_CSPACE_RGB);
        h->cupsCompression = 0;
        h->cupsRowCount = 1;
        h->cupsRowFeed = 1;
        h->cupsRowStep = 1;
        h->cupsNumColors = (monochrome ? 1 : 3);
        h->cupsImagingBBox[0] = 0.0;
        h->cupsImagingBBox[1] = 0.0;
        h->cupsImagingBBox[2] = 0.0;
        h->cupsImagingBBox[3] = 0.0;
        int j;
        for( j = 0; j<16; j++)
        {
            h->cupsInteger[j] = (int)1+j;
            h->cupsReal[j] = (float)j+1.1;
            snprintf(h->cupsString[j], 20, "String # %d", j+1);
        }

        strcpy(h->cupsMarkerType, "Marker Type");
        strcpy(h->cupsRenderingIntent, "endering IntentR");
        strcpy(h->cupsPageSizeName, "Letter");

        //Now write the header
        cupsRasterWriteHeader2(outputfile, h);



    }
}
#endif /* NO_CUPS */

static struct option long_options[] = {
    { "help",                0, NULL, '?'                        },
    { "debug",               1, NULL, 'd'                        },
    { "input",               1, NULL, 'i'                        },
    { "output",              1, NULL, 'o'                        },
    { "log-file",            1, NULL, 'l'                        },
    { "borderless",          0, NULL, 'b'                        },
    { "render-flags",        1, NULL, 'f'                        },
    { "media-size",          1, NULL, 'm'                        },
    { "printable-width",     1, NULL, 'w'                        },
    { "printable-height",    1, NULL, 'h'                        },
    { "stripe-height",       1, NULL, 's'                        },
    { "concurrent-stripes",  1, NULL, 'c'                        },
    { "rotation",            1, NULL, 'r'                        },
    { "duplex",              1, NULL, '2'                        },
    { "pcl-type",            1, NULL, 'p'                        },
    { "laser-jet",           0, NULL, 'z'                        },
    { "resolution",          1, NULL, 'x'                        },
    { "page-backside",       0, NULL, 'u'                        },
    { "extra-margin-left",   1, NULL, OPTION_EXTRA_MARGIN_LEFT   },
    { "extra-margin-right",  1, NULL, OPTION_EXTRA_MARGIN_RIGHT  },
    { "extra-margin-top",    1, NULL, OPTION_EXTRA_MARGIN_TOP    },
    { "extra-margin-bottom", 1, NULL, OPTION_EXTRA_MARGIN_BOTTOM },
    { "mono",                0, NULL, OPTION_MONOCHROME          },
    { "verbose",             0, NULL, 'v'                        },

    { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
    FILE *stdlog = NULL;
    bool borderless = false;
    char *input_pathname = NULL;
    char *output_pathname = NULL;
    int  debug = DBG_ALL;
    int option;
    int option_index;
    int render_flags = 0;
    int printable_width = -1;
    int printable_height = -1;
    int rotation = -1;
    int inkjet = 1;
    int resolution = 300;
    int page_backside = 0;
    int stripe_height = STRIPE_HEIGHT;
    int concurrent_stripes = (BUFFERED_ROWS / STRIPE_HEIGHT);
    DF_duplex_t duplex = DF_DUPLEX_MODE_NONE;
    DF_media_size_t media_size = DF_MEDIA_SIZE_UNKNOWN;
    output_format_t output_format = OUTPUT_PPM;
    int testResult = ERROR;
    double extra_margin_left, extra_margin_right, extra_margin_top, extra_margin_bottom;
    int padding_options = PAD_NONE;

    extra_margin_left = extra_margin_right = extra_margin_top = extra_margin_bottom = 0.0f;

    const char *logfilename = NULL;

    FILE *imgfile    = NULL;
    FILE *outputfile = NULL;

    bool monochrome = false;

#ifndef EXCLUDE_PCLM
    bool pclm_test_mode = false;
    uint8 *pclm_output_buffer = NULL;
    void *pclmgen_obj= NULL;

    int outBuffSize = 0;

    PCLmPageSetup mypage;
    PCLmPageSetup  *page_info = &mypage;
    memset(page_info, 0, sizeof(PCLmPageSetup));
#endif /* EXCLUDE_PCLM */

   //Add pwg definitions
#ifndef NO_CUPS
    cups_raster_t *ras_out = NULL; /* Raster stream */
    cups_page_header2_t header_pwg; /* Page header */
#endif /* NO_CUPS */
    while ( (option = getopt_long(argc, argv, "i:o:d:l:w:h:f:s:c:m:r:2:p:x:e:buz?v", 
                             long_options, &option_index)) != EOF )
    {
        switch(option)
        {
            case 'i':
                input_pathname = optarg;
                break;

            case 'o':
                output_pathname = optarg;
                break;

            case 'd':
                debug = atoi(optarg);
                break;

            case 'l':
                logfilename = optarg;
                break;

            case 'b':
                borderless = true;
                break;

            case 'f':
                render_flags = atoi(optarg);
                break;

            case 'm':
                media_size = atoi(optarg);
                break;

            case 'w':
                printable_width = atoi(optarg);
                break;

            case 'h':
                printable_height = atoi(optarg);
                break;

            case 'r':
                rotation = atoi(optarg);
                break;

            case '2':
                duplex = atoi(optarg);
                break;

            case 'z':
                inkjet = 0;
                break;

            case 'x':
                resolution = atoi(optarg);
                break;

            case 'u':
                page_backside = 1;
                break;

            case 'p':
                output_format = atoi(optarg);
                break;

            case 's':
                stripe_height = atoi(optarg);
                break;

            case 'c':
                concurrent_stripes = atoi(optarg);
                break;

            case 'e':
                padding_options = atoi(optarg) & PAD_ALL;
                break;

            case OPTION_EXTRA_MARGIN_LEFT:
                extra_margin_left = atof(optarg);
                break;

            case OPTION_EXTRA_MARGIN_RIGHT:
                extra_margin_right = atof(optarg);
                break;

            case OPTION_EXTRA_MARGIN_TOP:
                extra_margin_top = atof(optarg);
                break;
            case OPTION_EXTRA_MARGIN_BOTTOM:
                extra_margin_bottom = atof(optarg);
                break;

            case OPTION_MONOCHROME:
                monochrome = true;
                break;

            case 'v':
#ifndef EXCLUDE_PCLM
                pclm_test_mode = true;
                break;
#endif /* EXCLUDE_PCLM */

            case '?':
            default:
                _print_usage(argv[0]);
                return(0);
        }  /*  switch(option)  */
    }  /*  while */

    // logging to a file?
    if (logfilename != NULL) {
        // open the logfile
        stdlog = fopen(logfilename, "w");
    }

    // set the logging level and output
    wprint_set_debug_level(debug);
    wprint_set_stdlog(((stdlog != NULL) ? stdlog : stderr));
    {
        char buffer[4096];

		int param;
        snprintf(buffer, sizeof(buffer), "JOB PARAMETERS:");
		for(param = 1; param < argc; param++)
		{
            strncat(buffer, " ", sizeof(buffer));
            strncat(buffer, argv[param], sizeof(buffer));
		}
        ifprint((DBG_FORCE, "%s", buffer));
    }

    switch(output_format) {
#ifndef EXCLUDE_PCLM
        case OUTPUT_PDF:
            if (!pclm_test_mode) {
                padding_options = PAD_ALL;
            }
            break;
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
        case OUTPUT_PWG:
            break;
#endif /* NO_CUPS */

        case OUTPUT_PPM:
            break;

        default:
            ifprint((DBG_FORCE, "ERROR: output format not supported, switching to PPM"));
            output_format = OUTPUT_PPM;
            break;
    }

    do {

        /*  if input file is specified at the end of the command line 
         *  without the '-f' option, take it  
         */
        if (!input_pathname && optind < argc && argv[optind] != NULL)
        {
            // coverity[var_assign_var]
            input_pathname = argv[optind++];
        }

        if (!input_pathname)
        {
            ifprint((DBG_FORCE, "ERROR: invalid arguments"));
            _print_usage(argv[0]);
            continue;
        }

        /*  if output file is specified at the end of the command line 
         *  without the '-f' option, take it  
         */
        if (!output_pathname && optind < argc && argv[optind] != NULL)
        {
            // coverity[var_assign_var]
            output_pathname = argv[optind++];
        }

        if ((media_size != DF_MEDIA_SIZE_UNKNOWN) &&
            (printable_width <= 0) && (printable_height <= 0))
        {
            float margin_top, margin_bottom, margin_left, margin_right;
            wprint_job_params_t job_params;
            printer_capabilities_t printer_caps;

            memset(&job_params, 0, sizeof(wprint_job_params_t));
            memset(&printer_caps, 0, sizeof(printer_capabilities_t));
            printer_caps.canDuplex = 1;
            printer_caps.canPrintBorderless = 1;
            printer_caps.inkjet = inkjet;

            job_params.media_size = media_size;
            job_params.pixel_units = resolution;
            job_params.duplex = duplex,
            job_params.borderless = borderless;

            switch(output_format) {
#ifndef EXCLUDE_PCLM
                case OUTPUT_PDF:
                    job_params.pcl_type = PCLm;
                    break;
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
                case OUTPUT_PWG:
                    job_params.pcl_type = PCLPWG;
                    break;
#endif /* NO_CUPS */

                default:
                    job_params.pcl_type = PCLNONE;
                    break;
            }

            printable_area_get_default_margins(&job_params, &printer_caps, &margin_top, &margin_left, &margin_right, &margin_bottom);
            printable_area_get(&job_params, margin_top, margin_left, margin_right, margin_bottom);

            printable_width  = job_params.printable_area_width;
            printable_height = job_params.printable_area_height;

           // mypage.mediaWidthInPixels = printable_width;
            //mypage.mediaHeightInPixels = printable_height;

#ifndef EXCLUDE_PCLM
             if(margin_left < 0.0f || margin_top < 0.0f)
             {
                 mypage.mediaWidthOffset=0.0f;
                 mypage.mediaHeightOffset=0.0f;
             }
             else
             {
                 mypage.mediaWidthOffset=margin_left;
                 mypage.mediaHeightOffset=margin_top;
             }
             mypage.pageOrigin=top_left;	// REVISIT
             mypage.dstColorSpaceSpefication=deviceRGB;
#endif /* EXCLUDE_PCLM */
        }

#ifndef EXCLUDE_PCLM
        mypage.stripHeight=stripe_height;

        if(resolution==300)
        {
            mypage.destinationResolution=res300;
        }
        else if(resolution==600)
        {
            mypage.destinationResolution=res600;
        }
        else if(resolution==1200)
        {
            mypage.destinationResolution=res1200;
        }

        mypage.duplexDisposition=simplex;
        mypage.mirrorBackside=false;
#endif /* EXCLUDE_PCLM */

        if ((printable_width <= 0) || (printable_height <= 0)) {
            ifprint((DBG_FORCE, "ERROR: missing argumetns for dimensions"));
            _print_usage(argv[0]);
            continue;
        }

        imgfile = fopen(input_pathname, "r");
        if (imgfile == NULL) {
            ifprint((DBG_FORCE, "unable to open input file"));
            continue;
        }

        testResult = OK;

        outputfile = NULL;
        if (output_pathname != NULL) {
            outputfile = fopen(output_pathname, "w");
        } else {
            ifprint((DBG_FORCE, "output file not provided"));
        }

        wprint_image_info_t image_info;

        wprint_image_setup(&image_info, MIME_TYPE_HPIMAGE, &_wprint_ifc);
        wprint_image_init(&image_info);
    
        /*  get the image_info of the input file of specified MIME type  */
        if ( wprint_image_get_info(imgfile, &image_info) == OK )
        {
            if (rotation < 0) {
                rotation = ROT_0;
                if ((render_flags & RENDER_FLAG_PORTRAIT_MODE) != 0) {
                    ifprint((DBG_LOG, "_print_page(): portrait mode"));
                    rotation = ROT_0;
                } else if ((render_flags & RENDER_FLAG_LANDSCAPE_MODE) != 0) {
                    ifprint((DBG_LOG, "_print_page(): landscape mode"));
                    rotation = ROT_90;
                } else if (wprint_image_is_landscape(&image_info) &&
                    ((render_flags & RENDER_FLAG_AUTO_ROTATE) != 0)) {
                    ifprint((DBG_LOG, "_print_page(): auto mode"));
                    rotation = ROT_90;
                }
                if ((duplex == DF_DUPLEX_MODE_BOOK) &&
                    page_backside &&
                    ((render_flags & RENDER_FLAG_ROTATE_BACK_PAGE) != 0) &&
                    ((render_flags & RENDER_FLAG_BACK_PAGE_PREROTATED) == 0))
                    rotation = ((rotation == ROT_0) ? ROT_180 : ROT_270);
            }
        }
        else {
            ifprint((DBG_FORCE, "unable to process image"));
        }

        wprint_image_set_output_properties(&image_info,
                                           rotation,
                                           printable_width,
                                           printable_height,
                                           floor(resolution * extra_margin_top),
                                           floor(resolution * extra_margin_left),
                                           floor(resolution * extra_margin_right),
                                           floor(resolution * extra_margin_bottom),
                                           render_flags,
                                           stripe_height,
                                           concurrent_stripes,
                                           padding_options);

        int buff_size = wprint_image_get_output_buff_size(&image_info);

        unsigned char * buff = (unsigned char *)malloc(buff_size);
        memset(buff, 0xff, buff_size);

        int rows_left, num_rows;
        int output_width = wprint_image_get_width(&image_info);
        num_rows = rows_left = wprint_image_get_height(&image_info);
        int bytes_per_row = BYTES_PER_PIXEL(output_width);

        // process job start
        switch(output_format)
        {
#ifndef EXCLUDE_PCLM
            case OUTPUT_PDF: {
                pclmgen_obj= (void*)CreatePCLmGen();
                outBuffSize = 0;
                PCLmStartJob(pclmgen_obj, (void**)&pclm_output_buffer, &outBuffSize, pclm_test_mode);
                if (outputfile != NULL) {
                    fwrite((char *)pclm_output_buffer, 1, outBuffSize, outputfile);
                }
                break;
            }
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
            case OUTPUT_PWG: {
                ras_out = cupsRasterOpen(fileno(outputfile), CUPS_RASTER_WRITE_PWG);
                break;
            }
#endif /* NO_CUPS */

            default: {
                break;
            }

        }

        // write start page information
        switch(output_format)
        {
#ifndef EXCLUDE_PCLM
            case OUTPUT_PDF: {
                mypage.SourceWidthPixels    = output_width;
                mypage.SourceHeightPixels  = num_rows;
                if (media_size != DF_MEDIA_SIZE_UNKNOWN) {
                    _get_pclm_media_size_name(media_size, (char*)mypage.mediaSizeName);
                    PCLmGetMediaDimensions(pclmgen_obj, mypage.mediaSizeName, &mypage );
                } else {
                    strcpy((char*)mypage.mediaSizeName, "CUSTOM");
				    mypage.mediaWidth      = (((float)printable_width * STANDARD_SCALE_FOR_PDF)/(float)resolution);
				    mypage.mediaHeight     = (((float)printable_height * STANDARD_SCALE_FOR_PDF)/(float)resolution);
				    mypage.mediaWidthInPixels   = printable_width;
				    mypage.mediaHeightInPixels   = printable_height;
                }
                float   standard_scale =(float)resolution/(float)72;
                page_info->sourceHeight = (float)num_rows/standard_scale;
	            page_info->sourceWidth = (float)output_width/standard_scale;
	            page_info->colorContent=color_content;
                page_info->srcColorSpaceSpefication=deviceRGB;
                page_info->compTypeRequested=compressDCT;
                outBuffSize = 0;
                PCLmStartPage(pclmgen_obj, page_info, (void**)&pclm_output_buffer, &outBuffSize);
                if (outputfile != NULL) {
                    fwrite((char *)pclm_output_buffer, 1, outBuffSize, outputfile);
                }
                break;
            }
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
            case OUTPUT_PWG: {
                _write_header_pwg(ras_out, resolution, image_info, &header_pwg, monochrome);

                /*
                  * Output the pages...
                */

                ifprint((DBG_LOG, "cupsWidth = %d", header_pwg.cupsWidth));
                ifprint((DBG_LOG, "cupsHeight = %d", header_pwg.cupsHeight));
                ifprint((DBG_LOG, "cupsBitsPerColor = %d", header_pwg.cupsBitsPerColor));
                ifprint((DBG_LOG, "cupsBitsPerPixel = %d", header_pwg.cupsBitsPerPixel));
                ifprint((DBG_LOG, "cupsBytesPerLine = %d", header_pwg.cupsBytesPerLine));
                ifprint((DBG_LOG, "cupsColorOrder = %d", header_pwg.cupsColorOrder));
                ifprint((DBG_LOG, "cupsColorSpace = %d", header_pwg.cupsColorSpace));
                break;
            }
#endif /* NO_CUPS */

            default: {
                _write_header(outputfile, output_width, num_rows, monochrome);
                break;
            }
        }

        int height, nbytes;
        int image_row = 0;
        int actual_rows = 0;

        while(rows_left > 0) {
            height = MIN(rows_left, stripe_height);
        
            nbytes = wprint_image_decode_stripe(&image_info,
                                            image_row,
                                            &height,
                                            buff);

            if (nbytes > 0) {
                int rows_returned = (nbytes / bytes_per_row);
                actual_rows += rows_returned;
                if (height != rows_returned) {
                    ifprint((DBG_LOG, "LOG: mismatch in reported bytes & height: %d vs %d", height, rows_returned));
                }
                if (monochrome) {
                    int readIndex, writeIndex;
                    for(readIndex = writeIndex = 0; readIndex < nbytes; readIndex += BYTES_PER_PIXEL(1), writeIndex++) {
                        buff[writeIndex] = SP_GRAY(buff[readIndex + 0], buff[readIndex + 1], buff[readIndex + 2]);
                    }
                    nbytes = writeIndex;
                }

                // write the data
                switch(output_format)
                {
#ifndef EXCLUDE_PCLM
                    case OUTPUT_PDF: {
                        outBuffSize= 0;
                        PCLmEncapsulate(pclmgen_obj, buff, bytes_per_row, rows_returned, (void**)&pclm_output_buffer, &outBuffSize);
                        if (outputfile != NULL) {
                            fwrite((char *)pclm_output_buffer, 1, outBuffSize, outputfile);
                        }
                        break;
                    }
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
                    case OUTPUT_PWG: {
                        if (ras_out != NULL) {
                            cupsRasterWritePixels(ras_out, buff, nbytes);
                        }
                        break;
                    }
#endif /* NO_CUPS */
                    
                    default: {
                        if (outputfile != NULL) {
                            fwrite(buff, 1, nbytes, outputfile);
                        }
                    }
                }

                image_row += height;
                rows_left -= height;
            } else {
                if (nbytes < 0) {
                    ifprint((DBG_ERROR, "ERROR: file appears to be corrupted"));
                } else {
                    ifprint((DBG_ERROR, "LOG: data end with request image_row: %d for %d rows", image_row, MIN(rows_left, stripe_height)));
                }
                break;
            }
        }

        if (num_rows != actual_rows) {
            ifprint((DBG_ERROR, "ERROR: actual image rows: %d", actual_rows));
        }

        // end of page processing
        switch(output_format) {
#ifndef EXCLUDE_PCLM
                case OUTPUT_PDF: {
                    outBuffSize = 0;
                    PCLmEndPage(pclmgen_obj, (void**)&pclm_output_buffer, &outBuffSize);
                    if (outputfile != NULL) {
                        fwrite((char *)pclm_output_buffer, 1, outBuffSize, outputfile);
                    }
                    break;
                }
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
                case OUTPUT_PWG: {
                    break;
                }
#endif /* NO_CUPS */

                default: {
                    if (num_rows != actual_rows) {
                        _write_header(outputfile, wprint_image_get_width(&image_info), actual_rows, monochrome);
                        break;
                    }
                    break;
                }
        }

        // end of job processing
        switch(output_format) {
#ifndef EXCLUDE_PCLM
            case OUTPUT_PDF: {
                outBuffSize = 0;
                PCLmEndJob(pclmgen_obj, (void**)&pclm_output_buffer, &outBuffSize);
                if (outputfile != NULL) {
                    fwrite((char *)pclm_output_buffer, 1, outBuffSize, outputfile);
                }

                PCLmFreeBuffer(pclmgen_obj, pclm_output_buffer);
                DestroyPCLmGen(pclmgen_obj);
                break;
            }
#endif /* EXCLUDE_PCLM */

#ifndef NO_CUPS
            case OUTPUT_PWG: {
                break;
            }
#endif /* NO_CUPS */

            default: {
                break;
            }
        }

        wprint_image_cleanup(&image_info);

        if (buff != NULL) {
            free(buff);
        }

    } while(0);

    // close the imagefile
    if (imgfile != NULL) {
        fclose(imgfile);
    }

    // close the output file
    if (outputfile != NULL) {
        fclose(outputfile);
    }

    //if we use a res stream close it
#ifndef NO_CUPS
    if(ras_out != NULL){
        cupsRasterClose(ras_out);
    }
#endif /* NO_CUPS */

    // close the logfile
    if (stdlog != NULL) {
        fclose(stdlog);
    }

    return(testResult);
}  /* main */
