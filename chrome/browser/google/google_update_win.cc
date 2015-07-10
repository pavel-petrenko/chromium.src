// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/google/google_update_win.h"

#include <atlbase.h>
#include <atlcom.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/metrics/histogram.h"
#include "base/metrics/sparse_histogram.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/win/scoped_bstr.h"
#include "base/win/windows_version.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/helper.h"
#include "chrome/installer/util/install_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/win/atl_module.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"

namespace {

// The status of the upgrade. These values are used for a histogram. Do not
// reorder.
enum GoogleUpdateUpgradeStatus {
  // The upgrade has started. DEPRECATED.
  // UPGRADE_STARTED = 0,
  // A check for upgrade has been initiated. DEPRECATED.
  // UPGRADE_CHECK_STARTED = 1,
  // An update is available.
  UPGRADE_IS_AVAILABLE = 2,
  // The upgrade happened successfully.
  UPGRADE_SUCCESSFUL = 3,
  // No need to upgrade, Chrome is up to date.
  UPGRADE_ALREADY_UP_TO_DATE = 4,
  // An error occurred.
  UPGRADE_ERROR = 5,
  NUM_UPGRADE_STATUS
};

GoogleUpdate3ClassFactory* g_google_update_factory = nullptr;

// The time interval, in milliseconds, between polls to Google Update. This
// value was chosen unscientificaly during an informal discussion.
const int64_t kGoogleUpdatePollIntervalMs = 250;

// Constants from Google Update.
const HRESULT GOOPDATE_E_APP_UPDATE_DISABLED_BY_POLICY = 0x80040813;
const HRESULT GOOPDATEINSTALL_E_INSTALLER_FAILED = 0x80040902;

// Check if the currently running instance can be updated by Google Update.
// Returns GOOGLE_UPDATE_NO_ERROR only if the instance running is a Google
// Chrome distribution installed in a standard location.
GoogleUpdateErrorCode CanUpdateCurrentChrome(
    const base::FilePath& chrome_exe_path,
    bool system_level_install) {
  DCHECK_NE(InstallUtil::IsPerUserInstall(chrome_exe_path),
            system_level_install);
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::FilePath user_exe_path = installer::GetChromeInstallPath(false, dist);
  base::FilePath machine_exe_path = installer::GetChromeInstallPath(true, dist);
  if (!base::FilePath::CompareEqualIgnoreCase(chrome_exe_path.value(),
                                        user_exe_path.value()) &&
      !base::FilePath::CompareEqualIgnoreCase(chrome_exe_path.value(),
                                        machine_exe_path.value())) {
    return CANNOT_UPGRADE_CHROME_IN_THIS_DIRECTORY;
  }

  return GOOGLE_UPDATE_NO_ERROR;
}

// Explicitly allow the Google Update service to impersonate the client since
// some COM code elsewhere in the browser process may have previously used
// CoInitializeSecurity to set the impersonation level to something other than
// the default. Ignore errors since an attempt to use Google Update may succeed
// regardless.
void ConfigureProxyBlanket(IUnknown* interface_pointer) {
  ::CoSetProxyBlanket(interface_pointer,
                      RPC_C_AUTHN_DEFAULT,
                      RPC_C_AUTHZ_DEFAULT,
                      COLE_DEFAULT_PRINCIPAL,
                      RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                      RPC_C_IMP_LEVEL_IMPERSONATE,
                      nullptr,
                      EOAC_DYNAMIC_CLOAKING);
}

// Creates a class factory for a COM Local Server class using either plain
// vanilla CoGetClassObject, or using the Elevation moniker if running on
// Vista+. |hwnd| must refer to a foregound window in order to get the UAC
// prompt to appear in the foreground if running on Vista+. It can also be NULL
// if background UAC prompts are desired.
HRESULT CoGetClassObjectAsAdmin(REFCLSID class_id,
                                REFIID interface_id,
                                gfx::AcceleratedWidget hwnd,
                                void** interface_ptr) {
  if (!interface_ptr)
    return E_POINTER;

  // For Vista+, need to instantiate the class factory via the elevation
  // moniker. This ensures that the UAC dialog shows up.
  if (base::win::GetVersion() >= base::win::VERSION_VISTA) {
    wchar_t class_id_as_string[MAX_PATH] = {};
    StringFromGUID2(class_id, class_id_as_string,
                    arraysize(class_id_as_string));

    base::string16 elevation_moniker_name = base::StringPrintf(
        L"Elevation:Administrator!clsid:%ls", class_id_as_string);

    BIND_OPTS3 bind_opts;
    // An explicit memset is needed rather than relying on value initialization
    // since BIND_OPTS3 is not an aggregate (it is a derived type).
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.cbStruct = sizeof(bind_opts);
    bind_opts.dwClassContext = CLSCTX_LOCAL_SERVER;
    bind_opts.hwnd = hwnd;

    return ::CoGetObject(elevation_moniker_name.c_str(), &bind_opts,
                         interface_id, interface_ptr);
  }

  return ::CoGetClassObject(class_id, CLSCTX_LOCAL_SERVER, nullptr,
                            interface_id, interface_ptr);
}

HRESULT CreateGoogleUpdate3WebClass(
    bool system_level_install,
    bool install_update_if_possible,
    gfx::AcceleratedWidget elevation_window,
    base::win::ScopedComPtr<IGoogleUpdate3Web>* google_update) {
  if (g_google_update_factory)
    return g_google_update_factory->Run(google_update);

  const CLSID& google_update_clsid = system_level_install ?
      CLSID_GoogleUpdate3WebMachineClass :
      CLSID_GoogleUpdate3WebUserClass;
  base::win::ScopedComPtr<IClassFactory> class_factory;
  HRESULT hresult = S_OK;

  // For a user-level install, update checks and updates can both be done by a
  // normal user with the UserClass. For a system-level install, update checks
  // can be done by a normal user with the MachineClass.
  if (!system_level_install || !install_update_if_possible) {
    hresult = ::CoGetClassObject(google_update_clsid, CLSCTX_ALL, nullptr,
                                 base::win::ScopedComPtr<IClassFactory>::iid(),
                                 class_factory.ReceiveVoid());
  } else {
    // For a system-level install, an update requires Admin privileges for
    // writing to %ProgramFiles%. Elevate while instantiating the MachineClass.
    hresult = CoGetClassObjectAsAdmin(
        google_update_clsid, base::win::ScopedComPtr<IClassFactory>::iid(),
        elevation_window, class_factory.ReceiveVoid());
  }
  if (FAILED(hresult))
    return hresult;

  ConfigureProxyBlanket(class_factory.get());

  return class_factory->CreateInstance(
      nullptr,
      base::win::ScopedComPtr<IGoogleUpdate3Web>::iid(),
      google_update->ReceiveVoid());
}

// UpdateCheckDriver -----------------------------------------------------------

// A driver that is created and destroyed on the caller's thread and drives
// Google Update on another.
class UpdateCheckDriver {
 public:
  // Runs an update check on |task_runner|, invoking methods of |delegate| on
  // the caller's thread to report progress and final results.
  static void RunUpdateCheck(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      const std::string& locale,
      bool install_update_if_possible,
      gfx::AcceleratedWidget elevation_window,
      const base::WeakPtr<UpdateCheckDelegate>& delegate);

