// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_INTENTS_WEB_INTENT_PICKER_MODEL_OBSERVER_H_
#define CHROME_BROWSER_UI_INTENTS_WEB_INTENT_PICKER_MODEL_OBSERVER_H_
#pragma once

class WebIntentPickerModel;

// Observer for changes to the WebIntentPickerModel.
class WebIntentPickerModelObserver {
 public:
  virtual ~WebIntentPickerModelObserver() {}

  // Called when |model| has changed.
  virtual void OnModelChanged(WebIntentPickerModel* model) = 0;

  // Called when the favicon at |index| in |model| has changed.
  virtual void OnFaviconChanged(WebIntentPickerModel* model, size_t index) = 0;

  // Called when the inline disposition should be displayed for |model|.
  virtual void OnInlineDisposition(WebIntentPickerModel* model) = 0;
};

#endif  // CHROME_BROWSER_UI_INTENTS_WEB_INTENT_PICKER_MODEL_OBSERVER_H_
