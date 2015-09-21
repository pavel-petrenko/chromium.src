// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_STORE_IMPL_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_STORE_IMPL_H_

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/sequence_checker.h"
#include "components/data_reduction_proxy/core/browser/data_store.h"

namespace leveldb {
class DB;
}

namespace data_reduction_proxy {

// Implementation of |DataStore| using LevelDB.
class DataStoreImpl : public DataStore {
 public:
  explicit DataStoreImpl(const base::FilePath& profile_path);
  ~DataStoreImpl() override;

  // Overrides of DataStore.
  void InitializeOnDBThread() override;

  Status Get(const std::string& key, std::string* value) override;

  Status Put(const std::map<std::string, std::string>& map) override;

 private:
  // Opens the underlying LevelDB for read and write.
  Status OpenDB();

  // Deletes the LevelDB and recreates it. This method is called if any DB call
  // returns a |CORRUPTED| status.
  void RecreateDB();

  // The underlying LevelDB used by this implementation.
  scoped_ptr<leveldb::DB> db_;

  // Path to the profile using this store.
  const base::FilePath profile_path_;

  base::SequenceChecker sequence_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataStoreImpl);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_STORE_IMPL_H_
