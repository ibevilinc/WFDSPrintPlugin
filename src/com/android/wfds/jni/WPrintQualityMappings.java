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
import java.util.ListIterator;

import android.util.Pair;

import com.android.wfds.common.MobilePrintConstants;

public final class WPrintQualityMappings {

    private static final ArrayList<Pair<Integer,String>> mQualityPairs;
    static {
        mQualityPairs = new ArrayList<Pair<Integer, String>>();
        mQualityPairs.add(Pair.create(Integer.valueOf(0), MobilePrintConstants.PRINT_QUALITY_DRAFT));
        mQualityPairs.add(Pair.create(Integer.valueOf(1), MobilePrintConstants.PRINT_QUALITY_NORMAL));
        mQualityPairs.add(Pair.create(Integer.valueOf(2), MobilePrintConstants.PRINT_QUALITY_BEST));
    }

    public static String getQualityName(int qualityValue) {
        ListIterator<Pair<Integer, String>> iter = mQualityPairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.first.intValue() == qualityValue) {
                return entry.second;
            }
        }
        return null;
    }

    public static int getQualityValue(String qualityName) {
        ListIterator<Pair<Integer, String>> iter = mQualityPairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.second.equals(qualityName)) {
                return entry.first;
            }
        }
        return -1;
    }

    public static ArrayList<String> getQualityNames(boolean supports_draft, boolean supports_normal, boolean supports_best) {
        ArrayList<String> qualityList = new ArrayList<String>();
        if (supports_draft)
            qualityList.add(MobilePrintConstants.PRINT_QUALITY_DRAFT);
        if (supports_normal)
            qualityList.add(MobilePrintConstants.PRINT_QUALITY_NORMAL);
        if (supports_best)
            qualityList.add(MobilePrintConstants.PRINT_QUALITY_BEST);

        if (qualityList.isEmpty()) {
            qualityList = null;
        }
        return qualityList;
    }
}
