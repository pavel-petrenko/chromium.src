// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_LAUNCHER_SUPPORT_CHROME_LAUNCHER_SUPPORT_H_
#define CHROME_INSTALLER_LAUNCHER_SUPPORT_CHROME_LAUNCHER_SUPPORT_H_

namespace base {
class FilePath;
}

namespace chrome_launcher_support {

enum InstallationLevel {
  USER_LEVEL_INSTALLATION,
  SYSTEM_LEVEL_INSTALLATION,
};

// Returns the path to an installed chrome.exe at the specified level, if it can
// be found via Omaha client state. Prefers the installer from a multi-install,
// but may also return that of a single-install of Chrome if no multi-install
// exists.
base::FilePath GetChromePathForInstallationLevel(InstallationLevel level);

// Returns the path to an installed chrome.exe, or an empty path. Prefers a
// system-level installation to a user-level installation. Uses Omaha client
// state to identify a Chrome installation location.
// The file path returned (if any) is guaranteed to exist.
base::FilePath GetAnyChromePath();

}  // namespace chrome_launcher_support

#endif  // CHROME_INSTALLER_LAUNCHER_SUPPORT_CHROME_LAUNCHER_SUPPORT_H_