 private:
  friend class base::DeleteHelper<UpdateCheckDriver>;

  UpdateCheckDriver(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      const std::string& locale,
      bool install_update_if_possible,
      gfx::AcceleratedWidget elevation_window,
      const base::WeakPtr<UpdateCheckDelegate>& delegate);

  // Invokes a completion or error method on the caller's delegate, as
  // appropriate.
  ~UpdateCheckDriver();

  // Starts an update check.
  void BeginUpdateCheck();

  // Returns the result of initiating an update check. Sets |error_code| if the
  // result is any kind of failure.
  HRESULT BeginUpdateCheckInternal(GoogleUpdateErrorCode* error_code);

  // Sets status_ to UPGRADE_ERROR, error_code_ to |error_code|, hresult_ to
  // |hresult|, installer_exit_code_ to |installer_exit_code|, and
  // html_error_message_ to a composition of all values suitable for display
  // to the user. This call should be followed by deletion of the driver,
  // which will result in the caller being notified via its delegate.
  void OnUpgradeError(GoogleUpdateErrorCode error_code,
                      HRESULT hresult,
                      int installer_exit_code,
                      const base::string16& error_string);

  // Returns true if |current_state| and |state_value| can be obtained from the
  // ongoing update check. Otherwise, populates |hresult| with the reason they
  // could not be obtained.
  bool GetCurrentState(base::win::ScopedComPtr<ICurrentState>* current_state,
                       CurrentState* state_value,
                       HRESULT* hresult) const;

