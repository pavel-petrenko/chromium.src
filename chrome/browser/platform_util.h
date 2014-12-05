// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLATFORM_UTIL_H_
#define CHROME_BROWSER_PLATFORM_UTIL_H_

#include <string>

#include "base/strings/string16.h"
#include "ui/gfx/native_widget_types.h"

class GURL;
class Profile;

namespace base {
class FilePath;
}

namespace platform_util {

// Show the given file in a file manager. If possible, select the file.
// The |profile| is used to determine the running profile of file manager app
// in Chrome OS only. Not used in other platforms.
// Must be called from the UI thread.
void ShowItemInFolder(Profile* profile, const base::FilePath& full_path);

// Open the given file in the desktop's default manner.
// Must be called from the UI thread.
void OpenItem(Profile* profile, const base::FilePath& full_path);

// Open the given external protocol URL in the desktop's default manner.
// (For example, mailto: URLs in the default mail user agent.)
// Must be called from the UI thread.
void OpenExternal(Profile* profile, const GURL& url);
void OpenExternal2(const GURL& url);

// Get the top level window for the native view. This can return NULL.
gfx::NativeWindow GetTopLevel(gfx::NativeView view);

// Returns a NativeView handle for parenting dialogs off |window|. This can be
// used to position a dialog using a NativeWindow, when a NativeView (e.g.
// browser tab) isn't available.
gfx::NativeView GetViewForWindow(gfx::NativeWindow window);

// Get the direct parent of |view|, may return NULL.
gfx::NativeView GetParent(gfx::NativeView view);

// Returns true if |window| is the foreground top level window.
bool IsWindowActive(gfx::NativeWindow window);

// Activate the window, bringing it to the foreground top level.
void ActivateWindow(gfx::NativeWindow window);

// Returns true if the view is visible. The exact definition of this is
// platform-specific, but it is generally not "visible to the user", rather
// whether the view has the visible attribute set.
bool IsVisible(gfx::NativeView view);

#if defined(OS_MACOSX)
// On 10.7+, back and forward swipe gestures can be triggered using a scroll
// gesture, if enabled in System Preferences. This function returns true if
// the feature is supported and enabled, and false otherwise.
bool IsSwipeTrackingFromScrollEventsEnabled();
#endif

}  // namespace platform_util

#endif  // CHROME_BROWSER_PLATFORM_UTIL_H_
