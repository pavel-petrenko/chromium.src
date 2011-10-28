// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MAC_SCOPED_IOOBJECT_H_
#define CHROME_BROWSER_MAC_SCOPED_IOOBJECT_H_
#pragma once

#include <IOKit/IOKitLib.h>

#include "base/basictypes.h"
#include "base/compiler_specific.h"

// Just like ScopedCFTypeRef but for io_object_t and subclasses.
template<typename IOT>
class ScopedIOObject {
 public:
  typedef IOT element_type;

  explicit ScopedIOObject(IOT object = NULL)
      : object_(object) {
  }

  ~ScopedIOObject() {
    if (object_)
      IOObjectRelease(object_);
  }

  void reset(IOT object = NULL) {
    if (object_)
      IOObjectRelease(object_);
    object_ = object;
  }

  bool operator==(IOT that) const {
    return object_ == that;
  }

  bool operator!=(IOT that) const {
    return object_ != that;
  }

  operator IOT() const {
    return object_;
  }

  IOT get() const {
    return object_;
  }

  void swap(ScopedIOObject& that) {
    IOT temp = that.object_;
    that.object_ = object_;
    object_ = temp;
  }

  IOT release() WARN_UNUSED_RESULT {
    IOT temp = object_;
    object_ = NULL;
    return temp;
  }

 private:
  IOT object_;

  DISALLOW_COPY_AND_ASSIGN(ScopedIOObject);
};

#endif  // CHROME_BROWSER_MAC_SCOPED_IOOBJECT_H_
