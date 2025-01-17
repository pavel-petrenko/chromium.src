// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_COMMON_AW_SWITCHES_H_
#define ANDROID_WEBVIEW_COMMON_AW_SWITCHES_H_

namespace switches {

extern const char kWebViewLogJsConsoleMessages[];
extern const char kWebViewSandboxedRenderer[];
extern const char kWebViewDisableSafebrowsingSupport[];
extern const char kWebViewEnableVulkan[];
extern const char kWebViewSafebrowsingBlockAllResources[];
extern const char kHighlightAllWebViews[];
extern const char kWebViewVerboseLogging[];
extern const char kFinchSeedExpirationAge[];
extern const char kFinchSeedIgnorePendingDownload[];
extern const char kFinchSeedMinDownloadPeriod[];
extern const char kFinchSeedMinUpdatePeriod[];

}  // namespace switches

#endif  // ANDROID_WEBVIEW_COMMON_AW_SWITCHES_H_
