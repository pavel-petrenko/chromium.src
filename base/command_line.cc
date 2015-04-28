// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"

#include <algorithm>
#include <ostream>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#include <shellapi.h>
#endif

namespace base {

CommandLine* CommandLine::current_process_commandline_ = NULL;

namespace {

const CommandLine::CharType kSwitchTerminator[] = FILE_PATH_LITERAL("--");
const CommandLine::CharType kSwitchValueSeparator[] = FILE_PATH_LITERAL("=");

// Since we use a lazy match, make sure that longer versions (like "--") are
// listed before shorter versions (like "-") of similar prefixes.
#if defined(OS_WIN)
// By putting slash last, we can control whether it is treaded as a switch
// value by changing the value of switch_prefix_count to be one less than
// the array size.
const CommandLine::CharType* const kSwitchPrefixes[] = {L"--", L"-", L"/"};
#elif defined(OS_POSIX)
// Unixes don't use slash as a switch.
const CommandLine::CharType* const kSwitchPrefixes[] = {"--", "-"};
#endif
size_t switch_prefix_count = arraysize(kSwitchPrefixes);

size_t GetSwitchPrefixLength(const CommandLine::StringType& string) {
  for (size_t i = 0; i < switch_prefix_count; ++i) {
    CommandLine::StringType prefix(kSwitchPrefixes[i]);
    if (string.compare(0, prefix.length(), prefix) == 0)
      return prefix.length();
  }
  return 0;
}

} // namespace

// Fills in |switch_string| and |switch_value| if |string| is a switch.
// This will preserve the input switch prefix in the output |switch_string|.
bool IsSwitch(const CommandLine::StringType& string,
              CommandLine::StringType* switch_string,
              CommandLine::StringType* switch_value) {
  switch_string->clear();
  switch_value->clear();
  size_t prefix_length = GetSwitchPrefixLength(string);
  if (prefix_length == 0 || prefix_length == string.length())
    return false;

  const size_t equals_position = string.find(kSwitchValueSeparator);
  *switch_string = string.substr(0, equals_position);
  if (equals_position != CommandLine::StringType::npos)
    *switch_value = string.substr(equals_position + 1);
  return true;
}

namespace {

// Append switches and arguments, keeping switches before arguments.
void AppendSwitchesAndArguments(CommandLine& command_line,
                                const CommandLine::StringVector& argv) {
  bool parse_switches = true;
  for (size_t i = 1; i < argv.size(); ++i) {
    CommandLine::StringType arg = argv[i];
    TrimWhitespace(arg, TRIM_ALL, &arg);

    CommandLine::StringType switch_string;
    CommandLine::StringType switch_value;
    parse_switches &= (arg != kSwitchTerminator);
    if (parse_switches && IsSwitch(arg, &switch_string, &switch_value)) {
#if defined(OS_WIN)
      command_line.AppendSwitchNative(UTF16ToASCII(switch_string),
                                      switch_value);
#elif defined(OS_POSIX)
      command_line.AppendSwitchNative(switch_string, switch_value);
#endif
    } else {
      command_line.AppendArgNative(arg);
    }
  }
}

// Lowercase switches for backwards compatiblity *on Windows*.
const std::string& LowerASCIIOnWindows(const std::string& string) {
// #if defined(OS_WIN)
//   return base::StringToLowerASCII(string);
// #elif defined(OS_POSIX)
  return string;
// #endif
}


#if defined(OS_WIN)
// Quote a string as necessary for CommandLineToArgvW compatiblity *on Windows*.
std::wstring QuoteForCommandLineToArgvW(const std::wstring& arg) {
  // We follow the quoting rules of CommandLineToArgvW.
  // http://msdn.microsoft.com/en-us/library/17w5ykft.aspx
  if (arg.find_first_of(L" \\\"") == std::wstring::npos) {
    // No quoting necessary.
    return arg;
  }

  std::wstring out;
  out.push_back(L'"');
  for (size_t i = 0; i < arg.size(); ++i) {
    if (arg[i] == '\\') {
      // Find the extent of this run of backslashes.
      size_t start = i, end = start + 1;
      for (; end < arg.size() && arg[end] == '\\'; ++end)
        /* empty */;
      size_t backslash_count = end - start;

      // Backslashes are escapes only if the run is followed by a double quote.
      // Since we also will end the string with a double quote, we escape for
      // either a double quote or the end of the string.
      if (end == arg.size() || arg[end] == '"') {
        // To quote, we need to output 2x as many backslashes.
        backslash_count *= 2;
      }
      for (size_t j = 0; j < backslash_count; ++j)
        out.push_back('\\');

      // Advance i to one before the end to balance i++ in loop.
      i = end - 1;
    } else if (arg[i] == '"') {
      out.push_back('\\');
      out.push_back('"');
    } else {
      out.push_back(arg[i]);
    }
  }
  out.push_back('"');

  return out;
}
#endif

}  // namespace

CommandLine::CommandLine(NoProgram no_program)
    : argv_(1),
      begin_args_(1),
      argc0_(0), argv0_(NULL) {
}

#if defined(OS_WIN)
CommandLine::CommandLine(const CommandLine& other)
    : argv_(1),
    begin_args_(1),
    argc0_(0), argv0_(NULL) {
  argv_ = other.argv_;
  original_argv_ = other.original_argv_;
  switches_ = other.switches_;
  begin_args_ = other.begin_args_;
  argc0_ = other.argc0_;
  if (other.argv0_) {
    argv0_ = new char*[argc0_ + 1];
    for (int i = 0; i < argc0_; ++i) {
      argv0_[i] = new char[strlen(other.argv0_[i]) + 1];
      strcpy(argv0_[i], other.argv0_[i]);
    }
    argv0_[argc0_] = NULL;
  }
}
#endif

CommandLine::CommandLine(const FilePath& program)
    : argv_(1),
      begin_args_(1),
      argc0_(0), argv0_(NULL) {
  SetProgram(program);
}

CommandLine::CommandLine(int argc, const CommandLine::CharType* const* argv)
    : argv_(1),
      begin_args_(1),
      argc0_(0), argv0_(NULL) {
  InitFromArgv(argc, argv);
}

CommandLine::CommandLine(const StringVector& argv)
    : argv_(1),
      begin_args_(1),
      argc0_(0), argv0_(NULL) {
  InitFromArgv(argv);
}

CommandLine::~CommandLine() {
#if defined(OS_WIN)
  if (!argv0_)
    return;
  for (int i = 0; i < argc0_; i++) {
    delete[] argv0_[i];
  }
  delete[] argv0_;
#endif
}

#if defined(OS_WIN)
// static
void CommandLine::set_slash_is_not_a_switch() {
  // The last switch prefix should be slash, so adjust the size to skip it.
  DCHECK(wcscmp(kSwitchPrefixes[arraysize(kSwitchPrefixes) - 1], L"/") == 0);
  switch_prefix_count = arraysize(kSwitchPrefixes) - 1;
}
#endif

// static
bool CommandLine::Init(int argc, const char* const* argv) {
  if (current_process_commandline_) {
    // If this is intentional, Reset() must be called first. If we are using
    // the shared build mode, we have to share a single object across multiple
    // shared libraries.
    return false;
  }

  current_process_commandline_ = new CommandLine(NO_PROGRAM);
#if defined(OS_WIN)
  current_process_commandline_->ParseFromString(::GetCommandLineW());
#elif defined(OS_POSIX)
  current_process_commandline_->InitFromArgv(argc, argv);
#endif

  return true;
}

// static
void CommandLine::Reset() {
  DCHECK(current_process_commandline_);
  delete current_process_commandline_;
  current_process_commandline_ = NULL;
}

// static
CommandLine* CommandLine::ForCurrentProcess() {
  DCHECK(current_process_commandline_);
  return current_process_commandline_;
}

// static
bool CommandLine::InitializedForCurrentProcess() {
  return !!current_process_commandline_;
}

#if defined(OS_WIN)
// static
CommandLine CommandLine::FromString(const std::wstring& command_line) {
  CommandLine cmd(NO_PROGRAM);
  cmd.ParseFromString(command_line);
  return cmd;
}
#endif

void CommandLine::InitFromArgv(int argc,
                               const CommandLine::CharType* const* argv) {
  StringVector new_argv;
  argc0_ = argc;
#if !defined(OS_WIN)
  argv0_ = (char**)argv;
#else
  argv0_ = new char*[argc + 1];
  for (int i = 0; i < argc; ++i) {
    std::string str(base::WideToUTF8(argv[i]));
    argv0_[i] = new char[str.length() + 1];
    strcpy(argv0_[i], str.c_str());
  }
  argv0_[argc] = NULL;
#endif
  for (int i = 0; i < argc; ++i)
    new_argv.push_back(argv[i]);
  InitFromArgv(new_argv);
}

void CommandLine::InitFromArgv(const StringVector& argv) {
#if !defined(OS_MACOSX)
  original_argv_ = argv;
#else
  for (size_t index = 0; index < argv.size(); ++index) {
    if (argv[index].compare(0, strlen("--psn_"), "--psn_") != 0 &&
        argv[index].compare(0, strlen("-psn_"), "-psn_") != 0) {
      original_argv_.push_back(argv[index]);
    }
  }
#endif
  argv_ = StringVector(1);
  switches_.clear();
  begin_args_ = 1;
  SetProgram(argv.empty() ? FilePath() : FilePath(argv[0]));
  AppendSwitchesAndArguments(*this, argv);
}

CommandLine::StringType CommandLine::GetCommandLineString() const {
  StringType string(argv_[0]);
#if defined(OS_WIN)
  string = QuoteForCommandLineToArgvW(string);
#endif
  StringType params(GetArgumentsString());
  if (!params.empty()) {
    string.append(StringType(FILE_PATH_LITERAL(" ")));
    string.append(params);
  }
  return string;
}

CommandLine::StringType CommandLine::GetArgumentsString() const {
  StringType params;
  // Append switches and arguments.
  bool parse_switches = true;
  for (size_t i = 1; i < argv_.size(); ++i) {
    StringType arg = argv_[i];
    StringType switch_string;
    StringType switch_value;
    parse_switches &= arg != kSwitchTerminator;
    if (i > 1)
      params.append(StringType(FILE_PATH_LITERAL(" ")));
    if (parse_switches && IsSwitch(arg, &switch_string, &switch_value)) {
      params.append(switch_string);
      if (!switch_value.empty()) {
#if defined(OS_WIN)
        switch_value = QuoteForCommandLineToArgvW(switch_value);
#endif
        params.append(kSwitchValueSeparator + switch_value);
      }
    }
    else {
#if defined(OS_WIN)
      arg = QuoteForCommandLineToArgvW(arg);
#endif
      params.append(arg);
    }
  }
  return params;
}

FilePath CommandLine::GetProgram() const {
  return FilePath(argv_[0]);
}

void CommandLine::SetProgram(const FilePath& program) {
  TrimWhitespace(program.value(), TRIM_ALL, &argv_[0]);
}

bool CommandLine::HasSwitch(const std::string& switch_string) const {
  return switches_.find(LowerASCIIOnWindows(switch_string)) != switches_.end();
}

std::string CommandLine::GetSwitchValueASCII(
    const std::string& switch_string) const {
  StringType value = GetSwitchValueNative(switch_string);
  if (!IsStringASCII(value)) {
    DLOG(WARNING) << "Value of switch (" << switch_string << ") must be ASCII.";
    return std::string();
  }
#if defined(OS_WIN)
  return UTF16ToASCII(value);
#else
  return value;
#endif
}

FilePath CommandLine::GetSwitchValuePath(
    const std::string& switch_string) const {
  return FilePath(GetSwitchValueNative(switch_string));
}

CommandLine::StringType CommandLine::GetSwitchValueNative(
    const std::string& switch_string) const {
  SwitchMap::const_iterator result =
    switches_.find(LowerASCIIOnWindows(switch_string));
  return result == switches_.end() ? StringType() : result->second;
}

void CommandLine::AppendSwitch(const std::string& switch_string) {
  AppendSwitchNative(switch_string, StringType());
}

void CommandLine::AppendSwitchPath(const std::string& switch_string,
                                   const FilePath& path) {
  AppendSwitchNative(switch_string, path.value());
}

void CommandLine::AppendSwitchNative(const std::string& switch_string,
                                     const CommandLine::StringType& value) {
  std::string switch_key(LowerASCIIOnWindows(switch_string));
#if defined(OS_WIN)
  StringType combined_switch_string(ASCIIToWide(switch_key));
#elif defined(OS_POSIX)
  StringType combined_switch_string(switch_string);
#endif
  size_t prefix_length = GetSwitchPrefixLength(combined_switch_string);
  switches_[switch_key.substr(prefix_length)] = value;
  // Preserve existing switch prefixes in |argv_|; only append one if necessary.
  if (prefix_length == 0)
    combined_switch_string = kSwitchPrefixes[0] + combined_switch_string;
  if (!value.empty())
    combined_switch_string += kSwitchValueSeparator + value;
  // Append the switch and update the switches/arguments divider |begin_args_|.
  argv_.insert(argv_.begin() + begin_args_++, combined_switch_string);
}

void CommandLine::AppendSwitchASCII(const std::string& switch_string,
                                    const std::string& value_string) {
#if defined(OS_WIN)
  AppendSwitchNative(switch_string, ASCIIToWide(value_string));
#elif defined(OS_POSIX)
  AppendSwitchNative(switch_string, value_string);
#endif
}

void CommandLine::CopySwitchesFrom(const CommandLine& source,
                                   const char* const switches[],
                                   size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (source.HasSwitch(switches[i]))
      AppendSwitchNative(switches[i], source.GetSwitchValueNative(switches[i]));
  }
}

