// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTION_VIEW_DELEGATE_H_
#define CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTION_VIEW_DELEGATE_H_

class ToolbarActionViewController;

namespace content {
class WebContents;
}

// The view that surrounds a ToolbarAction and typically owns the
// ToolbarActionViewController.
class ToolbarActionViewDelegate {
 public:
  // In some cases (such as when an action is shown in a menu), a substitute
  // ToolbarActionViewController should be used for showing popups. This
  // returns the preferred controller.
  virtual ToolbarActionViewController* GetPreferredPopupViewController() = 0;

  // Returns the current web contents.
  virtual content::WebContents* GetCurrentWebContents() const = 0;

  // Updates the view to reflect current state.
  virtual void UpdateState() = 0;

 protected:
  virtual ~ToolbarActionViewDelegate() {}
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTION_VIEW_DELEGATE_H_
