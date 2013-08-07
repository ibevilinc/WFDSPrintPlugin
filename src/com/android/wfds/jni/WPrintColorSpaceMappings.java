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

public final class WPrintColorSpaceMappings {

    private static final ArrayList<Pair<Integer,String>> mColorSpacePairs;
    static {
        mColorSpacePairs = new ArrayList<Pair<Integer, String>>();
        mColorSpacePairs.add(Pair.create(Integer.valueOf(0), MobilePrintConstants.COLOR_SPACE_MONOCHROME));
        mColorSpacePairs.add(Pair.create(Integer.valueOf(1), MobilePrintConstants.COLOR_SPACE_COLOR));
    }

    public static String getColorSpaceName(int colorSpaceValue) {
        ListIterator<Pair<Integer, String>> iter = mColorSpacePairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.first.intValue() == colorSpaceValue) {
                return entry.second;
            }
        }
        return null;
    }

    public static int getColorSpaceValue(String colorSpaceName) {
        ListIterator<Pair<Integer, String>> iter = mColorSpacePairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.second.equals(colorSpaceName)) {
                return entry.first;
            }
        }
        return -1;
    }

    public static ArrayList<String> getColorSpaceNames(boolean canPrintColor) {
        ArrayList<String> colorSpaceList = new ArrayList<String>();
        colorSpaceList.add(MobilePrintConstants.COLOR_SPACE_MONOCHROME);
        if (canPrintColor)
            colorSpaceList.add(MobilePrintConstants.COLOR_SPACE_COLOR);

        return colorSpaceList;
    }
}
