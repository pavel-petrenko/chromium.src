// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/websockets/websocket_extension.h"

#include <map>
#include <string>

#include "base/logging.h"

namespace net {

WebSocketExtension::Parameter::Parameter(const std::string& name)
    : name_(name) {}

WebSocketExtension::Parameter::Parameter(const std::string& name,
                                         const std::string& value)
    : name_(name), value_(value) {
  DCHECK(!value.empty());
}

bool WebSocketExtension::Parameter::Equals(const Parameter& other) const {
  return name_ == other.name_ && value_ == other.value_;
}

WebSocketExtension::WebSocketExtension() {}

WebSocketExtension::WebSocketExtension(const std::string& name)
    : name_(name) {}

WebSocketExtension::~WebSocketExtension() {}

bool WebSocketExtension::Equals(const WebSocketExtension& other) const {
  if (name_ != other.name_) return false;
  if (parameters_.size() != other.parameters_.size()) return false;

  std::multimap<std::string, std::string> this_parameters, other_parameters;
  for (const auto& p : parameters_) {
    this_parameters.insert(std::make_pair(p.name(), p.value()));
  }
  for (const auto& p : other.parameters_) {
    other_parameters.insert(std::make_pair(p.name(), p.value()));
  }
  return this_parameters == other_parameters;
}

}  // namespace net
