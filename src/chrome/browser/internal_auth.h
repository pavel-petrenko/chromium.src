// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INTERNAL_AUTH_H_
#define CHROME_BROWSER_INTERNAL_AUTH_H_
#pragma once

#include <map>
#include <string>

#include "base/gtest_prod_util.h"

class WebSocketProxyPrivateGetPassportForTCPFunction;
class WebSocketProxyPrivateGetURLForTCPFunction;

namespace browser {

// Call InternalAuthVerification methods on any thread.
class InternalAuthVerification {
 public:
  // Used by consumer of passport in order to verify credentials.
  static bool VerifyPassport(
      const std::string& passport,
      const std::string& domain,
      const std::map<std::string, std::string>& var_value_map);

 private:
  // We allow for easy separation of InternalAuthVerification and
  // InternalAuthGeneration so the only thing they share (besides time) is
  // a key (regenerated infrequently).
  static void ChangeKey(const std::string& key);

#ifdef UNIT_TEST
  static void set_verification_window_seconds(int seconds) {
    verification_window_seconds_ = seconds;
  }
#endif

  static int get_verification_window_ticks();

  static int verification_window_seconds_;

  friend class InternalAuthGeneration;
  friend class InternalAuthVerificationService;
  friend class InternalAuthGenerationService;

  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, ExpirationAndBruteForce);
};

// Not thread-safe. Make all calls on the same thread (UI thread).
class InternalAuthGeneration {
 private:
  // Generates passport; do this only after successful check of credentials.
  static std::string GeneratePassport(
      const std::string& domain,
      const std::map<std::string, std::string>& var_value_map);

  // Used only by tests.
  static void GenerateNewKey();

  friend class ::WebSocketProxyPrivateGetPassportForTCPFunction;
  friend class ::WebSocketProxyPrivateGetURLForTCPFunction;

  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, BasicGeneration);
  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, DoubleGeneration);
  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, BadGeneration);
  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, BasicVerification);
  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, BruteForce);
  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, ExpirationAndBruteForce);
  FRIEND_TEST_ALL_PREFIXES(InternalAuthTest, ChangeKey);
};

}  // namespace browser

#endif  // CHROME_BROWSER_INTERNAL_AUTH_H_