  // Returns true if |current_state| and |state_value| constitute an error state
  // for the ongoing update check, in which case |error_code| is populated with
  // one of GOOGLE_UPDATE_ERROR_UPDATING, GOOGLE_UPDATE_DISABLED_BY_POLICY, or
  // GOOGLE_UPDATE_ONDEMAND_CLASS_REPORTED_ERROR. |hresult| is populated with
  // the most relevant HRESULT (which may be a value from Google Update; see
  // https://code.google.com/p/omaha/source/browse/trunk/base/error.h). In case
  // Chrome's installer failed during execution, |installer_exit_code| may be
  // populated with its process exit code (see enum installer::InstallStatus in
  // chrome/installer/util/util_constants.h); otherwise, it will be -1.
  // |error_string| will be populated with a completion message if one is
  // provided by Google Update.
  bool IsErrorState(const base::win::ScopedComPtr<ICurrentState>& current_state,
                    CurrentState state_value,
                    GoogleUpdateErrorCode* error_code,
                    HRESULT* hresult,
                    int* installer_exit_code,
                    base::string16* error_string) const;

  // Returns true if |current_state| and |state_value| constitute a final state
  // for the ongoing update check, in which case |upgrade_status| is populated
  // with one of UPGRADE_ALREADY_UP_TO_DATE or UPGRADE_IS_AVAILABLE (in case a
  // pure check is being performed rather than an update) or UPGRADE_SUCCESSFUL
  // (in case an update is being performed). For the UPGRADE_IS_AVAILABLE case,
  // |new_version| will be populated with the available version, if provided by
  // Google Update.
  bool IsFinalState(const base::win::ScopedComPtr<ICurrentState>& current_state,
                    CurrentState state_value,
                    GoogleUpdateUpgradeStatus* upgrade_status,
                    base::string16* new_version) const;

  // Returns true if |current_state| and |state_value| constitute an
  // intermediate state for the ongoing update check. |new_version| will be
  // populated with the version to be installed if it is provided by Google
  // Update for the current state. |progress| will be populated with a number
  // between 0 and 100 according to how far Google Update has progressed in the
  // download and install process.
  bool IsIntermediateState(
      const base::win::ScopedComPtr<ICurrentState>& current_state,
      CurrentState state_value,
      base::string16* new_version,
      int* progress) const;

  // Polls Google Update to determine the state of the ongoing check or
  // update. If the process has reached a terminal state, this instance will be
  // deleted and the caller will be notified of the final status. Otherwise, the
  // caller will be notified of the intermediate state (iff it differs from a
  // previous notification) and another future poll will be scheduled.
  void PollGoogleUpdate();

  // The task runner on which the update checks runs.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // The caller's task runner, on which methods of |delegate_| will be invoked.
  scoped_refptr<base::SingleThreadTaskRunner> result_runner_;

  // The UI locale.
  std::string locale_;

  // False to only check for an update; true to also install one if available.
  bool install_update_if_possible_;

  // A parent window in case any UX is required (e.g., an elevation prompt).
  gfx::AcceleratedWidget elevation_window_;

  // The caller's delegate by which feedback is conveyed.
  base::WeakPtr<UpdateCheckDelegate> delegate_;

  // True if operating on a per-machine installation rather than a per-user one.
  bool system_level_install_;

  // The on-demand updater that is doing the work.
  base::win::ScopedComPtr<IGoogleUpdate3Web> google_update_;

  // An app bundle containing the application being updated.
  base::win::ScopedComPtr<IAppBundleWeb> app_bundle_;

  // The application being updated (Chrome, Chrome Binaries, or Chrome SxS).
  base::win::ScopedComPtr<IAppWeb> app_;

  // The progress value reported most recently to the caller.
  int last_reported_progress_;

  // The results of the update check to be logged via UMA and/or reported to the
  // caller.
  GoogleUpdateUpgradeStatus status_;
  GoogleUpdateErrorCode error_code_;
  base::string16 html_error_message_;
  base::string16 new_version_;
  HRESULT hresult_;
  int installer_exit_code_;

