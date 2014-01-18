// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_platform_part_aurawin.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/process/kill.h"
#include "base/win/windows_version.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metro_viewer/chrome_metro_viewer_process_host_aurawin.h"
#include "chrome/common/chrome_switches.h"

BrowserProcessPlatformPart::BrowserProcessPlatformPart() {
}

BrowserProcessPlatformPart::~BrowserProcessPlatformPart() {
}

void BrowserProcessPlatformPart::OnMetroViewerProcessTerminated() {
  metro_viewer_process_host_.reset(NULL);
}

void BrowserProcessPlatformPart::PlatformSpecificCommandLineProcessing(
    const CommandLine& command_line) {
  // Check for Windows 8 specific commandlines requesting that this process
  // either connect to an existing viewer or launch a new viewer and
  // synchronously wait for it to connect.
  if (base::win::GetVersion() >= base::win::VERSION_WIN8) {
    bool launch = command_line.HasSwitch(switches::kViewerLaunchViaAppId);
    bool connect = (launch ||
                    (command_line.HasSwitch(switches::kViewerConnect) &&
                     !metro_viewer_process_host_.get()));
    if (connect) {
      // Create a host to connect to the Metro viewer process over IPC.
      metro_viewer_process_host_.reset(new ChromeMetroViewerProcessHost());
      if (launch) {
        CHECK(metro_viewer_process_host_->LaunchViewerAndWaitForConnection(
            command_line.GetSwitchValueNative(
                switches::kViewerLaunchViaAppId)));
      }
    }
  }
}