CommandLine::StringVector CommandLine::GetArgs() const {
  // Gather all arguments after the last switch (may include kSwitchTerminator).
  StringVector args(argv_.begin() + begin_args_, argv_.end());
  // Erase only the first kSwitchTerminator (maybe "--" is a legitimate page?)
  StringVector::iterator switch_terminator =
      std::find(args.begin(), args.end(), kSwitchTerminator);
  if (switch_terminator != args.end())
    args.erase(switch_terminator);
  return args;
}

void CommandLine::AppendArg(const std::string& value) {
#if defined(OS_WIN)
  DCHECK(IsStringUTF8(value));
  AppendArgNative(UTF8ToWide(value));
#elif defined(OS_POSIX)
  AppendArgNative(value);
#endif
}

void CommandLine::AppendArgPath(const FilePath& path) {
  AppendArgNative(path.value());
}

void CommandLine::AppendArgNative(const CommandLine::StringType& value) {
  argv_.push_back(value);
}

#if defined(OS_MACOSX)
void CommandLine::FixOrigArgv4Finder(const CommandLine::StringType& value) {
  original_argv_.push_back(value);
}
#endif

void CommandLine::AppendArguments(const CommandLine& other,
                                  bool include_program) {
  if (include_program)
    SetProgram(other.GetProgram());
  AppendSwitchesAndArguments(*this, other.argv());
}

void CommandLine::PrependWrapper(const CommandLine::StringType& wrapper) {
  if (wrapper.empty())
    return;
  // The wrapper may have embedded arguments (like "gdb --args"). In this case,
  // we don't pretend to do anything fancy, we just split on spaces.
  StringVector wrapper_argv;
  SplitString(wrapper, FILE_PATH_LITERAL(' '), &wrapper_argv);
  // Prepend the wrapper and update the switches/arguments |begin_args_|.
  argv_.insert(argv_.begin(), wrapper_argv.begin(), wrapper_argv.end());
  begin_args_ += wrapper_argv.size();
}

#if defined(OS_WIN)
void CommandLine::ParseFromString(const std::wstring& command_line) {
  std::wstring command_line_string;
  TrimWhitespace(command_line, TRIM_ALL, &command_line_string);
  if (command_line_string.empty())
    return;

  int num_args = 0;
  wchar_t** args = NULL;
  args = ::CommandLineToArgvW(command_line_string.c_str(), &num_args);

  DPLOG_IF(FATAL, !args) << "CommandLineToArgvW failed on command line: "
                         << UTF16ToUTF8(command_line);
  InitFromArgv(num_args, args);
  LocalFree(args);
}
#endif

}  // namespace base
