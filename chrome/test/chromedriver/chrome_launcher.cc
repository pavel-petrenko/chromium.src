// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome_launcher.h"

#include <algorithm>
#include <vector>

#include "base/base64.h"
#include "base/basictypes.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/format_macros.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/test/chromedriver/chrome/chrome_android_impl.h"
#include "chrome/test/chromedriver/chrome/chrome_desktop_impl.h"
#include "chrome/test/chromedriver/chrome/chrome_finder.h"
#include "chrome/test/chromedriver/chrome/chrome_remote_impl.h"
#include "chrome/test/chromedriver/chrome/device_manager.h"
#include "chrome/test/chromedriver/chrome/devtools_client_impl.h"
#include "chrome/test/chromedriver/chrome/devtools_event_listener.h"
#include "chrome/test/chromedriver/chrome/devtools_http_client.h"
#include "chrome/test/chromedriver/chrome/embedded_automation_extension.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/chrome/user_data_dir.h"
#include "chrome/test/chromedriver/chrome/version.h"
#include "chrome/test/chromedriver/chrome/web_view.h"
#include "chrome/test/chromedriver/net/port_server.h"
#include "chrome/test/chromedriver/net/url_request_context_getter.h"
#include "crypto/rsa_private_key.h"
#include "crypto/sha2.h"
#include "third_party/zlib/google/zip.h"

#if defined(OS_POSIX)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#elif defined(OS_WIN)
#include "base/win/scoped_handle.h"
#include "chrome/test/chromedriver/keycode_text_conversion.h"
#endif

