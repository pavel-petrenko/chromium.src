// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.features.start_surface;

import androidx.annotation.Nullable;

import org.chromium.chrome.browser.native_page.NativePageNavigationDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tasks.ReturnToChromeExperimentsUtil;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.mojom.WindowOpenDisposition;

/** Implementation of the {@link NativePageNavigationDelegate} for the explore surface. */
class ExploreSurfaceNavigationDelegate implements NativePageNavigationDelegate {
    private static final String NEW_TAB_URL_HELP = "https://support.google.com/chrome/?p=new_tab";

    ExploreSurfaceNavigationDelegate() {}

    @Override
    public boolean isOpenInNewWindowEnabled() {
        return false;
    }

    @Override
    @Nullable
    public Tab openUrl(int windowOpenDisposition, LoadUrlParams loadUrlParams) {
        boolean result = ReturnToChromeExperimentsUtil.willHandleLoadUrlFromStartSurface(
                loadUrlParams.getUrl(), PageTransition.AUTO_BOOKMARK,
                windowOpenDisposition == WindowOpenDisposition.OFF_THE_RECORD);
        assert result;
        return null;
    }

    @Override
    public void navigateToHelpPage() {
        openUrl(WindowOpenDisposition.CURRENT_TAB,
                new LoadUrlParams(NEW_TAB_URL_HELP, PageTransition.AUTO_BOOKMARK));
    }
}
