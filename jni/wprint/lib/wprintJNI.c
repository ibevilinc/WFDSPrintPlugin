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

#include <jni.h>
#include "lib_wprint.h"
#include "wprint_debug.h"
#include <string.h>
#include <android/log.h>
#include <errno.h>
#include "wprint_debug.h"
#include "wprint_status_types.h"
#include "ifc_osimg.h"
#include <android/bitmap.h>

static jclass   _wPrintJobParamsClass;
static jfieldID _wPrintJobParamsField__borderless;
static jfieldID _wPrintJobParamsField__duplex;
static jfieldID _wPrintJobParamsField__media_size;
static jfieldID _wPrintJobParamsField__media_type;
static jfieldID _wPrintJobParamsField__media_tray;
static jfieldID _wPrintJobParamsField__quality;
static jfieldID _wPrintJobParamsField__color_space;
static jfieldID _wPrintJobParamsField__render_flags;
static jfieldID _wPrintJobParamsField__num_copies;
static jfieldID _wPrintJobParamsField__page_range;
static jfieldID _wPrintJobParamsField__print_resolution;
static jfieldID _wPrintJobParamsField__printable_width;
static jfieldID _wPrintJobParamsField__printable_height;
static jfieldID _wPrintJobParamsField__page_width;
static jfieldID _wPrintJobParamsField__page_height;
static jfieldID _wPrintJobParamsField__page_margin_top;
static jfieldID _wPrintJobParamsField__page_margin_left;
static jfieldID _wPrintJobParamsField__page_margin_right;
static jfieldID _wPrintJobParamsField__page_margin_bottom;
static jfieldID _wPrintJobParamsField__job_margin_top;
static jfieldID _wPrintJobParamsField__job_margin_left;
static jfieldID _wPrintJobParamsField__job_margin_right;
static jfieldID _wPrintJobParamsField__job_margin_bottom;
static jfieldID _wPrintJobParamsField__fit_to_page;
static jfieldID _wPrintJobParamsField__fill_page;
static jfieldID _wPrintJobParamsField__auto_rotate;
static jfieldID _wPrintJobParamsField__portrait_mode;
static jfieldID _wPrintJobParamsField__landscape_mode;
static jfieldID _wPrintJobParamsField__nativeData;
static jfieldID _wPrintJobParamsField__document_category;
static jfieldID _wPrintJobParamsField__alignment;

static jclass   _wPrintPrinterCapabilitiesClass;
static jfieldID _wPrintPrinterCapabilitiesField__can_duplex;
static jfieldID _wPrintPrinterCapabilitiesField__has_photo_tray;
static jfieldID _wPrintPrinterCapabilitiesField__can_print_borderless;
static jfieldID _wPrintPrinterCapabilitiesField__can_print_color;
static jfieldID _wPrintPrinterCapabilitiesField__printer_name;
static jfieldID _wPrintPrinterCapabilitiesField__printer_make;
static jfieldID _wPrintPrinterCapabilitiesField__is_supported;
static jfieldID _wPrintPrinterCapabilitiesField__can_cancel;
static jfieldID _wPrintPrinterCapabilitiesField__nativeData;
static jfieldID _wPrintPrinterCapabilitiesField__supported_mime_types;
static jfieldID _wPrintPrinterCapabilitiesField__supported_media_trays;
static jfieldID _wPrintPrinterCapabilitiesField__supported_media_types;
static jfieldID _wPrintPrinterCapabilitiesField__supported_media_sizes;
static jfieldID _wPrintPrinterCapabilitiesField__supports_quality_draft;
static jfieldID _wPrintPrinterCapabilitiesField__supports_quality_normal;
static jfieldID _wPrintPrinterCapabilitiesField__supports_quality_best;
static jfieldID _wPrintPrinterCapabilitiesField__suppliesURI;

static jclass    _wPrintServiceJobHandlerClass;
static jobject   _callbackReceiver;
static jmethodID _wPrintServiceJobHandlerMethod__jobCallback;

static jclass    _wPrintCallbackParamsClass;
static jmethodID _wPrintCallbackParamsMethod__init;
static jfieldID  _wPrintCallbackParamsField__callback_id;
static jfieldID  _wPrintCallbackParamsField__job_state;
static jfieldID  _wPrintCallbackParamsField__page_num;
static jfieldID  _wPrintCallbackParamsField__copy_num;
static jfieldID  _wPrintCallbackParamsField__corrupted_file;
static jfieldID  _wPrintCallbackParamsField__current_page;
static jfieldID  _wPrintCallbackParamsField__total_pages;
static jfieldID  _wPrintCallbackParamsField__page_total_update;
static jfieldID  _wPrintCallbackParamsField__job_id;
static jfieldID  _wPrintCallbackParamsField__blocked_reasons;
static jfieldID  _wPrintCallbackParamsField__job_done_result;
static jfieldID  _wPrintCallbackParamsField__page_start_info;
static jfieldID  _wPrintCallbackParamsField__job_state_str;
static jfieldID  _wPrintCallbackParamsField__job_done_result_str;
static jfieldID  _wPrintCallbackParamsField__blocked_reasons_strings;
static jfieldID  _wPrintCallbackParamsField__printer_state_str;

static jclass    _WPrintPrinterStatusMonitorClass;
static jmethodID _WPrintPrinterStatusMonitorMethod__callbackReceiver;

static jclass    _PrintServiceStringsClass;
static jfieldID  _PrintServiceStringsField__PRINTER_STATE_IDLE;
static jfieldID  _PrintServiceStringsField__PRINTER_STATE_RUNNING;
static jfieldID  _PrintServiceStringsField__PRINTER_STATE_BLOCKED;
static jfieldID  _PrintServiceStringsField__PRINTER_STATE_UNKNOWN;
static jfieldID  _PrintServiceStringsField__JOB_STATE_QUEUED;
static jfieldID  _PrintServiceStringsField__JOB_STATE_RUNNING;
static jfieldID  _PrintServiceStringsField__JOB_STATE_BLOCKED;
static jfieldID  _PrintServiceStringsField__JOB_STATE_DONE;
static jfieldID  _PrintServiceStringsField__JOB_STATE_OTHER;
static jfieldID  _PrintServiceStringsField__JOB_DONE_OK;
static jfieldID  _PrintServiceStringsField__JOB_DONE_ERROR;
static jfieldID  _PrintServiceStringsField__JOB_DONE_CANCELLED;
static jfieldID  _PrintServiceStringsField__JOB_DONE_CORRUPT;
static jfieldID  _PrintServiceStringsField__JOB_DONE_OTHER;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__OFFLINE;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__BUSY;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__CANCELLED;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_PAPER;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_INK;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_TONER;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__JAMMED;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__DOOR_OPEN;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__SERVICE_REQUEST;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_INK;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_TONER;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__REALLY_LOW_ON_INK;
static jfieldID  _PrintServiceStringsField__BLOCKED_REASON__UNKNOWN;
static jfieldID  _PrintServiceStringsField__ALIGNMENT__CENTER;
static jfieldID  _PrintServiceStringsField__ALIGNMENT__CENTER_HORIZONTAL;
static jfieldID  _PrintServiceStringsField__ALIGNMENT__CENTER_VERTICAL;
static jfieldID  _PrintServiceStringsField__ALIGNMENT__CENTER_HORIZONTAL_ON_ORIENTATION;

static jclass    _BitmapClass;
static jmethodID _BitmapMethod__recycle;
static jmethodID _BitmapMethod__getPixel;

static jclass    _BitmapFactoryClass;
static jmethodID _BitmapFactoryMethod__decodeFileDescriptor;

static jclass    _BitmapConfigClass;
static jmethodID _BitmapConfigMethod__valueOf;

static jclass    _BitmapFactoryOptionsClass;
static jmethodID _BitmapFactoryOptionsMethod__init;
static jfieldID  _BitmapFactoryOptionsField__inJustDecodeBounds;
static jfieldID  _BitmapFactoryOptionsField__inSampleSize;
static jfieldID  _BitmapFactoryOptionsField__inPreferredConfig;
static jfieldID  _BitmapFactoryOptionsField__outHeight;
static jfieldID  _BitmapFactoryOptionsField__outWidth;

static jclass    _FileDescriptorClass;
static jmethodID _FileDescriptorMethod__init;
static jfieldID  _FileDescriptorField__descriptor;

static jclass    _ColorClass;
static jmethodID _ColorMethod__alpha;
static jmethodID _ColorMethod__red;
static jmethodID _ColorMethod__green;
static jmethodID _ColorMethod__blue;

static JavaVM *_JVM = NULL;
static JNIEnv *_JNIenv;
static jstring _fakeDir;

static int _logLevel = ANDROID_LOG_DEBUG;

#define TAG "wprintJNI"

typedef struct {
    AndroidBitmapInfo bitmapInfo;
    jobject bitmap;
    void *pixels;
} osimg_params_t;

static int _validate_sample_size(int sample) {
    int valid_sample = 1;
    if (sample > 0) {
        while(valid_sample < sample) valid_sample << 1;
    }
    
    return valid_sample;
}

