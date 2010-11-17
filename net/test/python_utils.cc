// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/python_utils.h"

#include "base/base_paths.h"
#include "base/environment.h"
#include "base/file_path.h"
#include "base/path_service.h"
#include "base/scoped_ptr.h"
#include "base/utf_string_conversions.h"

const char kPythonPathEnv[] = "PYTHONPATH";

void AppendToPythonPath(const FilePath& dir) {
  scoped_ptr<base::Environment> env(base::Environment::Create());
  std::string old_path;
  std::string dir_path;
#if defined(OS_WIN)
  dir_path = WideToUTF8(dir.value());
#elif defined(OS_POSIX)
  dir_path = dir.value();
#endif
  if (!env->GetVar(kPythonPathEnv, &old_path)) {
    env->SetVar(kPythonPathEnv, dir_path.c_str());
  } else if (old_path.find(dir_path) == std::string::npos) {
    std::string new_path(old_path);
#if defined(OS_WIN)
    new_path.append(";");
#elif defined(OS_POSIX)
    new_path.append(":");
#endif
    new_path.append(dir_path.c_str());
    env->SetVar(kPythonPathEnv, new_path);
  }
}

bool GetPyProtoPath(FilePath* dir) {
  // Locate the Python code generated by the protocol buffers compiler.
  FilePath generated_code_dir;
  if (!PathService::Get(base::DIR_EXE, &generated_code_dir)) {
    return false;
  }

  const FilePath kPyProto(FILE_PATH_LITERAL("pyproto"));

#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
  // On Mac and Chrome OS, DIR_EXE might be pointing deep into the Release/
  // (or Debug/) directory and we can't depend on how far down it goes. So we
  // walk upwards from DIR_EXE until we find a likely looking spot.
  while (!file_util::DirectoryExists(generated_code_dir.Append(kPyProto))) {
    FilePath parent = generated_code_dir.DirName();
    if (parent == generated_code_dir) {
      // We hit the root directory. Maybe we didn't build any targets which
      // produced Python protocol buffers.
      PathService::Get(base::DIR_EXE, &generated_code_dir);
      return false;
    }
    generated_code_dir = parent;
  }
#endif
  *dir = generated_code_dir.Append(kPyProto);
  return true;
}

bool GetPythonRunTime(FilePath* dir) {
#if defined(OS_WIN)
  if (!PathService::Get(base::DIR_SOURCE_ROOT, dir))
    return false;
  *dir = dir->Append(FILE_PATH_LITERAL("third_party"))
      .Append(FILE_PATH_LITERAL("python_24"))
      .Append(FILE_PATH_LITERAL("python.exe"));
#elif defined(OS_POSIX)
  *dir = FilePath("python");
#endif
  return true;
}