namespace {

const char* const kCommonSwitches[] = {
    "ignore-certificate-errors", "metrics-recording-only"};

#if defined(OS_LINUX)
const char kEnableCrashReport[] = "enable-crash-reporter-for-testing";
#endif

Status UnpackAutomationExtension(const base::FilePath& temp_dir,
                                 base::FilePath* automation_extension) {
  std::string decoded_extension;
  if (!base::Base64Decode(kAutomationExtension, &decoded_extension))
    return Status(kUnknownError, "failed to base64decode automation extension");

  base::FilePath extension_zip = temp_dir.AppendASCII("internal.zip");
  int size = static_cast<int>(decoded_extension.length());
  if (base::WriteFile(extension_zip, decoded_extension.c_str(), size)
      != size) {
    return Status(kUnknownError, "failed to write automation extension zip");
  }

  base::FilePath extension_dir = temp_dir.AppendASCII("internal");
  if (!zip::Unzip(extension_zip, extension_dir))
    return Status(kUnknownError, "failed to unzip automation extension");

  *automation_extension = extension_dir;
  return Status(kOk);
}

Status PrepareCommandLine(uint16 port,
                          const Capabilities& capabilities,
                          base::CommandLine* prepared_command,
                          base::ScopedTempDir* user_data_dir,
                          base::ScopedTempDir* extension_dir,
                          std::vector<std::string>* extension_bg_pages) {
  base::FilePath program = capabilities.binary;
  if (program.empty()) {
    if (!FindChrome(&program))
      return Status(kUnknownError, "cannot find Chrome binary");
  } else if (!base::PathExists(program)) {
    return Status(kUnknownError,
                  base::StringPrintf("no chrome binary at %" PRFilePath,
                                     program.value().c_str()));
  }
  base::CommandLine command(program);
  Switches switches;

  for (size_t i = 0; i < arraysize(kCommonSwitches); ++i)
    switches.SetSwitch(kCommonSwitches[i]);
  switches.SetSwitch("disable-hang-monitor");
  switches.SetSwitch("disable-prompt-on-repost");
  switches.SetSwitch("disable-sync");
  switches.SetSwitch("no-first-run");
  switches.SetSwitch("disable-background-networking");
  switches.SetSwitch("disable-web-resources");
  switches.SetSwitch("safebrowsing-disable-auto-update");
  switches.SetSwitch("safebrowsing-disable-download-protection");
  switches.SetSwitch("disable-client-side-phishing-detection");
  switches.SetSwitch("disable-component-update");
  switches.SetSwitch("disable-default-apps");
  switches.SetSwitch("enable-logging");
  switches.SetSwitch("log-level", "0");
  switches.SetSwitch("password-store", "basic");
  switches.SetSwitch("use-mock-keychain");
  switches.SetSwitch("remote-debugging-port", base::IntToString(port));
  switches.SetSwitch("test-type", "webdriver");

  for (std::set<std::string>::const_iterator iter =
           capabilities.exclude_switches.begin();
       iter != capabilities.exclude_switches.end();
       ++iter) {
    switches.RemoveSwitch(*iter);
  }
  switches.SetFromSwitches(capabilities.switches);
  base::FilePath user_data_dir_path;
  if (!switches.HasSwitch("user-data-dir")) {
    command.AppendArg("data:,");
    if (!user_data_dir->CreateUniqueTempDir())
      return Status(kUnknownError, "cannot create temp dir for user data dir");
    switches.SetSwitch("user-data-dir", user_data_dir->path().value());
    user_data_dir_path = user_data_dir->path();
  } else {
    user_data_dir_path = base::FilePath(
        switches.GetSwitchValueNative("user-data-dir"));
  }

  Status status = internal::PrepareUserDataDir(user_data_dir_path,
                                               capabilities.prefs.get(),
                                               capabilities.local_state.get());
  if (status.IsError())
    return status;

  if (!extension_dir->CreateUniqueTempDir()) {
    return Status(kUnknownError,
                  "cannot create temp dir for unpacking extensions");
  }
  status = internal::ProcessExtensions(capabilities.extensions,
                                       extension_dir->path(),
                                       true,
                                       &switches,
                                       extension_bg_pages);
  if (status.IsError())
    return status;
  switches.AppendToCommandLine(&command);
  *prepared_command = command;
  return Status(kOk);
}

Status WaitForDevToolsAndCheckVersion(
    const NetAddress& address,
    URLRequestContextGetter* context_getter,
    const SyncWebSocketFactory& socket_factory,
    const Capabilities* capabilities,
    scoped_ptr<DevToolsHttpClient>* user_client) {
  scoped_ptr<DeviceMetrics> device_metrics;
  if (capabilities && capabilities->device_metrics)
    device_metrics.reset(new DeviceMetrics(*capabilities->device_metrics));

  scoped_ptr<DevToolsHttpClient> client(new DevToolsHttpClient(
      address, context_getter, socket_factory, device_metrics.Pass()));
  base::TimeTicks deadline =
      base::TimeTicks::Now() + base::TimeDelta::FromSeconds(60);
  Status status = client->Init(deadline - base::TimeTicks::Now());
  if (status.IsError())
    return status;
  if (client->browser_info()->build_no < kMinimumSupportedChromeBuildNo) {
    return Status(kUnknownError, "Chrome version must be >= " +
        GetMinimumSupportedChromeVersion());
  }

  while (base::TimeTicks::Now() < deadline) {
    WebViewsInfo views_info;
    client->GetWebViewsInfo(&views_info);
    for (size_t i = 0; i < views_info.GetSize(); ++i) {
      if (views_info.Get(i).type == WebViewInfo::kPage) {
        *user_client = client.Pass();
        return Status(kOk);
      }
    }
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(50));
  }
  return Status(kUnknownError, "unable to discover open pages");
}

