// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/native_process_launcher.h"

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "chrome/common/chrome_paths.h"

namespace extensions {

namespace {

base::FilePath FindManifestInDir(int dir_key, const std::string& host_name) {
  base::FilePath base_path;
  if (PathService::Get(dir_key, &base_path)) {
    base::FilePath path = base_path.Append(host_name + ".json");
    if (base::PathExists(path))
      return path;
  }
  return base::FilePath();
}

}  // namespace

// static
base::FilePath NativeProcessLauncher::FindManifest(
    const std::string& host_name,
    bool allow_user_level_hosts,
    std::string* error_message) {
  base::FilePath result;
  if (allow_user_level_hosts)
    result = FindManifestInDir(chrome::DIR_USER_NATIVE_MESSAGING, host_name);
  if (result.empty())
    result = FindManifestInDir(chrome::DIR_NATIVE_MESSAGING, host_name);

  if (result.empty())
    *error_message = "Can't find native messaging host " + host_name;

  return result;
}

// static
bool NativeProcessLauncher::LaunchNativeProcess(
    const CommandLine& command_line,
    base::ProcessHandle* process_handle,
    base::PlatformFile* read_file,
    base::PlatformFile* write_file) {
  base::FileHandleMappingVector fd_map;

  int read_pipe_fds[2] = {0};
  if (HANDLE_EINTR(pipe(read_pipe_fds)) != 0) {
    LOG(ERROR) << "Bad read pipe";
    return false;
  }
  base::ScopedFD read_pipe_read_fd(read_pipe_fds[0]);
  base::ScopedFD read_pipe_write_fd(read_pipe_fds[1]);
  fd_map.push_back(std::make_pair(read_pipe_write_fd.get(), STDOUT_FILENO));

  int write_pipe_fds[2] = {0};
  if (HANDLE_EINTR(pipe(write_pipe_fds)) != 0) {
    LOG(ERROR) << "Bad write pipe";
    return false;
  }
  base::ScopedFD write_pipe_read_fd(write_pipe_fds[0]);
  base::ScopedFD write_pipe_write_fd(write_pipe_fds[1]);
  fd_map.push_back(std::make_pair(write_pipe_read_fd.get(), STDIN_FILENO));

  base::LaunchOptions options;
  options.fds_to_remap = &fd_map;
  if (!base::LaunchProcess(command_line, options, process_handle)) {
    LOG(ERROR) << "Error launching process";
    return false;
  }

  // We will not be reading from the write pipe, nor writing from the read pipe.
  write_pipe_read_fd.reset();
  read_pipe_write_fd.reset();

  *read_file = read_pipe_read_fd.release();
  *write_file = write_pipe_write_fd.release();

  return true;
}

}  // namespace extensions
