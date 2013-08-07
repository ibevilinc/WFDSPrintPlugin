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
package com.android.wfds.jni;

import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import java.lang.Math;

import com.hp.android.printplugin.support.PrintServiceStrings;
import com.android.wfds.common.IDefaultJobParams;
import com.android.wfds.common.MobilePrintConstants;

public final class wPrintJobParams implements IDefaultJobParams {

    public int borderless;
    public int duplex;

    public int media_size;
    public int media_type;
    public int media_tray;

    public int quality;
    public int render_flags;
    public int num_copies;
    public int color_space;

    public int print_resolution;
    public int printable_width;
    public int printable_height;

    public float job_margin_top;
    public float job_margin_left;
    public float job_margin_right;
    public float job_margin_bottom;

    public float page_width;
    public float page_height;
    public float page_margin_top;
    public float page_margin_left;
    public float page_margin_right;
    public float page_margin_bottom;

    public boolean fit_to_page;
    public boolean auto_rotate;
    public boolean fill_page;
    public boolean portrait_mode;
    public boolean landscape_mode;

    public String page_range = null;
    public String document_category = null;

    public byte[] nativeData = null;

    public int alignment = 0;

    public wPrintJobParams(wPrintJobParams other) {

        borderless = other.borderless;
        duplex = other.duplex;
        media_size = other.media_size;
        media_type = other.media_type;
        media_tray = other.media_tray;
        quality = other.quality;
        render_flags = other.render_flags;
        num_copies = other.num_copies;
        color_space = other.color_space;
        print_resolution = other.print_resolution;
        printable_width = other.printable_width;
        printable_height = other.printable_height;
        page_width = other.page_width;
        page_height = other.page_height;
        page_margin_top = other.page_margin_top;
        page_margin_bottom = other.page_margin_bottom;
        page_margin_left = other.page_margin_left;
        page_margin_right = other.page_margin_right;
        page_range = other.page_range;
        job_margin_top = other.job_margin_top;
        job_margin_bottom = other.job_margin_bottom;
        job_margin_left = other.job_margin_left;
        job_margin_right = other.job_margin_right;

        if (other.nativeData != null) {
            nativeData = new byte[other.nativeData.length];
            System.arraycopy(other.nativeData, 0, nativeData, 0, nativeData.length);
        }
    }

    public wPrintJobParams() {
    }

    public void updateJobParams(final Bundle settings) {
        if (settings.containsKey(MobilePrintConstants.FULL_BLEED)) {
            try {
                String fullbleed = settings.getString(MobilePrintConstants.FULL_BLEED);
                if(!TextUtils.isEmpty(fullbleed)) {
                    borderless = fullbleed.equals(MobilePrintConstants.FULL_BLEED_ON) ? 1 : 0;
                } else {
                    borderless = 0;
                }
            }
            catch(Exception e) {
                borderless = 0;
                Log.e("PRINT_LIB", "Exception caught setting bordless, defaulting to 0");
            }
        }
        if (settings.containsKey(MobilePrintConstants.SIDES)) {
            duplex = WPrintDuplexMappings.getDuplexValue(settings.getString(MobilePrintConstants.SIDES));
        }
        if (settings.containsKey(MobilePrintConstants.COPIES)) {
            num_copies = settings.getInt(MobilePrintConstants.COPIES);
        }
        if (settings.containsKey(MobilePrintConstants.PRINT_QUALITY)) {
            quality = WPrintQualityMappings.getQualityValue(settings.getString(MobilePrintConstants.PRINT_QUALITY));
        }
        if (settings.containsKey(MobilePrintConstants.FIT_TO_PAGE)) {
            fit_to_page = settings.getBoolean(MobilePrintConstants.FIT_TO_PAGE);
        }
        if (settings.containsKey(MobilePrintConstants.FILL_PAGE)) {
            fill_page = settings.getBoolean(MobilePrintConstants.FILL_PAGE);
        }

        String orientation = settings.getString(MobilePrintConstants.ORIENTATION_REQUESTED);
        if (TextUtils.isEmpty(orientation)) {
            orientation = MobilePrintConstants.ORIENTATION_AUTO;
        }
        if (orientation.equals(MobilePrintConstants.ORIENTATION_PORTRAIT)) {
            auto_rotate = false;
            portrait_mode = true;
            landscape_mode = false;
        } else if (orientation.equals(MobilePrintConstants.ORIENTATION_LANDSCAPE)) {
            auto_rotate = false;
            portrait_mode = false;
            landscape_mode = true;
        } else {
            auto_rotate = true;
            portrait_mode = false;
            landscape_mode = false;
        }

        if (settings.containsKey(MobilePrintConstants.MEDIA_SIZE_NAME)) {
            media_size = WPrintPaperSizeMappings.getPaperSizeValue(settings.getString(MobilePrintConstants.MEDIA_SIZE_NAME));
        }
        if (settings.containsKey(MobilePrintConstants.MEDIA_TYPE)) {
            media_type = WPrintPaperTypeMappings.getPaperTypeValue(settings.getString(MobilePrintConstants.MEDIA_TYPE));
        }
        if (settings.containsKey(MobilePrintConstants.MEDIA_SOURCE)) {
            media_tray = WPrintPaperTrayMappings.getPaperTrayValue(settings.getString(MobilePrintConstants.MEDIA_SOURCE));
        }
        if (settings.containsKey(MobilePrintConstants.PRINT_COLOR_MODE)) {
            color_space = WPrintColorSpaceMappings.getColorSpaceValue(settings.getString(MobilePrintConstants.PRINT_COLOR_MODE));
        }
        if (settings.containsKey(MobilePrintConstants.PAGE_RANGE)) {
            page_range = settings.getString(MobilePrintConstants.PAGE_RANGE);
        }

        if (settings.containsKey(MobilePrintConstants.PRINT_DOCUMENT_CATEGORY)) {
            document_category = settings.getString(MobilePrintConstants.PRINT_DOCUMENT_CATEGORY);
        }

        job_margin_top    = Math.max(settings.getFloat(MobilePrintConstants.JOB_MARGIN_TOP,    0.0f), 0.0f);
        job_margin_left   = Math.max(settings.getFloat(MobilePrintConstants.JOB_MARGIN_LEFT,   0.0f), 0.0f);
        job_margin_right  = Math.max(settings.getFloat(MobilePrintConstants.JOB_MARGIN_RIGHT,  0.0f), 0.0f);
        job_margin_bottom = Math.max(settings.getFloat(MobilePrintConstants.JOB_MARGIN_BOTTOM, 0.0f), 0.0f);

        if (settings.containsKey(PrintServiceStrings.ALIGNMENT)) {
            alignment = settings.getInt(PrintServiceStrings.ALIGNMENT);
        } else if (!TextUtils.isEmpty(document_category)) {
            if (document_category.equals(PrintServiceStrings.PRINT_DOCUMENT_CATEGORY__PHOTO)) {
                alignment = PrintServiceStrings.ALIGN_CENTER;
            } else if (document_category.equals(PrintServiceStrings.PRINT_DOCUMENT_CATEGORY__DOCUMENT)) {
                alignment = PrintServiceStrings.ALIGN_CENTER_HORIZONTAL_ON_ORIENTATION;
            }
        }
    }