static jobject _create_file_descriptor(JNIEnv *env, FILE *imgfile) {
    jobject fileDescriptor = NULL;

    do
    {
        if (env == NULL) {
            continue;
        }

        // valid file?
        if (imgfile == NULL) {
            continue;
        }

        // reset the file position
        fseek(imgfile, 0, SEEK_SET);

        // ged the fd
        int fd = fileno(imgfile);
        if (fd < 0) {
            continue;
        }

        // create file descriptor object
        fileDescriptor = (*env)->NewObject(env, _FileDescriptorClass, _FileDescriptorMethod__init);
        if (fileDescriptor == NULL) {
            continue;
        }
        
        // set the input field
        (*env)->SetIntField(env, fileDescriptor, _FileDescriptorField__descriptor, fd);

    } while(0);

    // return file descriptor object
    return fileDescriptor;
}

static jobject _create_bitmap_config(JNIEnv *env) {
    jobject bitmapConfig = NULL;
    do
    {
        if (env == NULL) {
            continue;
        }

        char enumName[10] = "ARGB_8888";
        jstring argb_8888_enum = (*env)->NewStringUTF(env, (const char*)enumName);
        if (argb_8888_enum == NULL) {
            continue;
        }

        bitmapConfig = (*env)->CallStaticObjectMethod(env, _BitmapConfigClass, _BitmapConfigMethod__valueOf, argb_8888_enum);

        (*env)->DeleteLocalRef(env, argb_8888_enum);

    } while(0);

    return bitmapConfig;
}

static jobject _create_bitmap_factory_options(JNIEnv *env, bool decodeBounds, int subsampling, jobject preferredConfig) {
    jobject bitmapFactoryOptions = NULL;
    do
    {
        if (env == NULL) {
            continue;
        }

        bitmapFactoryOptions = (*env)->NewObject(env, _BitmapFactoryOptionsClass, _BitmapFactoryOptionsMethod__init);
        if (bitmapFactoryOptions == NULL) {
            continue;
        }

        (*env)->SetBooleanField(env, bitmapFactoryOptions, _BitmapFactoryOptionsField__inJustDecodeBounds, decodeBounds);
        (*env)->SetIntField(env, bitmapFactoryOptions,     _BitmapFactoryOptionsField__inSampleSize,       _validate_sample_size(subsampling));
        (*env)->SetObjectField(env, bitmapFactoryOptions,  _BitmapFactoryOptionsField__inPreferredConfig,  preferredConfig);
        (*env)->SetIntField(env, bitmapFactoryOptions, _BitmapFactoryOptionsField__outWidth, 0);
        (*env)->SetIntField(env, bitmapFactoryOptions, _BitmapFactoryOptionsField__outHeight, 0);
    } while(0);

    if ((env != NULL) && (preferredConfig != NULL)) {
        (*env)->DeleteLocalRef(env, preferredConfig);
    }

    return bitmapFactoryOptions;
}


static int _osimg_ifc__decode_bounds(FILE *imgfile, int *width, int *height) {
    int result = ERROR;

    int needDetach = 0;
    JNIEnv *env;

    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0) {
            return ERROR;
        }
    }

    jobject fileDescriptor = _create_file_descriptor(env, imgfile);

    jobject factoryOptions = _create_bitmap_factory_options(env, true, 1, _create_bitmap_config(env));

    do {

        if (fileDescriptor == NULL) {
            continue;
        }

        if (factoryOptions == NULL) {
            continue;
        }

        (*env)->CallStaticObjectMethod(env, _BitmapFactoryClass, _BitmapFactoryMethod__decodeFileDescriptor, fileDescriptor, NULL, factoryOptions);
        int imgWidth  = (*env)->GetIntField(env, factoryOptions, _BitmapFactoryOptionsField__outWidth);
        int imgHeight = (*env)->GetIntField(env, factoryOptions, _BitmapFactoryOptionsField__outHeight);

        if (width != NULL) {
            *width = imgWidth;
        }

        if (height != NULL) {
            *height = imgHeight;
        }
        
        result = ((imgWidth > 0) && (imgHeight > 0)) ? OK : ERROR;
    } while(0);

    if (fileDescriptor != NULL) {
        (*env)->DeleteLocalRef(env, fileDescriptor);
    }

    if (factoryOptions != NULL) {
        (*env)->DeleteLocalRef(env, factoryOptions);
    }

    if (needDetach) {
        (*_JVM)->DetachCurrentThread(_JVM);
    }

    return result;;
}

static int _osimg_ifc__can_subsample(void) {
    return OK;
}

static void* _osimg_ifc__decode_start(FILE *imgfile, int subsample) {

    int result = ERROR;
    osimg_params_t* params = NULL;
    int needDetach = 0;
    JNIEnv *env;
    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0) {
            return params;
        }
    }

    jobject fileDescriptor = _create_file_descriptor(env, imgfile);

    jobject factoryOptions = _create_bitmap_factory_options(env, false, subsample, _create_bitmap_config(env));

    do {

        if (fileDescriptor == NULL) {
            continue;
        }

        if (factoryOptions == NULL) {
            continue;
        }
        
        // allocate BEFORE the bitmap just in case
        params = malloc(sizeof(osimg_params_t));

        if (params == NULL) {
            continue;
        }
        memset(params, 0, sizeof(osimg_params_t));

        jobject bitmap = (*env)->CallStaticObjectMethod(env, _BitmapFactoryClass, _BitmapFactoryMethod__decodeFileDescriptor, fileDescriptor, NULL, factoryOptions);
        if (bitmap == NULL) {
            continue;
        }

        params->bitmap = (*env)->NewGlobalRef(env, bitmap);

        AndroidBitmapInfo bitmapInfo;
        if (AndroidBitmap_getInfo(env, bitmap, &params->bitmapInfo) < 0) {
            params->bitmapInfo.width  = -1;
            params->bitmapInfo.height = -1;
            continue;
        }

        result = OK;
    } while(0);

    if ((result != OK) && (params != NULL)) {

        if (params->bitmap != NULL) {
            (*env)->CallVoidMethod(env, params->bitmap, _BitmapMethod__recycle);
            (*env)->DeleteGlobalRef(env, params->bitmap);
        }
        free(params);
        params = NULL;
    }

    if (needDetach) {
        (*_JVM)->DetachCurrentThread(_JVM);
    }

    return params;
}

static int _osimg_ifc__get_width(void *param) {
    osimg_params_t *decodeParams = (osimg_params_t*)param;
    int width = -1;
    int needDetach = 0;
    JNIEnv *env;
    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0)
            return width;
    }
    do {
        if (decodeParams == NULL) {
            continue;
        }

        if (decodeParams->bitmap == NULL) {
            continue;
        }

        width = decodeParams->bitmapInfo.width;

    } while(0);

    if (needDetach) {
        (*_JVM)->DetachCurrentThread(_JVM);
    }

    return width;
}

static int _osimg_ifc__get_height(void *param) {
    osimg_params_t *decodeParams = (osimg_params_t*)param;
    int height = -1;
    int needDetach = 0;
    JNIEnv *env;
    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0)
            return height;
    }
    do {
        if (decodeParams == NULL) {
            continue;
        }

        if (decodeParams->bitmap == NULL) {
            continue;
        }

        height = decodeParams->bitmapInfo.height;

    } while(0);

    if (needDetach) {
        (*_JVM)->DetachCurrentThread(_JVM);
    }

    return height;
}

#  define alpha_composite(composite, fg, alpha, bg)                            \
     { uint16 temp = (uint16)((uint16)(fg) * (uint16)(alpha) \
                        +        (uint16)(bg)*(uint16)(255 -       \
                        (uint16)(alpha)) + (uint16)128);           \
       (composite) = (uint8)((temp + (temp >> 8)) >> 8); }

#define GET_RED(x)   (((x) >>  0) & 0xff)
#define GET_BLUE(x)  (((x) >> 16) & 0xff)
#define GET_GREEN(x) (((x) >>  8) & 0xff)
#define GET_ALPHA(x) (((x) >> 24) & 0xff)

static int _osimg_ifc__decode_row(void *param, int row, unsigned char *rowBuf) {
    osimg_params_t *decodeParams = (osimg_params_t*)param;
    int needDetach = 0;
    int result = ERROR;
    JNIEnv *env;
    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0)
            return result;
    }
    do {
        if (decodeParams == NULL) {
            continue;
        }

        if (rowBuf == NULL) {
            continue;
        }

        if ((row < 0) || (row >= decodeParams->bitmapInfo.height)) {
            continue;
        }

        int x;
        for(x = 0; x < decodeParams->bitmapInfo.width; x++, rowBuf += 3) {
            int pixel = (*env)->CallIntMethod(env, decodeParams->bitmap, _BitmapMethod__getPixel, x, row);
    
            int a = (*env)->CallStaticIntMethod(env, _ColorClass, _ColorMethod__alpha, pixel);
            int r = (*env)->CallStaticIntMethod(env, _ColorClass, _ColorMethod__red, pixel);
            int g = (*env)->CallStaticIntMethod(env, _ColorClass, _ColorMethod__green, pixel);
            int b = (*env)->CallStaticIntMethod(env, _ColorClass, _ColorMethod__blue, pixel);
            alpha_composite(rowBuf[0], r, a, 255);
            alpha_composite(rowBuf[1], g, a, 255);
            alpha_composite(rowBuf[2], b, a, 255);
        }

        result = OK;
    } while(0);

    if (needDetach) {
        (*_JVM)->DetachCurrentThread(_JVM);
    }

    return result;
}

