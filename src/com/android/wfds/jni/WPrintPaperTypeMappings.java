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

public final class WPrintPaperTypeMappings {

    private static final ArrayList<Pair<Integer,String>> mPaperTypePairs;
    static {
        mPaperTypePairs = new ArrayList<Pair<Integer, String>>();
        mPaperTypePairs.add(Pair.create(Integer.valueOf(0),  MobilePrintConstants.MEDIA_TYPE_AUTO));
        mPaperTypePairs.add(Pair.create(Integer.valueOf(10), MobilePrintConstants.MEDIA_TYPE_PHOTO_GLOSSY));
    }

    public static String getPaperTypeName(int paperTypeValue) {
        ListIterator<Pair<Integer, String>> iter = mPaperTypePairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.first.intValue() == paperTypeValue) {
                return entry.second;
            }
        }
        return null;
    }

    public static int getPaperTypeValue(String paperTypeName) {
        ListIterator<Pair<Integer, String>> iter = mPaperTypePairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.second.equals(paperTypeName)) {
                return entry.first;
            }
        }
        return -1;
    }

    public static ArrayList<String> getPaperTypeNames(int[] paperTypes) {
        ArrayList<String> paperTypeList = new ArrayList<String>();
        for(int i = 0; ((paperTypes != null) && (i < paperTypes.length)); i++) {
            String name = getPaperTypeName(paperTypes[i]);
            if (!TextUtils.isEmpty(name) && !paperTypeList.contains(name)) {
                paperTypeList.add(name);
            }
        }

        if (paperTypeList.isEmpty()) {
            paperTypeList = null;
        }
        return paperTypeList;
    }
}