    public Bundle getJobParams() {
        Bundle settings = new Bundle();
        settings.putString(MobilePrintConstants.SIDES, WPrintDuplexMappings.getDuplexName(duplex));
        settings.putInt(MobilePrintConstants.COPIES, num_copies);
        settings.putString(MobilePrintConstants.PRINT_QUALITY, WPrintQualityMappings.getQualityName(quality));
        settings.putBoolean(MobilePrintConstants.FIT_TO_PAGE, fit_to_page);
        settings.putBoolean(MobilePrintConstants.FILL_PAGE, fill_page);
        if (auto_rotate) {
            settings.putString(MobilePrintConstants.ORIENTATION_REQUESTED, MobilePrintConstants.ORIENTATION_AUTO);
        } else if (landscape_mode) {
            settings.putString(MobilePrintConstants.ORIENTATION_REQUESTED, MobilePrintConstants.ORIENTATION_LANDSCAPE);
        } else if (portrait_mode) {
            settings.putString(MobilePrintConstants.ORIENTATION_REQUESTED, MobilePrintConstants.ORIENTATION_PORTRAIT);
        }

        settings.putString(MobilePrintConstants.MEDIA_SIZE_NAME, WPrintPaperSizeMappings.getPaperSizeName(media_size));
        settings.putString(MobilePrintConstants.MEDIA_TYPE, WPrintPaperTypeMappings.getPaperTypeName(media_type));
        settings.putString(MobilePrintConstants.MEDIA_SOURCE, WPrintPaperTrayMappings.getPaperTrayName(media_tray));
        settings.putString(MobilePrintConstants.PRINT_COLOR_MODE, WPrintColorSpaceMappings.getColorSpaceName(color_space));
        settings.putString(MobilePrintConstants.FULL_BLEED, (borderless != 0 ? MobilePrintConstants.FULL_BLEED_ON : MobilePrintConstants.FULL_BLEED_OFF));
        settings.putString(MobilePrintConstants.PAGE_RANGE, page_range);

        settings.putInt(MobilePrintConstants.PRINT_RESOLUTION,       print_resolution);
        settings.putInt(MobilePrintConstants.PRINTABLE_PIXEL_WIDTH,  printable_width);
        settings.putInt(MobilePrintConstants.PRINTABLE_PIXEL_HEIGHT, printable_height);

        settings.putFloat(MobilePrintConstants.PAGE_WIDTH,         page_width);
        settings.putFloat(MobilePrintConstants.PAGE_HEIGHT,        page_height);
        settings.putFloat(MobilePrintConstants.PAGE_MARGIN_TOP,    page_margin_top);
        settings.putFloat(MobilePrintConstants.PAGE_MARGIN_LEFT,   page_margin_left);
        settings.putFloat(MobilePrintConstants.PAGE_MARGIN_RIGHT,  page_margin_right);
        settings.putFloat(MobilePrintConstants.PAGE_MARGIN_BOTTOM, page_margin_bottom);

        return settings;
    }
}
