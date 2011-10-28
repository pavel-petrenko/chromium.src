// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// DataPack represents a read-only view onto an on-disk file that contains
// (key, value) pairs of data.  It's used to store static resources like
// translation strings and images.

#ifndef UI_BASE_RESOURCE_DATA_PACK_H_
#define UI_BASE_RESOURCE_DATA_PACK_H_
#pragma once

#include <map>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ui/base/ui_export.h"

class FilePath;
class RefCountedStaticMemory;

namespace base {
class StringPiece;
}

namespace file_util {
class MemoryMappedFile;
}

namespace ui {

class UI_EXPORT DataPack {
 public:
  // What type of encoding the text resources use.
  enum TextEncodingType {
    BINARY,
    UTF8,
    UTF16
  };

  DataPack();
  ~DataPack();

  // Load a pack file from |path|, returning false on error.
  bool Load(const FilePath& path);

  // Get resource by id |resource_id|, filling in |data|.
  // The data is owned by the DataPack object and should not be modified.
  // Returns false if the resource id isn't found.
  bool GetStringPiece(uint16 resource_id, base::StringPiece* data) const;

  // Like GetStringPiece(), but returns a reference to memory. This interface
  // is used for image data, while the StringPiece interface is usually used
  // for localization strings.
  RefCountedStaticMemory* GetStaticMemory(uint16 resource_id) const;

  // Writes a pack file containing |resources| to |path|. If there are any
  // text resources to be written, their encoding must already agree to the
  // |textEncodingType| specified. If no text resources are present, please
  // indicate BINARY.
  static bool WritePack(const FilePath& path,
                        const std::map<uint16, base::StringPiece>& resources,
                        TextEncodingType textEncodingType);

  // Get the encoding type of text resources.
  TextEncodingType GetTextEncodingType() const { return text_encoding_type_; }

 private:
  // The memory-mapped data.
  scoped_ptr<file_util::MemoryMappedFile> mmap_;

  // Number of resources in the data.
  size_t resource_count_;

  // Type of encoding for text resources.
  TextEncodingType text_encoding_type_;

  DISALLOW_COPY_AND_ASSIGN(DataPack);
};

}  // namespace ui

#endif  // UI_BASE_RESOURCE_DATA_PACK_H_
