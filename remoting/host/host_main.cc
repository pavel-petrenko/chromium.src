// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file implements the common entry point shared by all Chromoting Host
// processes.

#include "remoting/host/host_main.h"

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/stringize_macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "remoting/base/breakpad.h"
#include "remoting/host/host_exit_codes.h"
#include "remoting/host/logging.h"
#include "remoting/host/usage_stats_consent.h"

#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif  // defined(OS_MACOSX)

#if defined(OS_WIN)
#include <commctrl.h>
#include <shellapi.h>
#endif  // defined(OS_WIN)

namespace remoting {

// Known entry points.
int DaemonProcessMain();
int DesktopProcessMain();
int ElevatedControllerMain();
int HostProcessMain();
int RdpDesktopSessionMain();

const char kElevateSwitchName[] = "elevate";
const char kProcessTypeSwitchName[] = "type";

const char kProcessTypeController[] = "controller";
const char kProcessTypeDaemon[] = "daemon";
const char kProcessTypeDesktop[] = "desktop";
const char kProcessTypeHost[] = "host";
const char kProcessTypeRdpDesktopSession[] = "rdp_desktop_session";

namespace {

typedef int (*MainRoutineFn)();

// "--help" or "--?" prints the usage message.
const char kHelpSwitchName[] = "help";
const char kQuestionSwitchName[] = "?";

// The command line switch used to get version of the daemon.
const char kVersionSwitchName[] = "version";

const char kUsageMessage[] =
  "Usage: %s [options]\n"
  "\n"
  "Options:\n"
  "  --audio-pipe-name=<pipe> - Sets the pipe name to capture audio on Linux.\n"
  "  --console                - Runs the daemon interactively.\n"
  "  --daemon-pipe=<pipe>     - Specifies the pipe to connect to the daemon.\n"
  "  --elevate=<binary>       - Runs <binary> elevated.\n"
  "  --host-config=<config>   - Specifies the host configuration.\n"
  "  --help, -?               - Print this message.\n"
  "  --type                   - Specifies process type.\n"
  "  --version                - Prints the host version and exits.\n";

void Usage(const base::FilePath& program_name) {
  printf(kUsageMessage, program_name.MaybeAsASCII().c_str());
}

#if defined(OS_WIN)

// Runs the binary specified by the command line, elevated.
int RunElevated() {
  const CommandLine::SwitchMap& switches =
      CommandLine::ForCurrentProcess()->GetSwitches();
  const CommandLine::StringVector& args =
      CommandLine::ForCurrentProcess()->GetArgs();

  // Create the child process command line by copying switches from the current
  // command line.
  CommandLine command_line(CommandLine::NO_PROGRAM);
  for (CommandLine::SwitchMap::const_iterator i = switches.begin();
       i != switches.end(); ++i) {
    if (i->first != kElevateSwitchName)
      command_line.AppendSwitchNative(i->first, i->second);
  }
  for (CommandLine::StringVector::const_iterator i = args.begin();
       i != args.end(); ++i) {
    command_line.AppendArgNative(*i);
  }

  // Get the name of the binary to launch.
  base::FilePath binary =
      CommandLine::ForCurrentProcess()->GetSwitchValuePath(kElevateSwitchName);
  CommandLine::StringType parameters = command_line.GetCommandLineString();

  // Launch the child process requesting elevation.
  SHELLEXECUTEINFO info;
  memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);
  info.lpVerb = L"runas";
  info.lpFile = binary.value().c_str();
  info.lpParameters = parameters.c_str();
  info.nShow = SW_SHOWNORMAL;

  if (!ShellExecuteEx(&info)) {
    DWORD exit_code = GetLastError();
    LOG_GETLASTERROR(ERROR) << "Unable to launch '" << binary.value() << "'";
    return exit_code;
  }

  return kSuccessExitCode;
}

#else  // !defined(OS_WIN)

// Fake entry points that exist only on Windows.
int DaemonProcessMain() {
  NOTREACHED();
  return kInitializationFailed;
}

int DesktopProcessMain() {
  NOTREACHED();
  return kInitializationFailed;
}

int ElevatedControllerMain() {
  NOTREACHED();
  return kInitializationFailed;
}

int RdpDesktopSessionMain() {
  NOTREACHED();
  return kInitializationFailed;
}

#endif  // !defined(OS_WIN)

// Select the entry point corresponding to the process type.
MainRoutineFn SelectMainRoutine(const std::string& process_type) {
  MainRoutineFn main_routine = NULL;

  if (process_type == kProcessTypeDaemon) {
    main_routine = &DaemonProcessMain;
  } else if (process_type == kProcessTypeDesktop) {
    main_routine = &DesktopProcessMain;
  } else if (process_type == kProcessTypeController) {
    main_routine = &ElevatedControllerMain;
  } else if (process_type == kProcessTypeRdpDesktopSession) {
    main_routine = &RdpDesktopSessionMain;
  } else if (process_type == kProcessTypeHost) {
    main_routine = &HostProcessMain;
  }

  return main_routine;
}

}  // namespace

int HostMain(int argc, char** argv) {
#if defined(OS_MACOSX)
  // Needed so we don't leak objects when threads are created.
  base::mac::ScopedNSAutoreleasePool pool;
#endif

  CommandLine::Init(argc, argv);

  // Initialize Breakpad as early as possible. On Mac the command-line needs to
  // be initialized first, so that the preference for crash-reporting can be
  // looked up in the config file.
#if defined(REMOTING_ENABLE_BREAKPAD)
  if (IsUsageStatsAllowed()) {
    InitializeCrashReporting();
  }
#endif  // defined(REMOTING_ENABLE_BREAKPAD)

  // This object instance is required by Chrome code (for example,
  // LazyInstance, MessageLoop).
  base::AtExitManager exit_manager;

  // Enable debug logs.
  InitHostLogging();

  // Register and initialize common controls.
#if defined(OS_WIN)
  INITCOMMONCONTROLSEX info;
  info.dwSize = sizeof(info);
  info.dwICC = ICC_STANDARD_CLASSES;
  InitCommonControlsEx(&info);
#endif  // defined(OS_WIN)

  // Parse the command line.
  const CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kHelpSwitchName) ||
      command_line->HasSwitch(kQuestionSwitchName)) {
    Usage(command_line->GetProgram());
    return kSuccessExitCode;
  }

  if (command_line->HasSwitch(kVersionSwitchName)) {
    printf("%s\n", STRINGIZE(VERSION));
    return kSuccessExitCode;
  }

#if defined(OS_WIN)
  if (command_line->HasSwitch(kElevateSwitchName)) {
    return RunElevated();
  }
#endif  // defined(OS_WIN)

  // Assume the host process by default.
  std::string process_type = kProcessTypeHost;
  if (command_line->HasSwitch(kProcessTypeSwitchName)) {
    process_type = command_line->GetSwitchValueASCII(kProcessTypeSwitchName);
  }

  MainRoutineFn main_routine = SelectMainRoutine(process_type);
  if (!main_routine) {
    fprintf(stderr, "Unknown process type '%s' specified.",
            process_type.c_str());
    Usage(command_line->GetProgram());
    return kUsageExitCode;
  }

  // Invoke the entry point.
  int exit_code = main_routine();
  if (exit_code == kUsageExitCode) {
    Usage(command_line->GetProgram());
  }
  return exit_code;
}

}  // namespace remoting
