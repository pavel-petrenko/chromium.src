// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/screen.h"

#import <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>

namespace gfx {

// static
gfx::Point Screen::GetCursorScreenPoint() {
  NSPoint mouseLocation  = [NSEvent mouseLocation];
  // Flip coordinates to gfx (0,0 in top-left corner) using primary screen.
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
  mouseLocation.y = NSMaxY([screen frame]) - mouseLocation.y;
  return gfx::Point(mouseLocation.x, mouseLocation.y);
}

// static
gfx::Size Screen::GetPrimaryMonitorSize() {
  CGDirectDisplayID main_display = CGMainDisplayID();
  return gfx::Size(CGDisplayPixelsWide(main_display),
                   CGDisplayPixelsHigh(main_display));
}

// static
int Screen::GetNumMonitors() {
  // Don't just return the number of online displays.  It includes displays
  // that mirror other displays, which are not desired in the count.  It's
  // tempting to use the count returned by CGGetActiveDisplayList, but active
  // displays exclude sleeping displays, and those are desired in the count.

  // It would be ridiculous to have this many displays connected, but
  // CGDirectDisplayID is just an integer, so supporting up to this many
  // doesn't hurt.
  CGDirectDisplayID online_displays[128];
  CGDisplayCount online_display_count = 0;
  if (CGGetOnlineDisplayList(arraysize(online_displays),
                             online_displays,
                             &online_display_count) != kCGErrorSuccess) {
    // 1 is a reasonable assumption.
    return 1;
  }

  int display_count = 0;
  for (CGDisplayCount online_display_index = 0;
       online_display_index < online_display_count;
       ++online_display_index) {
    CGDirectDisplayID online_display = online_displays[online_display_index];
    if (CGDisplayMirrorsDisplay(online_display) == kCGNullDirectDisplay) {
      // If this display doesn't mirror any other, include it in the count.
      // The primary display in a mirrored set will be counted, but those that
      // mirror it will not be.
      ++display_count;
    }
  }

  return display_count;
}

}  // namespace gfx