static void _osimg_ifc__decode_end(void* param) {
    osimg_params_t *decodeParams = (osimg_params_t*)param;

    int needDetach = 0;
    JNIEnv *env;
    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0) {
            return;
        }
    }

    do {
        if (decodeParams == NULL) {
            continue;
        }

        if (decodeParams->bitmap == NULL) {
            continue;
        }

        (*env)->CallVoidMethod(env, decodeParams->bitmap, _BitmapMethod__recycle);

        (*env)->DeleteGlobalRef(env, decodeParams->bitmap);

    } while(0);

    if (decodeParams != NULL) {
        free(decodeParams);
    }

    if (needDetach) {
        (*_JVM)->DetachCurrentThread(_JVM);
    }
}

static ifc_osimg_t _jni_osimg_ifc = 
{
    .decode_bounds  = _osimg_ifc__decode_bounds,
    .can_subsample  = _osimg_ifc__can_subsample,
    .decode_start   = _osimg_ifc__decode_start,
    .get_width      = _osimg_ifc__get_width,
    .get_height     = _osimg_ifc__get_height,
    .decode_row     = _osimg_ifc__decode_row,
    .decode_end     = _osimg_ifc__decode_end,
};


static void _initJNI(JNIEnv *env, jobject callbackReceiver, jstring fakeDir) {

    (*env)->GetJavaVM(env, &_JVM);

    _fakeDir = (jstring)(*env)->NewGlobalRef(env, fakeDir);

     // fill out static accessors for wPrintJobParameters
    _wPrintJobParamsClass                     = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "com/android/wfds/jni/wPrintJobParams"));
    _wPrintJobParamsField__borderless         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "borderless",         "I");
    _wPrintJobParamsField__duplex             = (*env)->GetFieldID(env, _wPrintJobParamsClass, "duplex",             "I");
    _wPrintJobParamsField__media_size         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "media_size",         "I");
    _wPrintJobParamsField__media_type         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "media_type",         "I");
    _wPrintJobParamsField__media_tray         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "media_tray",         "I");
    _wPrintJobParamsField__quality            = (*env)->GetFieldID(env, _wPrintJobParamsClass, "quality",            "I");
    _wPrintJobParamsField__color_space        = (*env)->GetFieldID(env, _wPrintJobParamsClass, "color_space",        "I");
    _wPrintJobParamsField__render_flags       = (*env)->GetFieldID(env, _wPrintJobParamsClass, "render_flags",       "I");
    _wPrintJobParamsField__num_copies         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "num_copies",         "I");
    _wPrintJobParamsField__page_range         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "page_range",         "Ljava/lang/String;");
    _wPrintJobParamsField__print_resolution   = (*env)->GetFieldID(env, _wPrintJobParamsClass, "print_resolution",   "I");
    _wPrintJobParamsField__printable_width    = (*env)->GetFieldID(env, _wPrintJobParamsClass, "printable_width",    "I");
    _wPrintJobParamsField__printable_height   = (*env)->GetFieldID(env, _wPrintJobParamsClass, "printable_height",   "I");
    _wPrintJobParamsField__page_width         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "page_width",         "F");
    _wPrintJobParamsField__page_height        = (*env)->GetFieldID(env, _wPrintJobParamsClass, "page_height",        "F");
    _wPrintJobParamsField__page_margin_top    = (*env)->GetFieldID(env, _wPrintJobParamsClass, "page_margin_top",    "F");
    _wPrintJobParamsField__page_margin_left   = (*env)->GetFieldID(env, _wPrintJobParamsClass, "page_margin_left",   "F");
    _wPrintJobParamsField__page_margin_right  = (*env)->GetFieldID(env, _wPrintJobParamsClass, "page_margin_right",  "F");
    _wPrintJobParamsField__page_margin_bottom = (*env)->GetFieldID(env, _wPrintJobParamsClass, "page_margin_bottom", "F");
    _wPrintJobParamsField__nativeData         = (*env)->GetFieldID(env, _wPrintJobParamsClass, "nativeData",         "[B");
    _wPrintJobParamsField__fit_to_page        = (*env)->GetFieldID(env, _wPrintJobParamsClass, "fit_to_page",        "Z");
    _wPrintJobParamsField__fill_page          = (*env)->GetFieldID(env, _wPrintJobParamsClass, "fill_page",          "Z");
    _wPrintJobParamsField__auto_rotate        = (*env)->GetFieldID(env, _wPrintJobParamsClass, "auto_rotate",        "Z");
    _wPrintJobParamsField__portrait_mode      = (*env)->GetFieldID(env, _wPrintJobParamsClass, "portrait_mode",      "Z");
    _wPrintJobParamsField__landscape_mode     = (*env)->GetFieldID(env, _wPrintJobParamsClass, "landscape_mode",     "Z");
    _wPrintJobParamsField__document_category  = (*env)->GetFieldID(env, _wPrintJobParamsClass, "document_category",  "Ljava/lang/String;");
    _wPrintJobParamsField__alignment          = (*env)->GetFieldID(env, _wPrintJobParamsClass, "alignment",          "I");
    _wPrintJobParamsField__job_margin_top     = (*env)->GetFieldID(env, _wPrintJobParamsClass, "job_margin_top",     "F");
    _wPrintJobParamsField__job_margin_left    = (*env)->GetFieldID(env, _wPrintJobParamsClass, "job_margin_left",    "F");
    _wPrintJobParamsField__job_margin_right   = (*env)->GetFieldID(env, _wPrintJobParamsClass, "job_margin_right",   "F");
    _wPrintJobParamsField__job_margin_bottom  = (*env)->GetFieldID(env, _wPrintJobParamsClass, "job_margin_bottom",  "F");

     // fill out static accessors for wPrintPrinterCapabilities
    _wPrintPrinterCapabilitiesClass = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "com/android/wfds/jni/wPrintPrinterCapabilities"));
    _wPrintPrinterCapabilitiesField__can_duplex              = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "can_duplex",              "Z");
    _wPrintPrinterCapabilitiesField__has_photo_tray          = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "has_photo_tray",          "Z");
    _wPrintPrinterCapabilitiesField__can_print_borderless    = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "can_print_borderless",    "Z");
    _wPrintPrinterCapabilitiesField__can_print_color         = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "can_print_color",         "Z");
    _wPrintPrinterCapabilitiesField__printer_name            = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "printer_name",            "Ljava/lang/String;");
    _wPrintPrinterCapabilitiesField__printer_make            = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "printer_make",            "Ljava/lang/String;");
    _wPrintPrinterCapabilitiesField__is_supported            = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "is_supported",            "Z");
    _wPrintPrinterCapabilitiesField__can_cancel              = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "can_cancel",              "Z");
    _wPrintPrinterCapabilitiesField__nativeData              = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "nativeData",              "[B");
    _wPrintPrinterCapabilitiesField__supported_mime_types    = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "supported_mime_types",    "[Ljava/lang/String;");
    _wPrintPrinterCapabilitiesField__supported_media_trays   = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "supported_media_trays",   "[I");
    _wPrintPrinterCapabilitiesField__supported_media_types   = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "supported_media_types",   "[I");
    _wPrintPrinterCapabilitiesField__supported_media_sizes   = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "supported_media_sizes",   "[I");
    _wPrintPrinterCapabilitiesField__supports_quality_draft  = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "supports_quality_draft",  "Z");
    _wPrintPrinterCapabilitiesField__supports_quality_normal = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "supports_quality_normal", "Z");
    _wPrintPrinterCapabilitiesField__supports_quality_best   = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "supports_quality_best",   "Z");
    _wPrintPrinterCapabilitiesField__suppliesURI             = (*env)->GetFieldID(env, _wPrintPrinterCapabilitiesClass, "suppliesURI",             "Ljava/lang/String;");

    _wPrintCallbackParamsClass = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "com/android/wfds/jni/wPrintCallbackParams"));
    _wPrintCallbackParamsMethod__init                   = (*env)->GetMethodID(env, _wPrintCallbackParamsClass, "<init>", "()V");
    _wPrintCallbackParamsField__callback_id             = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "callback_id",             "I");
    _wPrintCallbackParamsField__job_state               = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "job_state",               "I");
    _wPrintCallbackParamsField__page_num                = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "page_num",                "I");
    _wPrintCallbackParamsField__copy_num                = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "copy_num",                "I");
    _wPrintCallbackParamsField__corrupted_file          = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "corrupted_file",          "I");
    _wPrintCallbackParamsField__current_page            = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "current_page",            "I");
    _wPrintCallbackParamsField__total_pages             = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "total_pages",             "I");
    _wPrintCallbackParamsField__page_total_update       = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "page_total_update",       "I");
    _wPrintCallbackParamsField__job_id                  = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "job_id",                  "I");
    _wPrintCallbackParamsField__blocked_reasons         = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "blocked_reasons",         "I");
    _wPrintCallbackParamsField__job_done_result         = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "job_done_result",         "I");
    _wPrintCallbackParamsField__page_start_info         = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "page_start_info",         "Z");
    _wPrintCallbackParamsField__job_state_str           = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "job_state_str",           "Ljava/lang/String;");
    _wPrintCallbackParamsField__job_done_result_str     = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "job_done_result_str",     "Ljava/lang/String;");
    _wPrintCallbackParamsField__blocked_reasons_strings = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "blocked_reasons_strings", "[Ljava/lang/String;");
    _wPrintCallbackParamsField__printer_state_str       = (*env)->GetFieldID(env, _wPrintCallbackParamsClass, "printer_state_str",       "Ljava/lang/String;");

    if (callbackReceiver) {
        _callbackReceiver = (jobject)(*env)->NewGlobalRef(env, callbackReceiver);
    }
    if (_callbackReceiver) {
        _wPrintServiceJobHandlerClass = (jclass)(*env)->NewGlobalRef(env, (*env)->GetObjectClass(env, _callbackReceiver));
        _wPrintServiceJobHandlerMethod__jobCallback = (*env)->GetMethodID(env, _wPrintServiceJobHandlerClass, "jobCallback", "(ILcom/android/wfds/jni/wPrintCallbackParams;)V");
    } else {
        _callbackReceiver = 0;
    }

    _WPrintPrinterStatusMonitorClass = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "com/android/wfds/common/statusmonitor/WPrintPrinterStatusMonitor"));
    _WPrintPrinterStatusMonitorMethod__callbackReceiver = (*env)->GetMethodID(env, _WPrintPrinterStatusMonitorClass, "callbackReceiver", "(Lcom/android/wfds/jni/wPrintCallbackParams;)V");

    _PrintServiceStringsClass = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "com/hp/android/printplugin/support/PrintServiceStrings"));
    _PrintServiceStringsField__PRINTER_STATE_IDLE                = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "PRINTER_STATE_IDLE",                "Ljava/lang/String;");
    _PrintServiceStringsField__PRINTER_STATE_RUNNING             = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "PRINTER_STATE_RUNNING",             "Ljava/lang/String;");
    _PrintServiceStringsField__PRINTER_STATE_BLOCKED             = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "PRINTER_STATE_BLOCKED",             "Ljava/lang/String;");
    _PrintServiceStringsField__PRINTER_STATE_UNKNOWN             = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "PRINTER_STATE_UNKNOWN",             "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_STATE_QUEUED                  = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_STATE_QUEUED",                  "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_STATE_RUNNING                 = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_STATE_RUNNING",                 "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_STATE_BLOCKED                 = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_STATE_BLOCKED",                 "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_STATE_DONE                    = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_STATE_DONE",                    "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_STATE_OTHER                   = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_STATE_OTHER",                   "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_DONE_OK                       = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_DONE_OK",                       "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_DONE_ERROR                    = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_DONE_ERROR",                    "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_DONE_CANCELLED                = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_DONE_CANCELLED",                "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_DONE_CORRUPT                  = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_DONE_CORRUPT",                  "Ljava/lang/String;");
    _PrintServiceStringsField__JOB_DONE_OTHER                    = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "JOB_DONE_OTHER",                    "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__OFFLINE           = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__OFFLINE",           "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__BUSY              = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__BUSY",              "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__CANCELLED         = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__CANCELLED",         "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_PAPER      = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__OUT_OF_PAPER",      "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_INK        = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__OUT_OF_INK",        "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_TONER      = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__OUT_OF_TONER",      "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__JAMMED            = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__JAMMED",            "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__DOOR_OPEN         = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__DOOR_OPEN",         "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__SERVICE_REQUEST   = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__SERVICE_REQUEST",   "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_INK        = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__LOW_ON_INK",        "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_TONER      = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__LOW_ON_TONER",      "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__REALLY_LOW_ON_INK = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__REALLY_LOW_ON_INK", "Ljava/lang/String;");
    _PrintServiceStringsField__BLOCKED_REASON__UNKNOWN           = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "BLOCKED_REASON__UNKNOWN",           "Ljava/lang/String;");

    _PrintServiceStringsField__ALIGNMENT__CENTER                           = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "ALIGN_CENTER",                           "I");
    _PrintServiceStringsField__ALIGNMENT__CENTER_HORIZONTAL                = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "ALIGN_CENTER_HORIZONTAL",                "I");
    _PrintServiceStringsField__ALIGNMENT__CENTER_VERTICAL                  = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "ALIGN_CENTER_VERTICIAL",                 "I");
    _PrintServiceStringsField__ALIGNMENT__CENTER_HORIZONTAL_ON_ORIENTATION = (*env)->GetStaticFieldID(env, _PrintServiceStringsClass, "ALIGN_CENTER_HORIZONTAL_ON_ORIENTATION", "I");

    _BitmapFactoryOptionsClass                     = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "android/graphics/BitmapFactory$Options"));
    _BitmapFactoryOptionsMethod__init              = (*env)->GetMethodID(env, _BitmapFactoryOptionsClass, "<init>", "()V");
    _BitmapFactoryOptionsField__inJustDecodeBounds = (*env)->GetFieldID(env, _BitmapFactoryOptionsClass, "inJustDecodeBounds", "Z");
    _BitmapFactoryOptionsField__inSampleSize       = (*env)->GetFieldID(env, _BitmapFactoryOptionsClass, "inSampleSize",       "I");
    _BitmapFactoryOptionsField__inPreferredConfig  = (*env)->GetFieldID(env, _BitmapFactoryOptionsClass, "inPreferredConfig",  "Landroid/graphics/Bitmap$Config;");
    _BitmapFactoryOptionsField__outHeight          = (*env)->GetFieldID(env, _BitmapFactoryOptionsClass, "outHeight",          "I");
    _BitmapFactoryOptionsField__outWidth           = (*env)->GetFieldID(env, _BitmapFactoryOptionsClass, "outWidth",           "I");

    _BitmapFactoryClass                        = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "android/graphics/BitmapFactory"));
    _BitmapFactoryMethod__decodeFileDescriptor = (*env)->GetStaticMethodID(env, _BitmapFactoryClass, "decodeFileDescriptor", "(Ljava/io/FileDescriptor;Landroid/graphics/Rect;Landroid/graphics/BitmapFactory$Options;)Landroid/graphics/Bitmap;");

    _BitmapConfigClass           = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "android/graphics/Bitmap$Config"));
    _BitmapConfigMethod__valueOf = (*env)->GetStaticMethodID(env, _BitmapConfigClass, "valueOf", "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");

    _BitmapClass            = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "android/graphics/Bitmap"));
    _BitmapMethod__recycle  = (*env)->GetMethodID(env, _BitmapClass, "recycle", "()V");
    _BitmapMethod__getPixel = (*env)->GetMethodID(env, _BitmapClass, "getPixel", "(II)I");

    _FileDescriptorClass             = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/io/FileDescriptor"));
    _FileDescriptorField__descriptor = (*env)->GetFieldID(env, _FileDescriptorClass, "descriptor", "I");
    _FileDescriptorMethod__init      = (*env)->GetMethodID(env, _FileDescriptorClass, "<init>", "()V");

    _ColorClass         = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "android/graphics/Color"));
    _ColorMethod__alpha = (*env)->GetStaticMethodID(env, _ColorClass, "alpha", "(I)I");
    _ColorMethod__red   = (*env)->GetStaticMethodID(env, _ColorClass, "red",   "(I)I");
    _ColorMethod__green = (*env)->GetStaticMethodID(env, _ColorClass, "green", "(I)I");
    _ColorMethod__blue  = (*env)->GetStaticMethodID(env, _ColorClass, "blue",  "(I)I");
}