Status CreateBrowserwideDevToolsClientAndConnect(
    const NetAddress& address,
    const PerfLoggingPrefs& perf_logging_prefs,
    const SyncWebSocketFactory& socket_factory,
    const ScopedVector<DevToolsEventListener>& devtools_event_listeners,
    scoped_ptr<DevToolsClient>* browser_client) {
  scoped_ptr<DevToolsClient> client(new DevToolsClientImpl(
      socket_factory, base::StringPrintf("ws://%s/devtools/browser/",
                                         address.ToString().c_str()),
      DevToolsClientImpl::kBrowserwideDevToolsClientId));
  for (ScopedVector<DevToolsEventListener>::const_iterator it =
          devtools_event_listeners.begin();
      it != devtools_event_listeners.end();
      ++it) {
    // Only add listeners that subscribe to the browser-wide |DevToolsClient|.
    // Otherwise, listeners will think this client is associated with a webview,
    // and will send unrecognized commands to it.
    if ((*it)->subscribes_to_browser())
      client->AddListener(*it);
  }
  // Provide the client regardless of whether it connects, so that Chrome always
  // has a valid |devtools_websocket_client_|. If not connected, no listeners
  // will be notified, and client will just return kDisconnected errors if used.
  *browser_client = client.Pass();
  // To avoid unnecessary overhead, only connect if tracing is enabled, since
  // the browser-wide client is currently only used for tracing.
  if (!perf_logging_prefs.trace_categories.empty()) {
    Status status = (*browser_client)->ConnectIfNecessary();
    if (status.IsError())
      return status;
  }
  return Status(kOk);
}

Status LaunchRemoteChromeSession(
    URLRequestContextGetter* context_getter,
    const SyncWebSocketFactory& socket_factory,
    const Capabilities& capabilities,
    ScopedVector<DevToolsEventListener>* devtools_event_listeners,
    scoped_ptr<Chrome>* chrome) {
  Status status(kOk);
  scoped_ptr<DevToolsHttpClient> devtools_http_client;
  status = WaitForDevToolsAndCheckVersion(
      capabilities.debugger_address, context_getter, socket_factory,
      NULL, &devtools_http_client);
  if (status.IsError()) {
    return Status(kUnknownError, "cannot connect to chrome at " +
                      capabilities.debugger_address.ToString(),
                  status);
  }

  scoped_ptr<DevToolsClient> devtools_websocket_client;
  status = CreateBrowserwideDevToolsClientAndConnect(
      capabilities.debugger_address, capabilities.perf_logging_prefs,
      socket_factory, *devtools_event_listeners, &devtools_websocket_client);
  if (status.IsError()) {
    LOG(WARNING) << "Browser-wide DevTools client failed to connect: "
                 << status.message();
  }

  chrome->reset(new ChromeRemoteImpl(devtools_http_client.Pass(),
                                     devtools_websocket_client.Pass(),
                                     *devtools_event_listeners));
  return Status(kOk);
}

Status LaunchDesktopChrome(
    URLRequestContextGetter* context_getter,
    uint16 port,
    scoped_ptr<PortReservation> port_reservation,
    const SyncWebSocketFactory& socket_factory,
    const Capabilities& capabilities,
    ScopedVector<DevToolsEventListener>* devtools_event_listeners,
    scoped_ptr<Chrome>* chrome) {
  base::CommandLine command(base::CommandLine::NO_PROGRAM);
  base::ScopedTempDir user_data_dir;
  base::ScopedTempDir extension_dir;
  std::vector<std::string> extension_bg_pages;
  Status status = PrepareCommandLine(port,
                                     capabilities,
                                     &command,
                                     &user_data_dir,
                                     &extension_dir,
                                     &extension_bg_pages);
  if (status.IsError())
    return status;

  base::LaunchOptions options;

#if defined(OS_LINUX)
  // If minidump path is set in the capability, enable minidump for crashes.
  if (!capabilities.minidump_path.empty()) {
    VLOG(0) << "Minidump generation specified. Will save dumps to: "
            << capabilities.minidump_path;

    options.environ["CHROME_HEADLESS"] = 1;
    options.environ["BREAKPAD_DUMP_LOCATION"] = capabilities.minidump_path;

    if (!command.HasSwitch(kEnableCrashReport))
      command.AppendSwitch(kEnableCrashReport);
  }

  // We need to allow new privileges so that chrome's setuid sandbox can run.
  options.allow_new_privs = true;
#endif

#if !defined(OS_WIN)
  if (!capabilities.log_path.empty())
    options.environ["CHROME_LOG_FILE"] = capabilities.log_path;
  if (capabilities.detach)
    options.new_process_group = true;
#endif

#if defined(OS_POSIX)
  base::FileHandleMappingVector no_stderr;
  base::ScopedFD devnull;
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch("verbose")) {
    // Redirect stderr to /dev/null, so that Chrome log spew doesn't confuse
    // users.
    devnull.reset(HANDLE_EINTR(open("/dev/null", O_WRONLY)));
    if (!devnull.is_valid())
      return Status(kUnknownError, "couldn't open /dev/null");
    no_stderr.push_back(std::make_pair(devnull.get(), STDERR_FILENO));
    options.fds_to_remap = &no_stderr;
  }
