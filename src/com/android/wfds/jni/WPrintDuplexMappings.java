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

public final class WPrintDuplexMappings {

    private static final ArrayList<Pair<Integer,String>> mDuplexPairs;
    static {
        mDuplexPairs = new ArrayList<Pair<Integer, String>>();
        mDuplexPairs.add(Pair.create(Integer.valueOf(0), MobilePrintConstants.SIDES_SIMPLEX));
        mDuplexPairs.add(Pair.create(Integer.valueOf(1), MobilePrintConstants.SIDES_DUPLEX_LONG_EDGE));
        mDuplexPairs.add(Pair.create(Integer.valueOf(2), MobilePrintConstants.SIDES_DUPLEX_SHORT_EDGE));
    }

    public static String getDuplexName(int duplexValue) {
        ListIterator<Pair<Integer, String>> iter = mDuplexPairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.first.intValue() == duplexValue) {
                return entry.second;
            }
        }
        return null;
    }

    public static int getDuplexValue(String duplexName) {
        ListIterator<Pair<Integer, String>> iter = mDuplexPairs.listIterator();
        while(iter.hasNext()) {
            Pair<Integer,String> entry = iter.next();
            if (entry.second.equals(duplexName)) {
                return entry.first;
            }
        }
        return -1;
    }

    public static ArrayList<String> getDuplexNames(boolean canDuplex) {
        ArrayList<String> duplexList = new ArrayList<String>();
        duplexList.add(MobilePrintConstants.SIDES_SIMPLEX);
        if (canDuplex) {
            duplexList.add(MobilePrintConstants.SIDES_DUPLEX_LONG_EDGE);
            duplexList.add(MobilePrintConstants.SIDES_DUPLEX_SHORT_EDGE);
        }

        return duplexList;
    }
}
