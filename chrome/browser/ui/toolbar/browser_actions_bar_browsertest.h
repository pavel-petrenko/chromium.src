// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_BROWSER_ACTIONS_BAR_BROWSERTEST_H_
#define CHROME_BROWSER_UI_TOOLBAR_BROWSER_ACTIONS_BAR_BROWSERTEST_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "extensions/common/feature_switch.h"

namespace extensions {
class Extension;
}

class BrowserActionTestUtil;
class ToolbarActionsModel;

// A platform-independent browser test class for the browser actions bar.
class BrowserActionsBarBrowserTest : public ExtensionBrowserTest {
 protected:
  BrowserActionsBarBrowserTest();
  ~BrowserActionsBarBrowserTest() override;

  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;

  BrowserActionTestUtil* browser_actions_bar() {
    return browser_actions_bar_.get();
  }
  ToolbarActionsModel* toolbar_model() { return toolbar_model_; }

  // Creates three different extensions, each with a browser action, and adds
  // them to associated ExtensionService. These can then be accessed via
  // extension_[a|b|c]().
  void LoadExtensions();

  const extensions::Extension* extension_a() const {
    return extension_a_.get();
  }
  const extensions::Extension* extension_b() const {
    return extension_b_.get();
  }
  const extensions::Extension* extension_c() const {
    return extension_c_.get();
  }

 private:
  scoped_ptr<BrowserActionTestUtil> browser_actions_bar_;

  // The associated toolbar model, weak.
  ToolbarActionsModel* toolbar_model_;

  // Extensions with browser actions used for testing.
  scoped_refptr<const extensions::Extension> extension_a_;
  scoped_refptr<const extensions::Extension> extension_b_;
  scoped_refptr<const extensions::Extension> extension_c_;

  DISALLOW_COPY_AND_ASSIGN(BrowserActionsBarBrowserTest);
};

// A test with the extension-action-redesign switch enabled.
class BrowserActionsBarRedesignBrowserTest
    : public BrowserActionsBarBrowserTest {
 protected:
  BrowserActionsBarRedesignBrowserTest();
  ~BrowserActionsBarRedesignBrowserTest() override;

  void SetUpCommandLine(base::CommandLine* command_line) override;

 private:
  // Enable the feature redesign switch.
  scoped_ptr<extensions::FeatureSwitch::ScopedOverride> enable_redesign_;

  DISALLOW_COPY_AND_ASSIGN(BrowserActionsBarRedesignBrowserTest);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_BROWSER_ACTIONS_BAR_BROWSERTEST_H_
