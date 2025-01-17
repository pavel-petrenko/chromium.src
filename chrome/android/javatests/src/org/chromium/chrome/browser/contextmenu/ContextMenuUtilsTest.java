// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import static org.junit.Assert.assertEquals;

import android.webkit.URLUtil;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.blink_public.common.ContextMenuDataMediaType;
import org.chromium.components.embedder_support.contextmenu.ContextMenuParams;

/**
 * Unit tests for {@link ContextMenuUtils}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class ContextMenuUtilsTest {
    private static final String sTitleText = "titleText";
    private static final String sLinkText = "linkText";
    private static final String sSrcUrl = "https://www.google.com/";

    @Test
    @SmallTest
    public void getTitle_hasTitleText() {
        ContextMenuParams params = new ContextMenuParams(0,
                org.chromium.blink_public.common.ContextMenuDataMediaType.IMAGE, "", "", sLinkText,
                "", sSrcUrl, sTitleText, null, false, 0, 0, 0);

        assertEquals(sTitleText, ContextMenuUtils.getTitle(params));
    }

    @Test
    @SmallTest
    public void getTitle_noTitleTextHasLinkText() {
        ContextMenuParams params = new ContextMenuParams(0, ContextMenuDataMediaType.IMAGE, "", "",
                sLinkText, "", sSrcUrl, "", null, false, 0, 0, 0);

        assertEquals(sLinkText, ContextMenuUtils.getTitle(params));
    }

    @Test
    @SmallTest
    public void getTitle_noTitleTextOrLinkText() {
        ContextMenuParams params = new ContextMenuParams(0, ContextMenuDataMediaType.IMAGE, "", "",
                "", "", sSrcUrl, "", null, false, 0, 0, 0);

        assertEquals(URLUtil.guessFileName(sSrcUrl, null, null), ContextMenuUtils.getTitle(params));
    }

    @Test
    @SmallTest
    public void getTitle_noShareParams() {
        ContextMenuParams params = new ContextMenuParams(
                0, ContextMenuDataMediaType.NONE, "", "", "", "", "", "", null, false, 0, 0, 0);

        assertEquals("", ContextMenuUtils.getTitle(params));
    }
}