static int _convertPrinterCaps_to_C(JNIEnv *env, jobject javaPrinterCaps, printer_capabilities_t *wprintPrinterCaps) {
    if (!javaPrinterCaps || !wprintPrinterCaps) {
        return ERROR;
    }

    jbyteArray nativeDataObject = (jbyteArray)(*env)->GetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__nativeData);
    if (!nativeDataObject) {
        return ERROR;
    }
    char *nativeDataPtr = (*env)->GetByteArrayElements(env, nativeDataObject, NULL);
    memcpy(wprintPrinterCaps, nativeDataPtr, sizeof(printer_capabilities_t));
    (*env)->ReleaseByteArrayElements(env, nativeDataObject, nativeDataPtr, JNI_ABORT);

    return OK;
}


static int _convertPrinterCaps_to_Java(JNIEnv *env, jobject javaPrinterCaps, const printer_capabilities_t *wprintPrinterCaps) {
    if (!javaPrinterCaps || !wprintPrinterCaps) {
        return ERROR;
    }

    int arrayCreated = 0;
    jbyteArray nativeDataObject = (jbyteArray)(*env)->GetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__nativeData);
    if (!nativeDataObject) {
        arrayCreated = 1;
        nativeDataObject = (*env)->NewByteArray(env, sizeof(printer_capabilities_t));
    }

    char *nativeDataPtr = (*env)->GetByteArrayElements(env, nativeDataObject, NULL);
    memcpy(nativeDataPtr, wprintPrinterCaps, sizeof(printer_capabilities_t));
    (*env)->ReleaseByteArrayElements(env, nativeDataObject, nativeDataPtr, 0);

    if (arrayCreated) {
        (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__nativeData, nativeDataObject);
        (*env)->DeleteLocalRef(env, nativeDataObject);
    }

    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__can_duplex,              (jboolean)wprintPrinterCaps->canDuplex);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__has_photo_tray,          (jboolean)wprintPrinterCaps->hasPhotoTray);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__can_print_borderless,    (jboolean)wprintPrinterCaps->canPrintBorderless);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__can_print_color,         (jboolean)wprintPrinterCaps->hasColor);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__is_supported,            (jboolean)wprintPrinterCaps->isSupported);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__can_cancel,              (jboolean)wprintPrinterCaps->canCancel);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__supports_quality_draft,  (jboolean)wprintPrinterCaps->canPrintQualityDraft);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__supports_quality_normal, (jboolean)wprintPrinterCaps->canPrintQualityNormal);
    (*env)->SetBooleanField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__supports_quality_best,   (jboolean)wprintPrinterCaps->canPrintQualityHigh);

    jstring jStr;
    jStr = (*env)->NewStringUTF(env, wprintPrinterCaps->name);
    (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__printer_name, jStr);
    (*env)->DeleteLocalRef(env, jStr);
    jStr = (*env)->NewStringUTF(env, wprintPrinterCaps->make);
    (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__printer_make, jStr);
    (*env)->DeleteLocalRef(env, jStr);

    jintArray intArray;
    int* intArrayPtr;
    int index;

    intArray = (*env)->NewIntArray(env, wprintPrinterCaps->num_supported_media_trays);
    intArrayPtr = (*env)->GetIntArrayElements(env, intArray, NULL);
    for(index = 0; index <  wprintPrinterCaps->num_supported_media_trays; index++) {
        intArrayPtr[index] = (int)wprintPrinterCaps->supported_media_trays[index];
    }
    (*env)->ReleaseIntArrayElements(env, intArray, intArrayPtr, 0);
    (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__supported_media_trays, intArray);
    (*env)->DeleteLocalRef(env, intArray);
   
    intArray = (*env)->NewIntArray(env, wprintPrinterCaps->num_supported_media_types);
    intArrayPtr = (*env)->GetIntArrayElements(env, intArray, NULL);
    for(index = 0; index <  wprintPrinterCaps->num_supported_media_types; index++) {
        intArrayPtr[index] = (int)wprintPrinterCaps->supported_media_types[index];
    }
    (*env)->ReleaseIntArrayElements(env, intArray, intArrayPtr, 0);
    (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__supported_media_types, intArray);
    (*env)->DeleteLocalRef(env, intArray);
   
    intArray = (*env)->NewIntArray(env, wprintPrinterCaps->num_supported_media_sizes);
    intArrayPtr = (*env)->GetIntArrayElements(env, intArray, NULL);
    for(index = 0; index <  wprintPrinterCaps->num_supported_media_sizes; index++) {
        intArrayPtr[index] = (int)wprintPrinterCaps->supported_media_sizes[index];
    }
    (*env)->ReleaseIntArrayElements(env, intArray, intArrayPtr, 0);
    (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__supported_media_sizes, intArray);
    (*env)->DeleteLocalRef(env, intArray);

    int count;
    for(count = index = 0; index < (sizeof(int) * 8); index++) {
        if ((wprintPrinterCaps->supported_input_mime_types & (1 << index)) != 0) {
            count++;
        }
    }

    jStr = (*env)->NewStringUTF(env, ((strlen(wprintPrinterCaps->suppliesURI) > 0) ? wprintPrinterCaps->suppliesURI : ""));
    (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__suppliesURI, jStr);
    (*env)->DeleteLocalRef(env, jStr);

    jStr = (*env)->NewStringUTF(env, "");
    jobjectArray stringArray = (*env)->NewObjectArray(env, count, (*env)->FindClass(env, "java/lang/String"), jStr);
    (*env)->DeleteLocalRef(env, jStr);

    const char *mimetype;
    for(count = index = 0; (stringArray && (index < (sizeof(int) * 8))); index++) {
        mimetype = 0;

        if ((wprintPrinterCaps->supported_input_mime_types & (1 << index)) == 0)
            continue;

        switch(index) {
            case INPUT_MIME_TYPE_PPM:
                mimetype = MIME_TYPE_PPM;
                break;
            case INPUT_MIME_TYPE_JPEG:
                mimetype = MIME_TYPE_JPEG;
                break;
            case INPUT_MIME_TYPE_PNG:
                mimetype = MIME_TYPE_PNG;
                break;
            case INPUT_MIME_TYPE_PDF:
                mimetype = MIME_TYPE_PDF;
                break;
            case INPUT_MIME_TYPE_WORD:
                mimetype = MIME_TYPE_WORD;
                break;
            case INPUT_MIME_TYPE_PPT:
                mimetype = MIME_TYPE_PPT;
                break;
            case INPUT_MIME_TYPE_EXCEL:
                mimetype = MIME_TYPE_EXCEL;
                break;
            case INPUT_MIME_TYPE_PCLM:
                mimetype = MIME_TYPE_PCLM;
                break;
            case INPUT_MIME_TYPE_TEXT:
                mimetype = MIME_TYPE_TXT;
                break;
            case INPUT_MIME_TYPE_HTML:
                mimetype = MIME_TYPE_HTML;
                break;
            case INPUT_MIME_TYPE_TIFF:
                mimetype = MIME_TYPE_TIFF;
                break;
            default:
                mimetype = "";
                break;
        }

        if (mimetype != 0) {
            jstring str =  (*env)->NewStringUTF(env, mimetype);
            if (str != 0) {
                jStr = (*env)->NewStringUTF(env, mimetype);
                (*env)->SetObjectArrayElement(env, stringArray, count++, jStr);
                (*env)->DeleteLocalRef(env, jStr);
            }
        }
    }
    (*env)->SetObjectField(env, javaPrinterCaps, _wPrintPrinterCapabilitiesField__supported_mime_types, stringArray);
    (*env)->DeleteLocalRef(env, stringArray);
}