#elif defined(OS_WIN)
  // Silence chrome error message.
  HANDLE out_read;
  HANDLE out_write;
  SECURITY_ATTRIBUTES sa_attr;

  sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa_attr.bInheritHandle = TRUE;
  sa_attr.lpSecurityDescriptor = NULL;
  if (!CreatePipe(&out_read, &out_write, &sa_attr, 0))
      return Status(kUnknownError, "CreatePipe() - Pipe creation failed");
  // Prevent handle leak.
  base::win::ScopedHandle scoped_out_read(out_read);
  base::win::ScopedHandle scoped_out_write(out_write);

  options.stdout_handle = out_write;
  options.stderr_handle = out_write;
  options.stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
  options.inherit_handles = true;

  if (!SwitchToUSKeyboardLayout())
    VLOG(0) << "Can not set to US keyboard layout - Some keycodes may be"
        "interpreted incorrectly";
#endif

#if defined(OS_WIN)
  std::string command_string = base::WideToUTF8(command.GetCommandLineString());
#else
  std::string command_string = command.GetCommandLineString();
#endif
  VLOG(0) << "Launching chrome: " << command_string;
  base::Process process = base::LaunchProcess(command, options);
  if (!process.IsValid())
    return Status(kUnknownError, "chrome failed to start");

  scoped_ptr<DevToolsHttpClient> devtools_http_client;
  status = WaitForDevToolsAndCheckVersion(
      NetAddress(port), context_getter, socket_factory, &capabilities,
      &devtools_http_client);

  if (status.IsError()) {
    int exit_code;
    base::TerminationStatus chrome_status =
        base::GetTerminationStatus(process.Handle(), &exit_code);
    if (chrome_status != base::TERMINATION_STATUS_STILL_RUNNING) {
      std::string termination_reason;
      switch (chrome_status) {
        case base::TERMINATION_STATUS_NORMAL_TERMINATION:
          termination_reason = "exited normally";
          break;
        case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
          termination_reason = "exited abnormally";
          break;
        case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
          termination_reason = "was killed";
          break;
        case base::TERMINATION_STATUS_PROCESS_CRASHED:
          termination_reason = "crashed";
          break;
        default:
          termination_reason = "unknown";
          break;
      }
      return Status(kUnknownError,
                    "Chrome failed to start: " + termination_reason);
    }
    if (!process.Terminate(0, true)) {
      int exit_code;
      if (base::GetTerminationStatus(process.Handle(), &exit_code) ==
          base::TERMINATION_STATUS_STILL_RUNNING)
        return Status(kUnknownError, "cannot kill Chrome", status);
    }
    return status;
  }

  scoped_ptr<DevToolsClient> devtools_websocket_client;
  status = CreateBrowserwideDevToolsClientAndConnect(
      NetAddress(port), capabilities.perf_logging_prefs, socket_factory,
      *devtools_event_listeners, &devtools_websocket_client);
  if (status.IsError()) {
    LOG(WARNING) << "Browser-wide DevTools client failed to connect: "
                 << status.message();
  }

  scoped_ptr<ChromeDesktopImpl> chrome_desktop(
      new ChromeDesktopImpl(devtools_http_client.Pass(),
                            devtools_websocket_client.Pass(),
                            *devtools_event_listeners,
                            port_reservation.Pass(),
                            process.Pass(),
                            command,
                            &user_data_dir,
                            &extension_dir));
  for (size_t i = 0; i < extension_bg_pages.size(); ++i) {
    VLOG(0) << "Waiting for extension bg page load: " << extension_bg_pages[i];
    scoped_ptr<WebView> web_view;
    Status status = chrome_desktop->WaitForPageToLoad(
        extension_bg_pages[i], base::TimeDelta::FromSeconds(10), &web_view);
    if (status.IsError()) {
      return Status(kUnknownError,
                    "failed to wait for extension background page to load: " +
                        extension_bg_pages[i],
                    status);
    }
  }
  *chrome = chrome_desktop.Pass();
  return Status(kOk);
}

