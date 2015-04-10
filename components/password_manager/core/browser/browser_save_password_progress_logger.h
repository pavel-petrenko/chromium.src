// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_BROWSER_SAVE_PASSWORD_PROGRESS_LOGGER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_BROWSER_SAVE_PASSWORD_PROGRESS_LOGGER_H_

#include <string>

#include "components/autofill/core/common/save_password_progress_logger.h"

namespace password_manager {

class PasswordManagerClient;

// This is the SavePasswordProgressLogger specialization for the browser code,
// where the PasswordManagerClient can be directly called.
class BrowserSavePasswordProgressLogger
    : public autofill::SavePasswordProgressLogger {
 public:
  explicit BrowserSavePasswordProgressLogger(
      const PasswordManagerClient* client);
  ~BrowserSavePasswordProgressLogger() override;

  // Browser-specific addition to the base class' Log* methods. The input is
  // sanitized and passed to SendLog for display.
  void LogFormSignatures(StringID label, const autofill::PasswordForm& form);

 protected:
  // autofill::SavePasswordProgressLogger:
  void SendLog(const std::string& log) override;

 private:
  // The PasswordManagerClient to which logs can be sent for display. The client
  // must outlive this logger.
  const PasswordManagerClient* const client_;

  DISALLOW_COPY_AND_ASSIGN(BrowserSavePasswordProgressLogger);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_BROWSER_SAVE_PASSWORD_PROGRESS_LOGGER_H_
