// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_STUB_WEB_VIEW_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_STUB_WEB_VIEW_H_

#include <list>
#include <string>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/test/chromedriver/chrome/web_view.h"

class StubWebView : public WebView {
 public:
  explicit StubWebView(const std::string& id);
  ~StubWebView() override;

  // Overridden from WebView:
  std::string GetId() override;
  bool WasCrashed() override;
  Status ConnectIfNecessary() override;
  Status HandleReceivedEvents() override;
  Status Load(const std::string& url) override;
  Status Reload() override;
  Status TraverseHistory(int delta) override;
  Status EvaluateScript(const std::string& frame,
                        const std::string& function,
                        scoped_ptr<base::Value>* result) override;
  Status CallFunction(const std::string& frame,
                      const std::string& function,
                      const base::ListValue& args,
                      scoped_ptr<base::Value>* result) override;
  Status CallAsyncFunction(const std::string& frame,
                           const std::string& function,
                           const base::ListValue& args,
                           const base::TimeDelta& timeout,
                           scoped_ptr<base::Value>* result) override;
  Status CallUserAsyncFunction(const std::string& frame,
                               const std::string& function,
                               const base::ListValue& args,
                               const base::TimeDelta& timeout,
                               scoped_ptr<base::Value>* result) override;
  Status GetFrameByFunction(const std::string& frame,
                            const std::string& function,
                            const base::ListValue& args,
                            std::string* out_frame) override;
  Status DispatchMouseEvents(const std::list<MouseEvent>& events,
                             const std::string& frame) override;
  Status DispatchTouchEvent(const TouchEvent& event) override;
  Status DispatchTouchEvents(const std::list<TouchEvent>& events) override;
  Status DispatchKeyEvents(const std::list<KeyEvent>& events) override;
  Status GetCookies(scoped_ptr<base::ListValue>* cookies) override;
  Status DeleteCookie(const std::string& name, const std::string& url) override;
  Status WaitForPendingNavigations(const std::string& frame_id,
                                   const base::TimeDelta& timeout,
                                   bool stop_load_on_timeout) override;
  Status IsPendingNavigation(const std::string& frame_id,
                             bool* is_pending) override;
  JavaScriptDialogManager* GetJavaScriptDialogManager() override;
  Status OverrideGeolocation(const Geoposition& geoposition) override;
  Status OverrideNetworkConditions(
      const NetworkConditions& network_conditions) override;
  Status CaptureScreenshot(std::string* screenshot) override;
  Status SetFileInputFiles(const std::string& frame,
                           const base::DictionaryValue& element,
                           const std::vector<base::FilePath>& files) override;
  Status TakeHeapSnapshot(scoped_ptr<base::Value>* snapshot) override;
  Status StartProfile() override;
  Status EndProfile(scoped_ptr<base::Value>* profile_data) override;

 private:
  std::string id_;
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_STUB_WEB_VIEW_H_
