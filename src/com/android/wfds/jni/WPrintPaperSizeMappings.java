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

import android.text.TextUtils;
import android.util.Pair;

import com.android.wfds.common.MobilePrintConstants;

public final class WPrintPaperSizeMappings {

    private static final ArrayList<Pair<Integer,String>> mPaperSizePairs;
    static {
        mPaperSizePairs = new ArrayList<Pair<Integer, String>>();
        mPaperSizePairs.add(Pair.create(Integer.valueOf(26),  MobilePrintConstants.MEDIA_SIZE_A4));
        mPaperSizePairs.add(Pair.create(Integer.valueOf(2),   MobilePrintConstants.MEDIA_SIZE_LETTER));
        mPaperSizePairs.add(Pair.create(Integer.valueOf(3),   MobilePrintConstants.MEDIA_SIZE_LEGAL));
        mPaperSizePairs.add(Pair.create(Integer.valueOf(74),  MobilePrintConstants.MEDIA_SIZE_PHOTO_4x6in));
        mPaperSizePairs.add(Pair.create(Integer.valueOf(118), MobilePrintConstants.MEDIA_SIZE_PHOTO_10x15cm));
        mPaperSizePairs.add(Pair.create(Integer.valueOf(121), MobilePrintConstants.MEDIA_SIZE_PHOTO_L));
        mPaperSizePairs.add(Pair.create(Integer.valueOf(78),  MobilePrintConstants.MEDIA_SIZE_PHOTO_3_5x5));
        mPaperSizePairs.add(Pair.create(Integer.valueOf(122), MobilePrintConstants.MEDIA_SIZE_PHOTO_5x7));
    }

    public static String getPaperSizeName(int paperSizeValue) {
        ListIterator<Pair<Integer, String>> iter = mPaperSizePairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.first.intValue() == paperSizeValue) {
                return entry.second;
            }
        }
        return null;
    }

    public static int getPaperSizeValue(String paperSizeName) {
        ListIterator<Pair<Integer, String>> iter = mPaperSizePairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.second.equals(paperSizeName)) {
                return entry.first;
            }
        }
        return -1;
    }

    public static ArrayList<String> getPaperSizeNames(int[] paperSizes) {
        ArrayList<String> paperSizeList = new ArrayList<String>();
        for(int i = 0; ((paperSizes != null) && (i < paperSizes.length)); i++) {
            String name = getPaperSizeName(paperSizes[i]);
            if (!TextUtils.isEmpty(name) && !paperSizeList.contains(name)) {
                paperSizeList.add(name);
            }
        }

        if (paperSizeList.isEmpty()) {
            paperSizeList = null;
        }
        return paperSizeList;
    }
}
