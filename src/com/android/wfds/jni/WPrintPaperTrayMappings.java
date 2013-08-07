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

public final class WPrintPaperTrayMappings {

    private static final ArrayList<Pair<Integer,String>> mPaperTrayPairs;
    static {
        mPaperTrayPairs = new ArrayList<Pair<Integer, String>>();
        mPaperTrayPairs.add(Pair.create(Integer.valueOf(7), MobilePrintConstants.MEDIA_TRAY_AUTO));
        mPaperTrayPairs.add(Pair.create(Integer.valueOf(6), MobilePrintConstants.MEDIA_TRAY_PHOTO));
    }

    public static String getPaperTrayName(int paperSizeValue) {
        ListIterator<Pair<Integer, String>> iter = mPaperTrayPairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.first.intValue() == paperSizeValue) {
                return entry.second;
            }
        }
        return null;
    }

    public static int getPaperTrayValue(String paperTrayName) {
        ListIterator<Pair<Integer, String>> iter = mPaperTrayPairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.second.equals(paperTrayName)) {
                return entry.first;
            }
        }
        return -1;
    }

    public static ArrayList<String> getPaperTrayNames(int[] paperTrays) {
        ArrayList<String> paperTrayList = new ArrayList<String>();
        for(int i = 0; ((paperTrays != null) && (i < paperTrays.length)); i++) {
            String name = getPaperTrayName(paperTrays[i]);
            if (!TextUtils.isEmpty(name) && !paperTrayList.contains(name)) {
                paperTrayList.add(name);
            }
        }

        if (paperTrayList.isEmpty()) {
            paperTrayList = null;
        }
        return paperTrayList;
    }
}
