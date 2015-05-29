// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "components/filesystem/files_test_base.h"

namespace filesystem {
namespace {

using DirectoryImplTest = FilesTestBase;

TEST_F(DirectoryImplTest, Read) {
  DirectoryPtr directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Make some files.
  const struct {
    const char* name;
    uint32_t open_flags;
  } files_to_create[] = {
      {"my_file1", kFlagRead | kFlagWrite | kFlagCreate},
      {"my_file2", kFlagWrite | kFlagCreate | kFlagOpenAlways},
      {"my_file3", kFlagWrite | kFlagCreate | kFlagAppend},
      {"my_file4", kFlagWrite | kFlagCreate}};
  for (size_t i = 0; i < arraysize(files_to_create); i++) {
    error = ERROR_FAILED;
    directory->OpenFile(files_to_create[i].name, nullptr,
                        files_to_create[i].open_flags, Capture(&error));
    ASSERT_TRUE(directory.WaitForIncomingResponse());
    EXPECT_EQ(ERROR_OK, error);
  }
  // Make a directory.
  error = ERROR_FAILED;
  directory->OpenDirectory(
      "my_dir", nullptr, kFlagRead | kFlagWrite | kFlagCreate, Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_OK, error);

  error = ERROR_FAILED;
  mojo::Array<DirectoryEntryPtr> directory_contents;
  directory->Read(Capture(&error, &directory_contents));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_OK, error);

  // Expected contents of the directory.
  std::map<std::string, FileType> expected_contents;
  expected_contents["my_file1"] = FILE_TYPE_REGULAR_FILE;
  expected_contents["my_file2"] = FILE_TYPE_REGULAR_FILE;
  expected_contents["my_file3"] = FILE_TYPE_REGULAR_FILE;
  expected_contents["my_file4"] = FILE_TYPE_REGULAR_FILE;
  expected_contents["my_dir"] = FILE_TYPE_DIRECTORY;
  // Note: We don't expose ".." or ".".

  EXPECT_EQ(expected_contents.size(), directory_contents.size());
  for (size_t i = 0; i < directory_contents.size(); i++) {
    ASSERT_TRUE(directory_contents[i]);
    ASSERT_TRUE(directory_contents[i]->name);
    auto it = expected_contents.find(directory_contents[i]->name.get());
    ASSERT_TRUE(it != expected_contents.end());
    EXPECT_EQ(it->second, directory_contents[i]->type);
    expected_contents.erase(it);
  }
}

// TODO(vtl): Properly test OpenFile() and OpenDirectory() (including flags).

TEST_F(DirectoryImplTest, BasicRenameDelete) {
  DirectoryPtr directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Create my_file.
  error = ERROR_FAILED;
  directory->OpenFile("my_file", nullptr, kFlagWrite | kFlagCreate,
                      Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_OK, error);

  // Opening my_file should succeed.
  error = ERROR_FAILED;
  directory->OpenFile("my_file", nullptr, kFlagRead | kFlagOpen,
                      Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_OK, error);

  // Rename my_file to my_new_file.
  directory->Rename("my_file", "my_new_file", Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_OK, error);

  // Opening my_file should fail.

  error = ERROR_FAILED;
  directory->OpenFile("my_file", nullptr, kFlagRead | kFlagOpen,
                      Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_FAILED, error);

  // Opening my_new_file should succeed.
  error = ERROR_FAILED;
  directory->OpenFile("my_new_file", nullptr, kFlagRead | kFlagOpen,
                      Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_OK, error);

  // Delete my_new_file (no flags).
  directory->Delete("my_new_file", 0, Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_OK, error);

  // Opening my_new_file should fail.
  error = ERROR_FAILED;
  directory->OpenFile("my_new_file", nullptr, kFlagRead | kFlagOpen,
                      Capture(&error));
  ASSERT_TRUE(directory.WaitForIncomingResponse());
  EXPECT_EQ(ERROR_FAILED, error);
}

// TODO(vtl): Test delete flags.

}  // namespace
}  // namespace filesystem