  DISALLOW_COPY_AND_ASSIGN(UpdateCheckDriver);
};

// static
void UpdateCheckDriver::RunUpdateCheck(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const std::string& locale,
    bool install_update_if_possible,
    gfx::AcceleratedWidget elevation_window,
    const base::WeakPtr<UpdateCheckDelegate>& delegate) {
  // The driver is owned by itself, and will self-destruct when its work is
  // done.
  UpdateCheckDriver* driver =
      new UpdateCheckDriver(task_runner, locale, install_update_if_possible,
                            elevation_window, delegate);
  task_runner->PostTask(FROM_HERE,
                        base::Bind(&UpdateCheckDriver::BeginUpdateCheck,
                                   base::Unretained(driver)));
}

// Runs on the caller's thread.
UpdateCheckDriver::UpdateCheckDriver(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const std::string& locale,
    bool install_update_if_possible,
    gfx::AcceleratedWidget elevation_window,
    const base::WeakPtr<UpdateCheckDelegate>& delegate)
    : task_runner_(task_runner),
      result_runner_(base::ThreadTaskRunnerHandle::Get()),
      locale_(locale),
      install_update_if_possible_(install_update_if_possible),
      elevation_window_(elevation_window),
      delegate_(delegate),
      system_level_install_(false),
      last_reported_progress_(0),
      status_(UPGRADE_ERROR),
      error_code_(GOOGLE_UPDATE_NO_ERROR),
      hresult_(S_OK),
      installer_exit_code_(-1) {
}

UpdateCheckDriver::~UpdateCheckDriver() {
  DCHECK(result_runner_->BelongsToCurrentThread());
  // If there is an error, then error_code must not be blank, and vice versa.
  DCHECK_NE(status_ == UPGRADE_ERROR, error_code_ == GOOGLE_UPDATE_NO_ERROR);
  UMA_HISTOGRAM_ENUMERATION("GoogleUpdate.UpgradeResult", status_,
                            NUM_UPGRADE_STATUS);
  if (status_ == UPGRADE_ERROR) {
    UMA_HISTOGRAM_ENUMERATION("GoogleUpdate.UpdateErrorCode", error_code_,
                              NUM_ERROR_CODES);
    if (FAILED(hresult_))
      UMA_HISTOGRAM_SPARSE_SLOWLY("GoogleUpdate.ErrorHresult", hresult_);
    if (installer_exit_code_ != -1) {
      UMA_HISTOGRAM_SPARSE_SLOWLY("GoogleUpdate.InstallerExitCode",
                                  installer_exit_code_);
    }
  }
  if (delegate_) {
    if (status_ == UPGRADE_ERROR)
      delegate_->OnError(error_code_, html_error_message_, new_version_);
    else if (install_update_if_possible_)
      delegate_->OnUpgradeComplete(new_version_);
    else
      delegate_->OnUpdateCheckComplete(new_version_);
  }
}

void UpdateCheckDriver::BeginUpdateCheck() {
  GoogleUpdateErrorCode error_code = GOOGLE_UPDATE_NO_ERROR;
  HRESULT hresult = BeginUpdateCheckInternal(&error_code);
  if (SUCCEEDED(hresult)) {
    // Start polling.
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&UpdateCheckDriver::PollGoogleUpdate,
                                      base::Unretained(this)));
  } else {
    // Return results immediately since the driver is not polling Google Update.
    OnUpgradeError(error_code, hresult, -1, base::string16());
    result_runner_->DeleteSoon(FROM_HERE, this);
  }
}