static int _covertJobParams_to_C(JNIEnv *env, jobject javaJobParams, wprint_job_params_t *wprintJobParams) {

    if (!javaJobParams || !wprintJobParams) {
        return ERROR;
    }

    jbyteArray nativeDataObject = (jbyteArray)(*env)->GetObjectField(env, javaJobParams, _wPrintJobParamsField__nativeData);
    if (!nativeDataObject != 0) {
        return ERROR;
    }

    char *nativeDataPtr = (*env)->GetByteArrayElements(env, nativeDataObject, NULL);
    memcpy(wprintJobParams, nativeDataPtr, sizeof(wprint_job_params_t));
    (*env)->ReleaseByteArrayElements(env, nativeDataObject, nativeDataPtr, JNI_ABORT);

    wprintJobParams->media_size    = (DF_media_size_t) (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__media_size);
    wprintJobParams->media_type    = (DF_media_type_t) (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__media_type);
    wprintJobParams->print_quality = (DF_quality_t)    (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__quality);
    wprintJobParams->duplex        = (DF_duplex_t)     (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__duplex);
    wprintJobParams->color_space   = (DF_color_space_t)(*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__color_space);
    wprintJobParams->media_tray    = (PT_media_src_t)  (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__media_tray);
    wprintJobParams->num_copies    = (unsigned int)    (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__num_copies);
    wprintJobParams->borderless    = (bool)            (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__borderless);
    wprintJobParams->render_flags  = (unsigned int)    (*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__render_flags);

    // job margin setting
    wprintJobParams->job_top_margin    = (float) (*env)->GetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_top);
    wprintJobParams->job_left_margin   = (float) (*env)->GetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_left);
    wprintJobParams->job_right_margin  = (float) (*env)->GetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_right);
    wprintJobParams->job_bottom_margin = (float) (*env)->GetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_bottom);

    if ((*env)->GetBooleanField(env, javaJobParams, _wPrintJobParamsField__portrait_mode)) {
        wprintJobParams->render_flags |= RENDER_FLAG_PORTRAIT_MODE;
    } else if ((*env)->GetBooleanField(env, javaJobParams, _wPrintJobParamsField__landscape_mode)) {
        wprintJobParams->render_flags |= RENDER_FLAG_LANDSCAPE_MODE;
    } else if ((*env)->GetBooleanField(env, javaJobParams, _wPrintJobParamsField__auto_rotate)) {
        wprintJobParams->render_flags |= RENDER_FLAG_AUTO_ROTATE;
    }
    if ((*env)->GetBooleanField(env, javaJobParams, _wPrintJobParamsField__fill_page)) {
        wprintJobParams->render_flags |= AUTO_SCALE_RENDER_FLAGS;
    } else if ((*env)->GetBooleanField(env, javaJobParams, _wPrintJobParamsField__fit_to_page)) {
        wprintJobParams->render_flags |= AUTO_FIT_RENDER_FLAGS;
    }

    int alignment = ((*env)->GetIntField(env, javaJobParams, _wPrintJobParamsField__alignment));
    if (alignment != 0) {
        wprintJobParams->render_flags &= ~(RENDER_FLAG_CENTER_VERTICAL | RENDER_FLAG_CENTER_HORIZONTAL | RENDER_FLAG_CENTER_ON_ORIENTATION);
        if (alignment & ((*env)->GetStaticIntField(env, _PrintServiceStringsClass, _PrintServiceStringsField__ALIGNMENT__CENTER_HORIZONTAL))) {
            wprintJobParams->render_flags |= RENDER_FLAG_CENTER_HORIZONTAL;
        }
        if (alignment & ((*env)->GetStaticIntField(env, _PrintServiceStringsClass, _PrintServiceStringsField__ALIGNMENT__CENTER_VERTICAL))) {
            wprintJobParams->render_flags |= RENDER_FLAG_CENTER_VERTICAL;
        }
        if (alignment & ((*env)->GetStaticIntField(env, _PrintServiceStringsClass, _PrintServiceStringsField__ALIGNMENT__CENTER_HORIZONTAL_ON_ORIENTATION))) {
            wprintJobParams->render_flags |= RENDER_FLAG_CENTER_ON_ORIENTATION;
        }
        if (alignment & ((*env)->GetStaticIntField(env, _PrintServiceStringsClass, _PrintServiceStringsField__ALIGNMENT__CENTER))) {
            wprintJobParams->render_flags &= ~RENDER_FLAG_CENTER_ON_ORIENTATION;
            wprintJobParams->render_flags |= (RENDER_FLAG_CENTER_VERTICAL | RENDER_FLAG_CENTER_HORIZONTAL);
        }
    }

    jstring docCategory = (jstring)(*env)->GetObjectField(env, javaJobParams, _wPrintJobParamsField__document_category);
    if (docCategory != NULL) {
        const char *category = (*env)->GetStringUTFChars(env, docCategory, NULL);
        if (category != NULL) {
            strncpy(wprintJobParams->docCategory, category, sizeof(wprintJobParams->docCategory) - 1);
			(*env)->ReleaseStringUTFChars(env, docCategory, (const char *)category);
        }
    }

    return OK;
}