Status LaunchAndroidChrome(
    URLRequestContextGetter* context_getter,
    uint16 port,
    scoped_ptr<PortReservation> port_reservation,
    const SyncWebSocketFactory& socket_factory,
    const Capabilities& capabilities,
    ScopedVector<DevToolsEventListener>* devtools_event_listeners,
    DeviceManager* device_manager,
    scoped_ptr<Chrome>* chrome) {
  Status status(kOk);
  scoped_ptr<Device> device;
  if (capabilities.android_device_serial.empty()) {
    status = device_manager->AcquireDevice(&device);
  } else {
    status = device_manager->AcquireSpecificDevice(
        capabilities.android_device_serial, &device);
  }
  if (status.IsError())
    return status;

  Switches switches(capabilities.switches);
  for (size_t i = 0; i < arraysize(kCommonSwitches); ++i)
    switches.SetSwitch(kCommonSwitches[i]);
  switches.SetSwitch("disable-fre");
  switches.SetSwitch("enable-remote-debugging");
  status = device->SetUp(capabilities.android_package,
                         capabilities.android_activity,
                         capabilities.android_process,
                         switches.ToString(),
                         capabilities.android_use_running_app,
                         port);
  if (status.IsError()) {
    device->TearDown();
    return status;
  }

  scoped_ptr<DevToolsHttpClient> devtools_http_client;
  status = WaitForDevToolsAndCheckVersion(NetAddress(port),
                                          context_getter,
                                          socket_factory,
                                          &capabilities,
                                          &devtools_http_client);
  if (status.IsError()) {
    device->TearDown();
    return status;
  }

  scoped_ptr<DevToolsClient> devtools_websocket_client;
  status = CreateBrowserwideDevToolsClientAndConnect(
      NetAddress(port), capabilities.perf_logging_prefs, socket_factory,
      *devtools_event_listeners, &devtools_websocket_client);
  if (status.IsError()) {
    LOG(WARNING) << "Browser-wide DevTools client failed to connect: "
                 << status.message();
  }

  chrome->reset(new ChromeAndroidImpl(devtools_http_client.Pass(),
                                      devtools_websocket_client.Pass(),
                                      *devtools_event_listeners,
                                      port_reservation.Pass(),
                                      device.Pass()));
  return Status(kOk);
}

}  // namespace

Status LaunchChrome(
    URLRequestContextGetter* context_getter,
    const SyncWebSocketFactory& socket_factory,
    DeviceManager* device_manager,
    PortServer* port_server,
    PortManager* port_manager,
    const Capabilities& capabilities,
    ScopedVector<DevToolsEventListener>* devtools_event_listeners,
    scoped_ptr<Chrome>* chrome) {
  if (capabilities.IsRemoteBrowser()) {
    return LaunchRemoteChromeSession(
        context_getter, socket_factory,
        capabilities, devtools_event_listeners, chrome);
  }

  uint16 port = 0;
  scoped_ptr<PortReservation> port_reservation;
  Status port_status(kOk);

  if (capabilities.IsAndroid()) {
    if (port_server)
      port_status = port_server->ReservePort(&port, &port_reservation);
    else
      port_status = port_manager->ReservePortFromPool(&port, &port_reservation);
    if (port_status.IsError())
      return Status(kUnknownError, "cannot reserve port for Chrome",
                    port_status);
    return LaunchAndroidChrome(context_getter,
                               port,
                               port_reservation.Pass(),
                               socket_factory,
                               capabilities,
                               devtools_event_listeners,
                               device_manager,
                               chrome);
  } else {
    if (port_server)
      port_status = port_server->ReservePort(&port, &port_reservation);
    else
      port_status = port_manager->ReservePort(&port, &port_reservation);
    if (port_status.IsError())
      return Status(kUnknownError, "cannot reserve port for Chrome",
                    port_status);
    return LaunchDesktopChrome(context_getter,
                               port,
                               port_reservation.Pass(),
                               socket_factory,
                               capabilities,
                               devtools_event_listeners,
                               chrome);
  }
}