HRESULT UpdateCheckDriver::BeginUpdateCheckInternal(
    GoogleUpdateErrorCode* error_code) {
  base::FilePath chrome_exe;
  if (!PathService::Get(base::DIR_EXE, &chrome_exe))
    NOTREACHED();

  system_level_install_ = !InstallUtil::IsPerUserInstall(chrome_exe);

  // Make sure ATL is initialized in this module.
  ui::win::CreateATLModuleIfNeeded();

  *error_code = CanUpdateCurrentChrome(chrome_exe, system_level_install_);
  if (*error_code != GOOGLE_UPDATE_NO_ERROR)
    return E_FAIL;

  HRESULT hresult = CreateGoogleUpdate3WebClass(
      system_level_install_, install_update_if_possible_, elevation_window_,
      &google_update_);
  if (FAILED(hresult)) {
    *error_code = GOOGLE_UPDATE_ONDEMAND_CLASS_NOT_FOUND;
    return hresult;
  }

  ConfigureProxyBlanket(google_update_.get());

  // The class was created, so all subsequent errors are reported as:
  *error_code = GOOGLE_UPDATE_ONDEMAND_CLASS_REPORTED_ERROR;
  base::string16 app_guid =
      installer::GetAppGuidForUpdates(system_level_install_);
  DCHECK(!app_guid.empty());

  {
    base::win::ScopedComPtr<IDispatch> dispatch;
    hresult = google_update_->createAppBundleWeb(dispatch.Receive());
    if (FAILED(hresult))
      return hresult;
    hresult = dispatch.QueryInterface(app_bundle_.Receive());
    if (FAILED(hresult))
      return hresult;
  }
  ConfigureProxyBlanket(app_bundle_.get());

  if (!locale_.empty()) {
    // Ignore the result of this since, while setting the display language is
    // nice to have, a failure to do so does not affect the likelihood that the
    // update check and/or install will succeed.
    app_bundle_->put_displayLanguage(
        base::win::ScopedBstr(base::UTF8ToUTF16(locale_).c_str()));
  }

  hresult = app_bundle_->initialize();
  if (FAILED(hresult))
    return hresult;
  if (elevation_window_) {
    // Likewise, a failure to set the parent window need not block an update
    // check.
    app_bundle_->put_parentHWND(reinterpret_cast<ULONG_PTR>(elevation_window_));
  }

  base::win::ScopedComPtr<IDispatch> dispatch;
  hresult =
      app_bundle_->createInstalledApp(base::win::ScopedBstr(app_guid.c_str()));
  if (FAILED(hresult))
    return hresult;
  hresult = app_bundle_->get_appWeb(0, dispatch.Receive());
  if (FAILED(hresult))
    return hresult;
  hresult = dispatch.QueryInterface(app_.Receive());
  if (FAILED(hresult))
    return hresult;
  ConfigureProxyBlanket(app_.get());
  return app_bundle_->checkForUpdate();
}

bool UpdateCheckDriver::GetCurrentState(
    base::win::ScopedComPtr<ICurrentState>* current_state,
    CurrentState* state_value,
    HRESULT* hresult) const {
  base::win::ScopedComPtr<IDispatch> dispatch;
  *hresult = app_->get_currentState(dispatch.Receive());
  if (FAILED(*hresult))
    return false;
  *hresult = dispatch.QueryInterface(current_state->Receive());
  if (FAILED(*hresult))
    return false;
  ConfigureProxyBlanket(current_state->get());
  LONG value = 0;
  *hresult = (*current_state)->get_stateValue(&value);
  if (FAILED(*hresult))
    return false;
  *state_value = static_cast<CurrentState>(value);
  return true;
}

bool UpdateCheckDriver::IsErrorState(
    const base::win::ScopedComPtr<ICurrentState>& current_state,
    CurrentState state_value,
    GoogleUpdateErrorCode* error_code,
    HRESULT* hresult,
    int* installer_exit_code,
    base::string16* error_string) const {
  if (state_value == STATE_ERROR) {
    // In general, errors reported by Google Update fall under this category
    // (see special case below).
    *error_code = GOOGLE_UPDATE_ERROR_UPDATING;

    // In general, the exit code of Chrome's installer is unknown (see special
    // case below).
    *installer_exit_code = -1;

    // Report the error_code provided by Google Update if possible, or the
    // reason it wasn't possible otherwise.
    LONG long_value = 0;
    *hresult = current_state->get_errorCode(&long_value);
    if (SUCCEEDED(*hresult))
      *hresult = long_value;

    // Special cases:
    // - Use a custom error code if Google Update repoted that the update was
    //   disabled by a Group Policy setting.
    // - Extract the exit code of Chrome's installer if Google Update repoted
    //   that the update failed because of a failure in the installer.
    LONG code = 0;
    if (*hresult == GOOPDATE_E_APP_UPDATE_DISABLED_BY_POLICY) {
      *error_code = GOOGLE_UPDATE_DISABLED_BY_POLICY;
    } else if (*hresult == GOOPDATEINSTALL_E_INSTALLER_FAILED &&
               SUCCEEDED(current_state->get_installerResultCode(&code))) {
      *installer_exit_code = code;
    }

    base::win::ScopedBstr message;
    if (SUCCEEDED(current_state->get_completionMessage(message.Receive())))
      error_string->assign(message, message.Length());

    return true;
  }
  if (state_value == STATE_UPDATE_AVAILABLE && install_update_if_possible_) {
    *hresult = app_bundle_->install();
    if (FAILED(*hresult)) {
      // Report a failure to start the install as a general error while trying
      // to interact with Google Update.
      *error_code = GOOGLE_UPDATE_ONDEMAND_CLASS_REPORTED_ERROR;
      *installer_exit_code = -1;
      return true;
    }
    // Return false for handling in IsIntermediateState.
  }
  return false;
}