static int _covertJobParams_to_Java(JNIEnv *env, jobject javaJobParams, wprint_job_params_t *wprintJobParams) {

    if (!javaJobParams || !wprintJobParams) {
        return ERROR;
    }

    jbyteArray nativeDataObject = (jbyteArray)(*env)->GetObjectField(env, javaJobParams, _wPrintJobParamsField__nativeData);
    if (!nativeDataObject) {
        nativeDataObject = (*env)->NewByteArray(env, sizeof(wprint_job_params_t));
        (*env)->SetObjectField(env, javaJobParams, _wPrintJobParamsField__nativeData, nativeDataObject);
        nativeDataObject = (jbyteArray)(*env)->GetObjectField(env, javaJobParams, _wPrintJobParamsField__nativeData);
    }

    char *nativeDataPtr = (*env)->GetByteArrayElements(env, nativeDataObject, NULL);
    memcpy(nativeDataPtr, wprintJobParams, sizeof(wprint_job_params_t));
    (*env)->ReleaseByteArrayElements(env, nativeDataObject, nativeDataPtr, 0);

    // update job parameters
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__media_size,   (int)wprintJobParams->media_size);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__media_type,   (int)wprintJobParams->media_type);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__quality,      (int)wprintJobParams->print_quality);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__duplex,       (int)wprintJobParams->duplex);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__color_space,  (int)wprintJobParams->color_space);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__media_tray,   (int)wprintJobParams->media_tray);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__num_copies,   (int)wprintJobParams->num_copies);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__borderless,   (int)wprintJobParams->borderless);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__render_flags, (int)wprintJobParams->render_flags);

    (*env)->SetBooleanField(env, javaJobParams, _wPrintJobParamsField__fit_to_page,    (jboolean)((wprintJobParams->render_flags & AUTO_FIT_RENDER_FLAGS) == AUTO_FIT_RENDER_FLAGS));
    (*env)->SetBooleanField(env, javaJobParams, _wPrintJobParamsField__fill_page,      (jboolean)((wprintJobParams->render_flags & AUTO_SCALE_RENDER_FLAGS) == AUTO_SCALE_RENDER_FLAGS));
    (*env)->SetBooleanField(env, javaJobParams, _wPrintJobParamsField__auto_rotate,    (jboolean)((wprintJobParams->render_flags & RENDER_FLAG_AUTO_ROTATE) != 0));
    (*env)->SetBooleanField(env, javaJobParams, _wPrintJobParamsField__portrait_mode,  (jboolean)((wprintJobParams->render_flags & RENDER_FLAG_PORTRAIT_MODE) != 0));
    (*env)->SetBooleanField(env, javaJobParams, _wPrintJobParamsField__landscape_mode, (jboolean)((wprintJobParams->render_flags & RENDER_FLAG_LANDSCAPE_MODE) != 0));
    
    // update the printable area & DPI information
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__print_resolution, (int)wprintJobParams->pixel_units);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__printable_width,  (int)wprintJobParams->width);
    (*env)->SetIntField(env, javaJobParams, _wPrintJobParamsField__printable_height, (int)wprintJobParams->height);

    // update the page size information
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__page_width,         wprintJobParams->page_width);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__page_height,        wprintJobParams->page_height);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__page_margin_top,    wprintJobParams->page_top_margin);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__page_margin_left,   wprintJobParams->page_left_margin);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__page_margin_right,  wprintJobParams->page_right_margin);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__page_margin_bottom, wprintJobParams->page_bottom_margin);

    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_top,    wprintJobParams->job_top_margin);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_left,   wprintJobParams->job_left_margin);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_right,  wprintJobParams->job_right_margin);
    (*env)->SetFloatField(env, javaJobParams, _wPrintJobParamsField__job_margin_bottom, wprintJobParams->job_bottom_margin);

    return OK;
}

static void _wprint_callback_fn(wJob_t job_handle, void *param) {
    jstring jStr;
    wprint_job_callback_params_t  *cb_param = (wprint_job_callback_params_t *)param;
    if (!cb_param) {
        return;
    }

    int needDetach = 0;
    JNIEnv *env;
    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0)
            return;
    }

    jobject callbackParams = (*env)->NewObject(env, _wPrintCallbackParamsClass, _wPrintCallbackParamsMethod__init);
    if (callbackParams != 0) {
        (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__callback_id, (jint)cb_param->id);

        (*env)->SetBooleanField(env, callbackParams, _wPrintCallbackParamsField__page_start_info, (jboolean)(cb_param->id == WPRINT_CB_PARAM_PAGE_START_INFO));

        if (cb_param->id == WPRINT_CB_PARAM_JOB_STATE) {

            const char *cStr;
            switch(cb_param->param.state) {
                case JOB_QUEUED:
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_STATE_QUEUED);
                    break;
                case JOB_RUNNING:
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_STATE_RUNNING);
                    break;
                case JOB_BLOCKED:
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_STATE_BLOCKED);
                    break;
                case JOB_DONE:
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_STATE_DONE);
                    break;
                default:
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_STATE_OTHER);
                    break;
            }
            (*env)->SetObjectField(env, callbackParams, _wPrintCallbackParamsField__job_state_str, jStr);

            if (cb_param->param.state == JOB_DONE) {

                switch(cb_param->job_done_result) {
                    case OK:
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_DONE_OK);
                        break;
                    case ERROR:
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_DONE_ERROR);
                        break;
                    case CANCELLED:
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_DONE_CANCELLED);
                        break;
                    case CORRUPT:
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_DONE_CORRUPT);
                        break;
                    default:
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__JOB_DONE_OTHER);
                        break;
                }
      
                (*env)->SetObjectField(env, callbackParams, _wPrintCallbackParamsField__job_done_result_str, jStr);
            }

            int i, count;
            for(count = i = 0; i < PRINT_STATUS_MAX_STATE; i++) {
                if (cb_param->blocked_reasons & (1 << i)) {
                    count++;
                }
            }

            if (count > 0) {
                jStr = (*env)->NewStringUTF(env, "");
                jobjectArray stringArray = (*env)->NewObjectArray(env, count, (*env)->FindClass(env, "java/lang/String"), jStr);
                (*env)->DeleteLocalRef(env, jStr);

                unsigned int blocked_reasons = cb_param->blocked_reasons;
                for(count = i = 0; i < PRINT_STATUS_MAX_STATE; i++) {
                    jStr = NULL;

                    if ((blocked_reasons & (1 << i)) == 0) {
                        jStr = NULL;
                    } else if (blocked_reasons & BLOCKED_REASON_UNABLE_TO_CONNECT) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OFFLINE);
                    } else if (blocked_reasons & BLOCKED_REASON_BUSY) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__BUSY);
                    } else if (blocked_reasons & BLOCKED_REASONS_CANCELLED) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__CANCELLED);
                    } else if (blocked_reasons & BLOCKED_REASON_OUT_OF_PAPER) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_PAPER);
                    } else if (blocked_reasons & BLOCKED_REASON_OUT_OF_INK) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_INK);
                    } else if (blocked_reasons & BLOCKED_REASON_OUT_OF_TONER) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_TONER);
                    } else if (blocked_reasons & BLOCKED_REASON_JAMMED) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__JAMMED);
                    } else if (blocked_reasons & BLOCKED_REASON_DOOR_OPEN) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__DOOR_OPEN);
                    } else if (blocked_reasons & BLOCKED_REASON_SVC_REQUEST) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__SERVICE_REQUEST);
                    } else if (blocked_reasons & BLOCKED_REASON_LOW_ON_INK) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_INK);
                    } else if (blocked_reasons & BLOCKED_REASON_LOW_ON_TONER) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_TONER);
                    } else if (blocked_reasons & BLOCKED_REASON_PRINT_STATUS_KINDA_LOW_ON_INK_PROBABLY_OUT_OF_INK_BUT_NOT_REALLY) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__REALLY_LOW_ON_INK);
                    } else if (blocked_reasons & BLOCKED_REASON_UNKNOWN) {
                        jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__UNKNOWN);
                    }

                    blocked_reasons &= ~(1 << i);

                    if (jStr != 0) {
                        (*env)->SetObjectArrayElement(env, stringArray, count++, jStr);
                    }
                }

                (*env)->SetObjectField(env, callbackParams, _wPrintCallbackParamsField__blocked_reasons_strings, stringArray);
            }

            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__job_state,       (jint)cb_param->param.state);
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__blocked_reasons, (jint)cb_param->blocked_reasons);
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__job_done_result, cb_param->job_done_result);
        } else {
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__page_num,          cb_param->param.page_info.page_num);
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__copy_num,          cb_param->param.page_info.copy_num);
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__corrupted_file,    cb_param->param.page_info.corrupted_file);
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__current_page,      cb_param->param.page_info.current_page);
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__total_pages,       cb_param->param.page_info.total_pages);
            (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__page_total_update, cb_param->param.page_info.page_total_update);
        }
        (*env)->SetIntField(env, callbackParams, _wPrintCallbackParamsField__job_id, (jint)job_handle);
        (*env)->CallVoidMethod(env, _callbackReceiver, _wPrintServiceJobHandlerMethod__jobCallback, (jint)job_handle, callbackParams);
        (*env)->DeleteLocalRef(env, callbackParams);
    }

    if (needDetach)
        (*_JVM)->DetachCurrentThread(_JVM);
}

