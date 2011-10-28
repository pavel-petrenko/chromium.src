// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CUSTOM_HANDLERS_PROTOCOL_HANDLER_H_
#define CHROME_COMMON_CUSTOM_HANDLERS_PROTOCOL_HANDLER_H_
#pragma once

#include <string>

#include "base/values.h"
#include "googleurl/src/gurl.h"

// A single tuple of (protocol, url, title) that indicates how URLs of the
// given protocol should be rewritten to be handled.

class ProtocolHandler {
 public:
  static ProtocolHandler CreateProtocolHandler(const std::string& protocol,
                                               const GURL& url,
                                               const string16& title);

  // Creates a ProtocolHandler with fields from the dictionary. Returns an
  // empty ProtocolHandler if the input is invalid.
  static ProtocolHandler CreateProtocolHandler(const DictionaryValue* value);

  // Returns true if the dictionary value has all the necessary fields to
  // define a ProtocolHandler.
  static bool IsValidDict(const DictionaryValue* value);

  // Returns true if this handler's url has the same origin as the given one.
  bool IsSameOrigin(const ProtocolHandler& handler) const;

  // Canonical empty ProtocolHandler.
  static const ProtocolHandler& EmptyProtocolHandler();

  // Interpolates the given URL into the URL template of this handler.
  GURL TranslateUrl(const GURL& url) const;

  // Returns true if the handlers are considered equivalent when determining
  // if both handlers can be registered, or if a handler has previously been
  // ignored.
  bool IsEquivalent(const ProtocolHandler& other) const;

  // Encodes this protocol handler as a DictionaryValue. The caller is
  // responsible for deleting the returned value.
  DictionaryValue* Encode() const;

  const std::string& protocol() const { return protocol_; }
  const GURL& url() const { return url_;}
  const string16& title() const { return title_; }

  bool IsEmpty() const {
    return protocol_.empty();
  }

  bool operator==(const ProtocolHandler& other) const;
  bool operator<(const ProtocolHandler& other) const;

 private:
  ProtocolHandler(const std::string& protocol,
                  const GURL& url,
                  const string16& title);
  ProtocolHandler();

  std::string protocol_;
  GURL url_;
  string16 title_;
};

#endif  // CHROME_COMMON_CUSTOM_HANDLERS_PROTOCOL_HANDLER_H_
