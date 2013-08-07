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
package com.android.wfds.printplugin;

import java.io.IOException;
import java.io.InputStreamReader;

import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class LegalNotice extends Activity {

    private static final String LICENSE_FILE_NAME = "legal_notice.html";

    private String loadLicenseText() {
        InputStreamReader reader = null;
        String txt = null;
        try {
            reader = new InputStreamReader(getAssets().open(LICENSE_FILE_NAME));
            StringBuilder sb = new StringBuilder(2048);
            char buffer[] = new char[2048];
            int n;
            while ((n = reader.read(buffer)) >= 0) {
                sb.append(buffer, 0, n);
            }
            if (sb.length() > 0) {
                txt = sb.toString();
            }
        } catch (IOException e) {

        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch(IOException e) {

                }
            }
        }
        return txt;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        WebView webView = new WebView(this);
        String text = loadLicenseText();
        if (!TextUtils.isEmpty(text)) {
            webView.loadDataWithBaseURL(null, text, "text/html", "utf-8", null);
        }
        webView.setWebViewClient(new WebViewClient());
        setContentView(webView);
    }

}