static void _wprint_debug_callback_fn(int cond, const char *dbg_msg) {
    int level = ANDROID_LOG_SILENT;
    switch(cond) {
        case DBG_FORCE:
            level = ANDROID_LOG_FATAL;
            break;
        case DBG_ERROR:
            level = ANDROID_LOG_ERROR;
            break;
        case DBG_SESSION:
            level = ANDROID_LOG_WARN;
            break;
        case DBG_IO:
            level = ANDROID_LOG_INFO;
            break;
        case DBG_LOG:
            level = ANDROID_LOG_DEBUG;
            break;
        case DBG_VERBOSE:
            level = ANDROID_LOG_VERBOSE;
            break;
        default:
            level = ANDROID_LOG_SILENT;
            break;
    }
    if ((level != ANDROID_LOG_SILENT)  && (level >= _logLevel)) {
        __android_log_write(level, TAG, dbg_msg);
    }
}

JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeInit(JNIEnv *env, jobject obj, jobject callbackReceiver, jstring fakeDir) {

    int result;
    _initJNI(env, callbackReceiver, fakeDir);

    /* configure debugging */
    wprint_set_debug_level(DBG_ALL);
    wprint_set_stdlog(NULL);
    wprint_set_debug_callback(_wprint_debug_callback_fn);

    /* initialize wprint library */
    result = wprintInit();
    wprintSetOSImgIFC(&_jni_osimg_ifc);

    /* return the result */
    return result;
}


JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeGetCapabilities(JNIEnv *env, jobject obj, jstring address, jint port, jobject printerCaps) {
    jint result;
    printer_capabilities_t caps;

    const char *addressStr = (*env)->GetStringUTFChars(env, address, NULL);
    result = wprintGetCapabilities(addressStr, port, &caps);
    (*env)->ReleaseStringUTFChars(env, address, addressStr);

    // additional IPP checks 
    if ((result == 0) && (port == PORT_IPP)) {

        if (caps.isSupported && (caps.ippVersionMajor < 1)) {
            // ok it's passed the rigorous IPP version checks
            caps.isSupported = 0;
        }
    }

    _convertPrinterCaps_to_Java(env, printerCaps, &caps);

    return result;
}


JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeGetDefaultJobParameters(JNIEnv *env, jobject obj, jstring address, jint port, jobject jobParams, jobject printerCaps) {
    jint result;
    wprint_job_params_t params;
    printer_capabilities_t caps;

    _convertPrinterCaps_to_C(env, printerCaps, &caps);

    const char *addressStr = (*env)->GetStringUTFChars(env, address, NULL);
    result = wprintGetDefaultJobParams(addressStr, port, &params, &caps);
    (*env)->ReleaseStringUTFChars(env, address, addressStr);
    _covertJobParams_to_Java(env, jobParams, &params);

    return result;
}


JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeGetFinalJobParameters(JNIEnv *env, jobject obj, jstring address, jint port, jobject jobParams, jobject printerCaps) {
    jint result;
    wprint_job_params_t params;
    printer_capabilities_t caps;

    _covertJobParams_to_C(env, jobParams, &params);
    _convertPrinterCaps_to_C(env, printerCaps, &caps);
    
    const char *addressStr = (*env)->GetStringUTFChars(env, address, NULL);
    result = wprintGetFinalJobParams((const char*)addressStr, port, &params, &caps);
    (*env)->ReleaseStringUTFChars(env, address, addressStr);

    _covertJobParams_to_Java(env, jobParams, &params);

    return result;
}


JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeStartJob(JNIEnv *env, jobject obj, jstring address, jint port, jstring mimeType, jobject jobParams, jobject printerCaps, jobject fileArray, jstring jobDebugDir) {
    jint result = ERROR;
    wJob_t job_handle = ERROR;
    bool hasFiles = false;

    wprint_job_params_t params;
    printer_capabilities_t caps;

    _covertJobParams_to_C(env, jobParams, &params);
    _convertPrinterCaps_to_C(env, printerCaps, &caps);

    const char *addressStr = (*env)->GetStringUTFChars(env, address, NULL);
    const char *mimeTypeStr = (*env)->GetStringUTFChars(env, mimeType, NULL);
    const char *dataDirStr = (*env)->GetStringUTFChars(env, _fakeDir, NULL);

    if (fileArray) {
        hasFiles = ((*env)->GetArrayLength(env, (jobjectArray)fileArray) > 0);
    }

    if (hasFiles) {
        const char *jobDebugDirStr = NULL;
        if (jobDebugDir != NULL) {
            jobDebugDirStr = (*env)->GetStringUTFChars(env, jobDebugDir, NULL);
        }
        result = wprintStartJob((const char*)addressStr, port, &params, &caps, (char *)mimeTypeStr, (char*)dataDirStr, _wprint_callback_fn, jobDebugDirStr);
        if (result == ERROR) {
            ifprint((DBG_ERROR, "failed to start job: error code :%d", errno));
        }
        if ((jobDebugDir != NULL) && (jobDebugDirStr != NULL)) {
            (*env)->ReleaseStringUTFChars(env, jobDebugDir, jobDebugDirStr);
        }
    } else {
            ifprint((DBG_ERROR, "empty file list"));
    }
    if (result != ERROR) {
        job_handle = (wJob_t)result;
        // register job handle with service

        // add pages to the job allowing it to run
        int index, pageIndex, incrementor;
        jsize len;
        jobjectArray array = (jobjectArray)fileArray;
        len = (*env)->GetArrayLength(env, array);

        if (caps.hasFaceDownTray || params.duplex) {
            index       = 0;
            incrementor = 1;
        } else {
            index       = len - 1;
            incrementor = -1;
        }

        result = OK;
        for(pageIndex = 1; ((result == OK) && (pageIndex <= len)); pageIndex++) {
            jstring page = (jstring)(*env)->GetObjectArrayElement(env, array, index);
            const char *pageStr = (*env)->GetStringUTFChars(env, page, NULL);
            if (pageStr == NULL) {
                result = ERROR;
            } else {
                {
                    result = wprintPage(job_handle, pageIndex, (char*)pageStr, false, 0, 0, 0, 0);
                }
			}
			(*env)->ReleaseStringUTFChars(env, page, pageStr);
			index += incrementor;
		}

        wprintPage(job_handle, pageIndex, NULL, true, 0, 0, 0, 0);
        if (result != OK) {
            ifprint((DBG_ERROR, "failed to add some pages, aborting job"));
            wprintCancelJob(job_handle);
            wprintEndJob(job_handle);
            job_handle = ERROR;
        }
    }

    (*env)->ReleaseStringUTFChars(env, mimeType, mimeTypeStr);
    (*env)->ReleaseStringUTFChars(env, address, addressStr);
    (*env)->ReleaseStringUTFChars(env, _fakeDir, dataDirStr);
    return job_handle;
}


JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeEndJob(JNIEnv *env, jobject obj, jint job_handle) {
    return wprintEndJob((wJob_t)job_handle);
}


JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeCancelJob(JNIEnv *env, jobject obj, jint job_handle) {
    return wprintCancelJob((wJob_t)job_handle);
}


JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeResumeJob(JNIEnv *env, jobject obj, jint job_handle) {
    return wprintResumePrint((wJob_t)job_handle);
}


JNIEXPORT void JNICALL Java_com_android_wfds_jni_WprintJNI_nativeSetTimeouts(JNIEnv *env, jobject obj, jint tcp_timeout, jint udp_timeout, jint udp_retries) {
    wprintSetTimeouts((long int)tcp_timeout, (long int)udp_timeout, udp_retries);
}

