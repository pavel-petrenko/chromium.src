// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.suggestions.tile;

import android.graphics.drawable.Drawable;
import android.view.View;

import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel.WritableBooleanPropertyKey;
import org.chromium.ui.modelutil.PropertyModel.WritableIntPropertyKey;
import org.chromium.ui.modelutil.PropertyModel.WritableObjectPropertyKey;

/** TileView properties. */
public final class TileViewProperties {
    /** The title of the tile. */
    public static final WritableObjectPropertyKey<String> TITLE = new WritableObjectPropertyKey<>();

    /** Maximum number of lines used to present the title. */
    public static final WritableIntPropertyKey TITLE_LINES = new WritableIntPropertyKey();

    /** The primary icon used by the tile. */
    public static final WritableObjectPropertyKey<Drawable> ICON =
            new WritableObjectPropertyKey<>();

    /** Whether Tile should present a large icon. */
    public static final WritableBooleanPropertyKey SHOW_LARGE_ICON =
            new WritableBooleanPropertyKey();

    /** Badge visibility. */
    public static final WritableBooleanPropertyKey BADGE_VISIBLE = new WritableBooleanPropertyKey();

    /** Handler receiving focus events. */
    public static final WritableObjectPropertyKey<Runnable> ON_FOCUS_VIA_SELECTION =
            new WritableObjectPropertyKey<>();

    /** Handler receiving click events. */
    public static final WritableObjectPropertyKey<View.OnClickListener> ON_CLICK =
            new WritableObjectPropertyKey<>();

    /** Handler receiving long-click events. */
    public static final WritableObjectPropertyKey<View.OnLongClickListener> ON_LONG_CLICK =
            new WritableObjectPropertyKey<>();

    /** Handler receiving context menu call events. */
    public static final WritableObjectPropertyKey<View.OnCreateContextMenuListener>
            ON_CREATE_CONTEXT_MENU = new WritableObjectPropertyKey<>();

    public static final PropertyKey[] ALL_KEYS =
            new PropertyKey[] {ICON, TITLE, TITLE_LINES, BADGE_VISIBLE, SHOW_LARGE_ICON,
                    ON_FOCUS_VIA_SELECTION, ON_CLICK, ON_LONG_CLICK, ON_CREATE_CONTEXT_MENU};
}