bool UpdateCheckDriver::IsFinalState(
    const base::win::ScopedComPtr<ICurrentState>& current_state,
    CurrentState state_value,
    GoogleUpdateUpgradeStatus* upgrade_status,
    base::string16* new_version) const {
  if (state_value == STATE_UPDATE_AVAILABLE && !install_update_if_possible_) {
    base::win::ScopedBstr version;
    *upgrade_status = UPGRADE_IS_AVAILABLE;
    if (SUCCEEDED(current_state->get_availableVersion(version.Receive())))
      new_version->assign(version, version.Length());
    return true;
  }
  if (state_value == STATE_INSTALL_COMPLETE) {
    DCHECK(install_update_if_possible_);
    *upgrade_status = UPGRADE_SUCCESSFUL;
    return true;
  }
  if (state_value == STATE_NO_UPDATE) {
    *upgrade_status = UPGRADE_ALREADY_UP_TO_DATE;
    return true;
  }
  return false;
}

bool UpdateCheckDriver::IsIntermediateState(
    const base::win::ScopedComPtr<ICurrentState>& current_state,
    CurrentState state_value,
    base::string16* new_version,
    int* progress) const {
  // ERROR will have been handled in IsErrorState. UPDATE_AVAILABLE, and
  // NO_UPDATE will have been handled in IsFinalState if not doing an install,
  // as will STATE_INSTALL_COMPLETE when doing an install. All other states
  // following UPDATE_AVAILABLE will only happen when an install is to be done.
  DCHECK(state_value < STATE_UPDATE_AVAILABLE || install_update_if_possible_);
  *progress = 0;

  switch (state_value) {
    case STATE_INIT:
    case STATE_WAITING_TO_CHECK_FOR_UPDATE:
    case STATE_CHECKING_FOR_UPDATE:
      // There is no news to report yet.
      break;

    case STATE_UPDATE_AVAILABLE: {
      base::win::ScopedBstr version;
      if (SUCCEEDED(current_state->get_availableVersion(version.Receive())))
        new_version->assign(version, version.Length());
      break;
    }

    case STATE_WAITING_TO_DOWNLOAD:
    case STATE_RETRYING_DOWNLOAD:
      break;

    case STATE_DOWNLOADING: {
      ULONG bytes_downloaded = 0;
      ULONG total_bytes = 0;
      if (SUCCEEDED(current_state->get_bytesDownloaded(&bytes_downloaded)) &&
          SUCCEEDED(current_state->get_totalBytesToDownload(&total_bytes)) &&
          total_bytes) {
        // 0-50 is downloading.
        *progress = gfx::ToFlooredInt((static_cast<double>(bytes_downloaded) /
                                       static_cast<double>(total_bytes)) *
                                      50.0);
      }
      break;
    }

    case STATE_DOWNLOAD_COMPLETE:
    case STATE_EXTRACTING:
    case STATE_APPLYING_DIFFERENTIAL_PATCH:
    case STATE_READY_TO_INSTALL:
    case STATE_WAITING_TO_INSTALL:
      *progress = 50;
      break;

    case STATE_INSTALLING: {
      *progress = 50;
      LONG install_progress = 0;
      if (SUCCEEDED(current_state->get_installProgress(&install_progress)) &&
          install_progress >= 0 && install_progress <= 100) {
        // 50-100 is installing.
        *progress = (50 + install_progress / 2);
      }
      break;
    }

    case STATE_INSTALL_COMPLETE:
    case STATE_PAUSED:
    case STATE_NO_UPDATE:
    case STATE_ERROR:
    default:
      NOTREACHED();
      UMA_HISTOGRAM_SPARSE_SLOWLY("GoogleUpdate.UnexpectedState", state_value);
      return false;
  }
  return true;
}