namespace internal {

void ConvertHexadecimalToIDAlphabet(std::string* id) {
  for (size_t i = 0; i < id->size(); ++i) {
    int val;
    if (base::HexStringToInt(base::StringPiece(id->begin() + i,
                                               id->begin() + i + 1),
                             &val)) {
      (*id)[i] = val + 'a';
    } else {
      (*id)[i] = 'a';
    }
  }
}

std::string GenerateExtensionId(const std::string& input) {
  uint8 hash[16];
  crypto::SHA256HashString(input, hash, sizeof(hash));
  std::string output =
      base::StringToLowerASCII(base::HexEncode(hash, sizeof(hash)));
  ConvertHexadecimalToIDAlphabet(&output);
  return output;
}

Status GetExtensionBackgroundPage(const base::DictionaryValue* manifest,
                                  const std::string& id,
                                  std::string* bg_page) {
  std::string bg_page_name;
  bool persistent = true;
  manifest->GetBoolean("background.persistent", &persistent);
  const base::Value* unused_value;
  if (manifest->Get("background.scripts", &unused_value))
    bg_page_name = "_generated_background_page.html";
  manifest->GetString("background.page", &bg_page_name);
  manifest->GetString("background_page", &bg_page_name);
  if (bg_page_name.empty() || !persistent)
    return Status(kOk);
  *bg_page = "chrome-extension://" + id + "/" + bg_page_name;
  return Status(kOk);
}

Status ProcessExtension(const std::string& extension,
                        const base::FilePath& temp_dir,
                        base::FilePath* path,
                        std::string* bg_page) {
  // Decodes extension string.
  // Some WebDriver client base64 encoders follow RFC 1521, which require that
  // 'encoded lines be no more than 76 characters long'. Just remove any
  // newlines.
  std::string extension_base64;
  base::RemoveChars(extension, "\n", &extension_base64);
  std::string decoded_extension;
  if (!base::Base64Decode(extension_base64, &decoded_extension))
    return Status(kUnknownError, "cannot base64 decode");

  // If the file is a crx file, extract the extension's ID from its public key.
  // Otherwise generate a random public key and use its derived extension ID.
  std::string public_key;
  std::string magic_header = decoded_extension.substr(0, 4);
  if (magic_header.size() != 4)
    return Status(kUnknownError, "cannot extract magic number");

  const bool is_crx_file = magic_header == "Cr24";

  if (is_crx_file) {
    // Assume a CRX v2 file - see https://developer.chrome.com/extensions/crx.
    std::string key_len_str = decoded_extension.substr(8, 4);
    if (key_len_str.size() != 4)
      return Status(kUnknownError, "cannot extract public key length");
    uint32 key_len = *reinterpret_cast<const uint32*>(key_len_str.c_str());
    public_key = decoded_extension.substr(16, key_len);
    if (key_len != public_key.size())
      return Status(kUnknownError, "invalid public key length");
  } else {
    // Not a CRX file. Generate RSA keypair to get a valid extension id.
    scoped_ptr<crypto::RSAPrivateKey> key_pair(
        crypto::RSAPrivateKey::Create(2048));
    if (!key_pair)
      return Status(kUnknownError, "cannot generate RSA key pair");
    std::vector<uint8> public_key_vector;
    if (!key_pair->ExportPublicKey(&public_key_vector))
      return Status(kUnknownError, "cannot extract public key");
    public_key =
        std::string(reinterpret_cast<char*>(&public_key_vector.front()),
                    public_key_vector.size());
  }
  std::string public_key_base64;
  base::Base64Encode(public_key, &public_key_base64);
  std::string id = GenerateExtensionId(public_key);

  // Unzip the crx file.
  base::ScopedTempDir temp_crx_dir;
  if (!temp_crx_dir.CreateUniqueTempDir())
    return Status(kUnknownError, "cannot create temp dir");
  base::FilePath extension_crx = temp_crx_dir.path().AppendASCII("temp.crx");
  int size = static_cast<int>(decoded_extension.length());
  if (base::WriteFile(extension_crx, decoded_extension.c_str(), size) !=
      size) {
    return Status(kUnknownError, "cannot write file");
  }
  base::FilePath extension_dir = temp_dir.AppendASCII("extension_" + id);
  if (!zip::Unzip(extension_crx, extension_dir))
    return Status(kUnknownError, "cannot unzip");

  // Parse the manifest and set the 'key' if not already present.
  base::FilePath manifest_path(extension_dir.AppendASCII("manifest.json"));
  std::string manifest_data;
  if (!base::ReadFileToString(manifest_path, &manifest_data))
    return Status(kUnknownError, "cannot read manifest");
  scoped_ptr<base::Value> manifest_value =
      base::JSONReader::Read(manifest_data);
  base::DictionaryValue* manifest;
  if (!manifest_value || !manifest_value->GetAsDictionary(&manifest))
    return Status(kUnknownError, "invalid manifest");

  std::string manifest_key_base64;
  if (manifest->GetString("key", &manifest_key_base64)) {
    // If there is a key in both the header and the manifest, use the key in the
    // manifest. This allows chromedriver users users who generate dummy crxs
    // to set the manifest key and have a consistent ID.
    std::string manifest_key;
    if (!base::Base64Decode(manifest_key_base64, &manifest_key))
      return Status(kUnknownError, "'key' in manifest is not base64 encoded");
    std::string manifest_id = GenerateExtensionId(manifest_key);
    if (id != manifest_id) {
      if (is_crx_file) {
        LOG(WARNING)
            << "Public key in crx header is different from key in manifest"
            << std::endl << "key from header:   " << public_key_base64
            << std::endl << "key from manifest: " << manifest_key_base64
            << std::endl << "generated extension id from header key:   " << id
            << std::endl << "generated extension id from manifest key: "
            << manifest_id;
      }
      id = manifest_id;
    }
  } else {
    manifest->SetString("key", public_key_base64);
    base::JSONWriter::Write(*manifest, &manifest_data);
    if (base::WriteFile(
            manifest_path, manifest_data.c_str(), manifest_data.size()) !=
        static_cast<int>(manifest_data.size())) {
      return Status(kUnknownError, "cannot add 'key' to manifest");
    }
  }

  // Get extension's background page URL, if there is one.
  std::string bg_page_tmp;
  Status status = GetExtensionBackgroundPage(manifest, id, &bg_page_tmp);
  if (status.IsError())
    return status;

  *path = extension_dir;
  if (bg_page_tmp.size())
    *bg_page = bg_page_tmp;
  return Status(kOk);
}

void UpdateExtensionSwitch(Switches* switches,
                           const char name[],
                           const base::FilePath::StringType& extension) {
  base::FilePath::StringType value = switches->GetSwitchValueNative(name);
  if (value.length())
    value += FILE_PATH_LITERAL(",");
  value += extension;
  switches->SetSwitch(name, value);
}

Status ProcessExtensions(const std::vector<std::string>& extensions,
                         const base::FilePath& temp_dir,
                         bool include_automation_extension,
                         Switches* switches,
                         std::vector<std::string>* bg_pages) {
  std::vector<std::string> bg_pages_tmp;
  std::vector<base::FilePath::StringType> extension_paths;
  for (size_t i = 0; i < extensions.size(); ++i) {
    base::FilePath path;
    std::string bg_page;
    Status status = ProcessExtension(extensions[i], temp_dir, &path, &bg_page);
    if (status.IsError()) {
      return Status(
          kUnknownError,
          base::StringPrintf("cannot process extension #%" PRIuS, i + 1),
          status);
    }
    extension_paths.push_back(path.value());
    if (bg_page.length())
      bg_pages_tmp.push_back(bg_page);
  }

  if (include_automation_extension) {
    base::FilePath automation_extension;
    Status status = UnpackAutomationExtension(temp_dir, &automation_extension);
    if (status.IsError())
      return status;
    if (switches->HasSwitch("disable-extensions")) {
      UpdateExtensionSwitch(switches, "load-component-extension",
                            automation_extension.value());
    } else {
      extension_paths.push_back(automation_extension.value());
    }
  }

  if (extension_paths.size()) {
    base::FilePath::StringType extension_paths_value = JoinString(
        extension_paths, FILE_PATH_LITERAL(','));
    UpdateExtensionSwitch(switches, "load-extension", extension_paths_value);
  }
  bg_pages->swap(bg_pages_tmp);
  return Status(kOk);
}

Status WritePrefsFile(
    const std::string& template_string,
    const base::DictionaryValue* custom_prefs,
    const base::FilePath& path) {
  int code;
  std::string error_msg;
  scoped_ptr<base::Value> template_value(
      base::JSONReader::DeprecatedReadAndReturnError(template_string, 0, &code,
                                                     &error_msg));
  base::DictionaryValue* prefs;
  if (!template_value || !template_value->GetAsDictionary(&prefs)) {
    return Status(kUnknownError,
                  "cannot parse internal JSON template: " + error_msg);
  }

  if (custom_prefs) {
    for (base::DictionaryValue::Iterator it(*custom_prefs); !it.IsAtEnd();
         it.Advance()) {
      prefs->Set(it.key(), it.value().DeepCopy());
    }
  }

  std::string prefs_str;
  base::JSONWriter::Write(*prefs, &prefs_str);
  VLOG(0) << "Populating " << path.BaseName().value()
          << " file: " << PrettyPrintValue(*prefs);
  if (static_cast<int>(prefs_str.length()) != base::WriteFile(
          path, prefs_str.c_str(), prefs_str.length())) {
    return Status(kUnknownError, "failed to write prefs file");
  }
  return Status(kOk);
}

Status PrepareUserDataDir(
    const base::FilePath& user_data_dir,
    const base::DictionaryValue* custom_prefs,
    const base::DictionaryValue* custom_local_state) {
  base::FilePath default_dir =
      user_data_dir.AppendASCII(chrome::kInitialProfile);
  if (!base::CreateDirectory(default_dir))
    return Status(kUnknownError, "cannot create default profile directory");

  std::string preferences;
  base::FilePath preferences_path =
      default_dir.Append(chrome::kPreferencesFilename);

  if (base::PathExists(preferences_path))
    base::ReadFileToString(preferences_path, &preferences);
  else
    preferences = kPreferences;

  Status status =
      WritePrefsFile(preferences,
                     custom_prefs,
                     default_dir.Append(chrome::kPreferencesFilename));
  if (status.IsError())
    return status;

  std::string local_state;
  base::FilePath local_state_path =
      user_data_dir.Append(chrome::kLocalStateFilename);

  if (base::PathExists(local_state_path))
    base::ReadFileToString(local_state_path, &local_state);
  else
    local_state = kLocalState;

  status = WritePrefsFile(local_state,
                          custom_local_state,
                          user_data_dir.Append(chrome::kLocalStateFilename));
  if (status.IsError())
    return status;

  // Write empty "First Run" file, otherwise Chrome will wipe the default
  // profile that was written.
  if (base::WriteFile(
          user_data_dir.Append(chrome::kFirstRunSentinel), "", 0) != 0) {
    return Status(kUnknownError, "failed to write first run file");
  }
  return Status(kOk);
}

}  // namespace internal
