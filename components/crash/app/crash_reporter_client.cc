// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/app/crash_reporter_client.h"

#include "base/files/file_path.h"
#include "base/logging.h"

namespace crash_reporter {

namespace {

CrashReporterClient* g_client = NULL;

}  // namespace

void SetCrashReporterClient(CrashReporterClient* client) {
  g_client = client;
}

CrashReporterClient* GetCrashReporterClient() {
  DCHECK(g_client);
  return g_client;
}

CrashReporterClient::CrashReporterClient() {}
CrashReporterClient::~CrashReporterClient() {}

void CrashReporterClient::SetCrashReporterClientIdFromGUID(
    const std::string& client_guid) {
}

#if defined(OS_WIN)
bool CrashReporterClient::GetAlternativeCrashDumpLocation(
    base::FilePath* crash_dir) {
  return false;
}

void CrashReporterClient::GetProductNameAndVersion(
    const base::FilePath& exe_path,
    base::string16* product_name,
    base::string16* version,
    base::string16* special_build,
    base::string16* channel_name) {
}

bool CrashReporterClient::ShouldShowRestartDialog(base::string16* title,
                                                  base::string16* message,
                                                  bool* is_rtl_locale) {
  return false;
}

bool CrashReporterClient::AboutToRestart() {
  return false;
}

bool CrashReporterClient::GetDeferredUploadsSupported(bool is_per_usr_install) {
  return false;
}

bool CrashReporterClient::GetIsPerUserInstall(const base::FilePath& exe_path) {
  return true;
}

bool CrashReporterClient::GetShouldDumpLargerDumps(bool is_per_user_install) {
  return false;
}

int CrashReporterClient::GetResultCodeRespawnFailed() {
  return 0;
}

void CrashReporterClient::InitBrowserCrashDumpsRegKey() {
}

void CrashReporterClient::RecordCrashDumpAttempt(bool is_real_crash) {
}
#endif

#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_IOS)
void CrashReporterClient::GetProductNameAndVersion(std::string* product_name,
                                                   std::string* version) {
}

base::FilePath CrashReporterClient::GetReporterLogFilename() {
  return base::FilePath();
}
#endif

bool CrashReporterClient::GetCrashDumpLocation(base::FilePath* crash_dir) {
  return false;
}

size_t CrashReporterClient::RegisterCrashKeys() {
  return 0;
}

bool CrashReporterClient::IsRunningUnattended() {
  return true;
}

bool CrashReporterClient::GetCollectStatsConsent() {
  return false;
}

#if defined(OS_WIN) || defined(OS_MACOSX)
bool CrashReporterClient::ReportingIsEnforcedByPolicy(bool* breakpad_enabled) {
  return false;
}
#endif

#if defined(OS_ANDROID)
int CrashReporterClient::GetAndroidMinidumpDescriptor() {
  return 0;
}
#endif

#if defined(OS_MACOSX)
void CrashReporterClient::InstallAdditionalFilters(BreakpadRef breakpad) {
}
#endif

bool CrashReporterClient::EnableBreakpadForProcess(
    const std::string& process_type) {
  return false;
}

}  // namespace crash_reporter