static jobject _process_printer_status(JNIEnv *env, const printer_state_dyn_t *printer_status) {
    jobject statusObj = (*env)->NewObject(env, _wPrintCallbackParamsClass, _wPrintCallbackParamsMethod__init);
    if (statusObj != NULL) {
        print_status_t statusnew;
        unsigned int i, count, blocked_reasons;
        jstring jStr;

        jobject statsObj = (*env)->NewObject(env, _wPrintCallbackParamsClass, _wPrintCallbackParamsMethod__init);
        (*env)->SetBooleanField(env, statusObj, _wPrintCallbackParamsField__page_start_info, (jboolean)false);

        statusnew = printer_status->printer_status & ~PRINTER_IDLE_BIT;

        count = blocked_reasons = 0;
        for (count = i = 0; i < PRINT_STATUS_MAX_STATE; i++)
        {
            if (printer_status->printer_reasons[i] == PRINT_STATUS_MAX_STATE)
                break;
            if ((blocked_reasons & (1 << printer_status->printer_reasons[i])) == 0) {
                count++;
                blocked_reasons |= (1 << printer_status->printer_reasons[i]);
            }
    	}

        if (count > 0) {
            jStr = (*env)->NewStringUTF(env, "");
            jobjectArray stringArray = (*env)->NewObjectArray(env, count, (*env)->FindClass(env, "java/lang/String"), jStr);
            (*env)->DeleteLocalRef(env, jStr);

            for(count = i = 0; i < PRINT_STATUS_MAX_STATE; i++) {
                jStr = NULL;

                if ((blocked_reasons & (1 << i)) == 0) {
                    jStr = NULL;
                } else if (blocked_reasons & BLOCKED_REASON_UNABLE_TO_CONNECT) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OFFLINE);
                } else if (blocked_reasons & BLOCKED_REASON_BUSY) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__BUSY);
                } else if (blocked_reasons & BLOCKED_REASONS_CANCELLED) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__CANCELLED);
                } else if (blocked_reasons & BLOCKED_REASON_OUT_OF_PAPER) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_PAPER);
                } else if (blocked_reasons & BLOCKED_REASON_OUT_OF_INK) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_INK);
                } else if (blocked_reasons & BLOCKED_REASON_OUT_OF_TONER) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__OUT_OF_TONER);
                } else if (blocked_reasons & BLOCKED_REASON_JAMMED) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__JAMMED);
                } else if (blocked_reasons & BLOCKED_REASON_DOOR_OPEN) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__DOOR_OPEN);
                } else if (blocked_reasons & BLOCKED_REASON_SVC_REQUEST) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__SERVICE_REQUEST);
                } else if (blocked_reasons & BLOCKED_REASON_LOW_ON_INK) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_INK);
                } else if (blocked_reasons & BLOCKED_REASON_LOW_ON_TONER) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__LOW_ON_TONER);
                } else if (blocked_reasons & BLOCKED_REASON_PRINT_STATUS_KINDA_LOW_ON_INK_PROBABLY_OUT_OF_INK_BUT_NOT_REALLY) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__REALLY_LOW_ON_INK);
                } else if (blocked_reasons & BLOCKED_REASON_UNKNOWN) {
                    jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__BLOCKED_REASON__UNKNOWN);
                }

                blocked_reasons &= ~(1 << i);

                if (jStr != NULL) {
                    (*env)->SetObjectArrayElement(env, stringArray, count++, jStr);
                }
            }

            (*env)->SetObjectField(env, statusObj, _wPrintCallbackParamsField__blocked_reasons_strings, stringArray);
        }

        switch(statusnew) {
            case PRINT_STATUS_UNKNOWN:
                jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__PRINTER_STATE_UNKNOWN);
                break;
            case PRINT_STATUS_IDLE:
                jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__PRINTER_STATE_IDLE);
                break;
            case PRINT_STATUS_CANCELLED:
            case PRINT_STATUS_PRINTING:
                jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__PRINTER_STATE_RUNNING);
                break;
            default:
                jStr = (jstring)(*env)->GetStaticObjectField(env, _PrintServiceStringsClass, _PrintServiceStringsField__PRINTER_STATE_BLOCKED);
                break;
        }
        (*env)->SetObjectField(env, statusObj, _wPrintCallbackParamsField__printer_state_str, jStr);
    }
    return statusObj;
}

static void _printer_status_callback(const printer_state_dyn_t *new_status,
                                     const printer_state_dyn_t *old_status,
                                     void *param)
{
    int needDetach = 0;
    JNIEnv *env;
    if ((*_JVM)->GetEnv(_JVM, (void **) &env, JNI_VERSION_1_6) < 0) {
        needDetach = 1;
        if ((*_JVM)->AttachCurrentThread(_JVM, &env, NULL) < 0)
            return;
    }

    jobject receiver = (jobject)param;
    if (new_status->printer_status == PRINT_STATUS_UNKNOWN) {
       if (new_status->printer_reasons[0] == PRINT_STATUS_INITIALIZING) {
           receiver = NULL;
       } else if (new_status->printer_reasons[0] == PRINT_STATUS_SHUTTING_DOWN) {
           if (receiver != NULL) {
               (*env)->DeleteGlobalRef(env, receiver);
           }
           receiver = NULL;
       }
    }

    if (receiver != NULL) {
        jobject statusObj = _process_printer_status(env, new_status);
        (*env)->CallVoidMethod(env, receiver, _WPrintPrinterStatusMonitorMethod__callbackReceiver, statusObj);
    }

    if (needDetach)
        (*_JVM)->DetachCurrentThread(_JVM);
}

JNIEXPORT jobject JNICALL Java_com_android_wfds_jni_WprintJNI_nativeGetPrinterStatus(JNIEnv *env, jobject obj, jstring address, jint port) {
    printer_state_dyn_t printer_status;
    jobject statusObj = NULL;
    const char *addressStr = (*env)->GetStringUTFChars(env, address, NULL);

    int result = wprintGetPrinterStatus(addressStr, port, &printer_status);
    if (!(result < 0)) {
        statusObj = _process_printer_status(env, &printer_status);
    }
    return statusObj;
}

JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeMonitorStatusSetup(JNIEnv *env, jobject obj, jstring address, jint port) {
    wStatus_t status_handle;
    const char *addressStr = (*env)->GetStringUTFChars(env, address, NULL);
    status_handle = wprintStatusMonitorSetup(addressStr, port);
    return (jint)status_handle;
}

JNIEXPORT void JNICALL Java_com_android_wfds_jni_WprintJNI_nativeMonitorStatusStart(JNIEnv *env, jobject obj, jint statusID, jobject receiver) {
    wStatus_t status_handle = (wStatus_t)statusID;
    if (status_handle != 0) {
        if (receiver != NULL) {
            receiver = (*env)->NewGlobalRef(env, receiver);
        }
        if (wprintStatusMonitorStart(status_handle, _printer_status_callback, receiver) != OK) {
            if (receiver != NULL) {
                (*env)->DeleteGlobalRef(env, receiver);
         }
        }
    }
}

JNIEXPORT void JNICALL Java_com_android_wfds_jni_WprintJNI_nativeMonitorStatusStop(JNIEnv *env, jobject obj, jint statusID) {
    wStatus_t status_handle = (wStatus_t)statusID;
    if (status_handle != 0) {
        wprintStatusMonitorStop(status_handle);
    }
}

JNIEXPORT jint JNICALL Java_com_android_wfds_jni_WprintJNI_nativeExit(JNIEnv *env, jobject obj) {

    (*env)->DeleteGlobalRef(env, _wPrintJobParamsClass);
    (*env)->DeleteGlobalRef(env, _wPrintPrinterCapabilitiesClass);
    (*env)->DeleteGlobalRef(env, _wPrintCallbackParamsClass);
    if (_callbackReceiver)
        (*env)->DeleteGlobalRef(env, _callbackReceiver);
    if (_wPrintServiceJobHandlerClass)
        (*env)->DeleteGlobalRef(env, _wPrintServiceJobHandlerClass);
    (*env)->DeleteGlobalRef(env, _fakeDir);
    (*env)->DeleteGlobalRef(env, _WPrintPrinterStatusMonitorClass);
    (*env)->DeleteGlobalRef(env, _PrintServiceStringsClass);
    (*env)->DeleteGlobalRef(env, _BitmapClass);
    (*env)->DeleteGlobalRef(env, _BitmapFactoryClass);
    (*env)->DeleteGlobalRef(env, _BitmapConfigClass);
    (*env)->DeleteGlobalRef(env, _BitmapFactoryOptionsClass);
    (*env)->DeleteGlobalRef(env, _FileDescriptorClass);
    (*env)->DeleteGlobalRef(env, _ColorClass);

    return wprintExit();
}

JNIEXPORT void JNICALL Java_com_android_wfds_jni_WprintJNI_nativeSetApplicationID(JNIEnv *env, jobject obj, jstring id) {
    if (id) {
        const char *idStr = (*env)->GetStringUTFChars(env, id, NULL);
        wprintSetApplicationID(idStr);
        (*env)->ReleaseStringUTFChars(env, id, idStr);
    }
}

JNIEXPORT void JNICALL Java_com_android_wfds_jni_WprintJNI_nativeSetLogLevel(JNIEnv *env, jobject obj, jint logLevel) {
    if (logLevel < ANDROID_LOG_UNKNOWN) {
        logLevel = ANDROID_LOG_UNKNOWN;
    } else if (logLevel > ANDROID_LOG_SILENT) {
        logLevel = ANDROID_LOG_SILENT;
    }
    _logLevel = logLevel;
}