void UpdateCheckDriver::PollGoogleUpdate() {
  base::win::ScopedComPtr<ICurrentState> state;
  CurrentState state_value = STATE_INIT;
  HRESULT hresult = S_OK;
  GoogleUpdateErrorCode error_code = GOOGLE_UPDATE_NO_ERROR;
  int installer_exit_code = -1;
  base::string16 error_string;
  GoogleUpdateUpgradeStatus upgrade_status = UPGRADE_ERROR;
  base::string16 new_version;
  int progress = 0;

  if (!GetCurrentState(&state, &state_value, &hresult)) {
    OnUpgradeError(GOOGLE_UPDATE_ONDEMAND_CLASS_REPORTED_ERROR, hresult, -1,
                   base::string16());
  } else if (IsErrorState(state, state_value, &error_code, &hresult,
                          &installer_exit_code, &error_string)) {
    OnUpgradeError(error_code, hresult, installer_exit_code, error_string);
  } else if (IsFinalState(state, state_value, &upgrade_status, &new_version)) {
    status_ = upgrade_status;
    error_code_ = GOOGLE_UPDATE_NO_ERROR;
    html_error_message_.clear();
    if (!new_version.empty())
      new_version_ = new_version;
    hresult_ = S_OK;
    installer_exit_code_ = -1;
  } else if (IsIntermediateState(state, state_value, &new_version, &progress)) {
    bool got_new_version = new_version_.empty() && !new_version.empty();
    if (got_new_version)
      new_version_ = new_version;
    // Give the caller this status update if it differs from the last one given.
    if (got_new_version || progress != last_reported_progress_) {
      last_reported_progress_ = progress;

      // It is safe to post this task with an unretained pointer since the task
      // is guaranteed to run before a subsequent DeleteSoon is handled.
      result_runner_->PostTask(
          FROM_HERE,
          base::Bind(&UpdateCheckDelegate::OnUpgradeProgress, delegate_,
                     last_reported_progress_, new_version_));
    }

    // Schedule the next check.
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&UpdateCheckDriver::PollGoogleUpdate,
                              base::Unretained(this)),
        base::TimeDelta::FromMilliseconds(kGoogleUpdatePollIntervalMs));
    // Early return for this non-terminal state.
    return;
  }

  // Release the reference on the COM objects before bouncing back to the
  // caller's thread.
  state.Release();
  app_.Release();
  app_bundle_.Release();
  google_update_.Release();

  result_runner_->DeleteSoon(FROM_HERE, this);
}

void UpdateCheckDriver::OnUpgradeError(GoogleUpdateErrorCode error_code,
                                       HRESULT hresult,
                                       int installer_exit_code,
                                       const base::string16& error_string) {
  status_ = UPGRADE_ERROR;
  error_code_ = error_code;
  hresult_ = hresult;
  installer_exit_code_ = installer_exit_code;
  base::string16 html_error_msg =
      base::StringPrintf(L"%d: <a href='%ls0x%X' target=_blank>0x%X</a>",
                         error_code_, base::UTF8ToUTF16(
                             chrome::kUpgradeHelpCenterBaseURL).c_str(),
                         hresult_, hresult_);
  if (installer_exit_code_ != -1)
    html_error_msg += base::StringPrintf(L": %d", installer_exit_code_);
  if (system_level_install_)
    html_error_msg += L" -- system level";
  if (error_string.empty()) {
    html_error_message_ = l10n_util::GetStringFUTF16(
        IDS_ABOUT_BOX_ERROR_UPDATE_CHECK_FAILED, html_error_msg);
  } else {
    html_error_message_ = l10n_util::GetStringFUTF16(
        IDS_ABOUT_BOX_GOOGLE_UPDATE_ERROR, error_string, html_error_msg);
  }
}

}  // namespace


// Globals ---------------------------------------------------------------------

void BeginUpdateCheck(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const std::string& locale,
    bool install_update_if_possible,
    gfx::AcceleratedWidget elevation_window,
    const base::WeakPtr<UpdateCheckDelegate>& delegate) {
  UpdateCheckDriver::RunUpdateCheck(task_runner, locale,
                                    install_update_if_possible,
                                    elevation_window, delegate);
}


// Private API exposed for testing. --------------------------------------------

void SetGoogleUpdateFactoryForTesting(
    const GoogleUpdate3ClassFactory& google_update_factory) {
  if (g_google_update_factory) {
    delete g_google_update_factory;
    g_google_update_factory = nullptr;
  }
  if (!google_update_factory.is_null()) {
    g_google_update_factory =
        new GoogleUpdate3ClassFactory(google_update_factory);
  }
}
