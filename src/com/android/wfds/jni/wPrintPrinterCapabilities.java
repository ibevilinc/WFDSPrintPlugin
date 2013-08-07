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

import java.util.ArrayList;

import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;

import com.android.wfds.common.IPrinterCapabilities;
import com.android.wfds.common.MobilePrintConstants;

public final class wPrintPrinterCapabilities implements IPrinterCapabilities {

    public boolean can_duplex;
    public boolean has_photo_tray;
    public boolean can_print_borderless;
    public boolean can_print_color;

    public boolean supports_quality_draft;
    public boolean supports_quality_normal;
    public boolean supports_quality_best;
    public boolean has_facedown_tray;

    public String printer_name;
    public String printer_make;

    public String suppliesURI;

    public String[] supported_mime_types;
    public int[] supported_media_trays;
    public int[] supported_media_types;
    public int[] supported_media_sizes;

    public boolean is_supported;
    public boolean can_cancel;

    public byte[] nativeData;

    public wPrintPrinterCapabilities() {
    }

    @Override
    public Uri getSuppliesURI() {
        if (!TextUtils.isEmpty(suppliesURI)) {
            return Uri.parse(suppliesURI);
        }
        return null;
    }

    public Bundle getPrinterCapabilities() {
        Bundle caps = new Bundle();
        caps.putString(MobilePrintConstants.PRINTER_NAME, printer_name);
        caps.putString(MobilePrintConstants.PRINTER_MAKE_MODEL, printer_make);

        caps.putStringArrayList(MobilePrintConstants.PRINT_COLOR_MODE, WPrintColorSpaceMappings.getColorSpaceNames(can_print_color));

        caps.putStringArrayList(MobilePrintConstants.SIDES, WPrintDuplexMappings.getDuplexNames(can_duplex));
        caps.putBoolean(MobilePrintConstants.SUPPORTS_CANCEL, can_cancel);
        caps.putBoolean(MobilePrintConstants.IS_SUPPORTED, is_supported);
        caps.putBoolean(MobilePrintConstants.FULL_BLEED_SUPPORTED, can_print_borderless);

        ArrayList<String> mediaSizeList = WPrintPaperSizeMappings.getPaperSizeNames(supported_media_sizes);
        if (mediaSizeList != null) {
            caps.putStringArrayList(MobilePrintConstants.MEDIA_SIZE_NAME, mediaSizeList);
        }

        ArrayList<String> mediaTrayList = WPrintPaperTrayMappings.getPaperTrayNames(supported_media_trays);
        if (mediaTrayList != null) {
            caps.putStringArrayList(MobilePrintConstants.MEDIA_SOURCE, mediaTrayList);
        }

        ArrayList<String> mediaTypeList = WPrintPaperTypeMappings.getPaperTypeNames(supported_media_types);
        if (mediaTypeList != null) {
            caps.putStringArrayList(MobilePrintConstants.MEDIA_TYPE, mediaTypeList);
        }

        ArrayList<String> qualityList = WPrintQualityMappings.getQualityNames(supports_quality_draft, supports_quality_normal, supports_quality_best);
        if (qualityList != null)
            caps.putStringArrayList(MobilePrintConstants.PRINT_QUALITY, qualityList);

        if (supported_mime_types != null) {
            ArrayList<String> mimeTypes = new ArrayList<String>(supported_mime_types.length);
            for(int i = 0; i < supported_mime_types.length; i++) {
                mimeTypes.add(supported_mime_types[i]);
            }
            caps.putStringArrayList(MobilePrintConstants.MIME_TYPES, mimeTypes);
        }

        return caps;
    }
}
