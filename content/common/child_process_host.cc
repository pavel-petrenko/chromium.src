// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/child_process_host.h"

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/third_party/dynamic_annotations/dynamic_annotations.h"
#include "content/common/child_process_info.h"
#include "content/common/child_process_messages.h"
#include "content/common/content_paths.h"
#include "content/common/content_switches.h"
#include "ipc/ipc_logging.h"

#if defined(OS_LINUX)
#include "base/linux_util.h"
#endif  // OS_LINUX

#if defined(OS_MACOSX)
namespace {

// Given |path| identifying a Mac-style child process executable path, adjusts
// it to correspond to |feature|. For a child process path such as
// ".../Chromium Helper.app/Contents/MacOS/Chromium Helper", the transformed
// path for feature "NP" would be
// ".../Chromium Helper NP.app/Contents/MacOS/Chromium Helper NP". The new
// path is returned.
FilePath TransformPathForFeature(const FilePath& path,
                                 const std::string& feature) {
  std::string basename = path.BaseName().value();

  FilePath macos_path = path.DirName();
  const char kMacOSName[] = "MacOS";
  DCHECK_EQ(kMacOSName, macos_path.BaseName().value());

  FilePath contents_path = macos_path.DirName();
  const char kContentsName[] = "Contents";
  DCHECK_EQ(kContentsName, contents_path.BaseName().value());

  FilePath helper_app_path = contents_path.DirName();
  const char kAppExtension[] = ".app";
  std::string basename_app = basename;
  basename_app.append(kAppExtension);
  DCHECK_EQ(basename_app, helper_app_path.BaseName().value());

  FilePath root_path = helper_app_path.DirName();

  std::string new_basename = basename;
  new_basename.append(1, ' ');
  new_basename.append(feature);
  std::string new_basename_app = new_basename;
  new_basename_app.append(kAppExtension);

  FilePath new_path = root_path.Append(new_basename_app)
                               .Append(kContentsName)
                               .Append(kMacOSName)
                               .Append(new_basename);

  return new_path;
}

}  // namespace
#endif  // OS_MACOSX

ChildProcessHost::ChildProcessHost()
    : ALLOW_THIS_IN_INITIALIZER_LIST(listener_(this)),
      opening_channel_(false) {
}

ChildProcessHost::~ChildProcessHost() {
  for (size_t i = 0; i < filters_.size(); ++i) {
    filters_[i]->OnChannelClosing();
    filters_[i]->OnFilterRemoved();
  }
  listener_.Shutdown();
}

void ChildProcessHost::AddFilter(IPC::ChannelProxy::MessageFilter* filter) {
  filters_.push_back(filter);

  if (channel_.get())
    filter->OnFilterAdded(channel_.get());
}

// static
FilePath ChildProcessHost::GetChildPath(int flags) {
  FilePath child_path;

  child_path = CommandLine::ForCurrentProcess()->GetSwitchValuePath(
      switches::kBrowserSubprocessPath);

#if defined(OS_LINUX)
  // Use /proc/self/exe rather than our known binary path so updates
  // can't swap out the binary from underneath us.
  // When running under Valgrind, forking /proc/self/exe ends up forking the
  // Valgrind executable, which then crashes. However, it's almost safe to
  // assume that the updates won't happen while testing with Valgrind tools.
  if (child_path.empty() && flags & CHILD_ALLOW_SELF && !RunningOnValgrind())
    child_path = FilePath("/proc/self/exe");
#endif

  // On most platforms, the child executable is the same as the current
  // executable.
  if (child_path.empty())
    PathService::Get(content::CHILD_PROCESS_EXE, &child_path);

#if defined(OS_MACOSX)
  DCHECK(!(flags & CHILD_NO_PIE && flags & CHILD_ALLOW_HEAP_EXECUTION));

  // If needed, choose an executable with special flags set that inform the
  // kernel to enable or disable specific optional process-wide features.
  if (flags & CHILD_NO_PIE) {
    // "NP" is "No PIE". This results in Chromium Helper NP.app or
    // Google Chrome Helper NP.app.
    child_path = TransformPathForFeature(child_path, "NP");
  } else if (flags & CHILD_ALLOW_HEAP_EXECUTION) {
    // "EH" is "Executable Heap". A non-executable heap is only available to
    // 32-bit processes on Mac OS X 10.7. Most code can and should run with a
    // non-executable heap, but the "EH" feature is provided to allow code
    // intolerant of a non-executable heap to work properly on 10.7. This
    // results in Chromium Helper EH.app or Google Chrome Helper EH.app.
    child_path = TransformPathForFeature(child_path, "EH");
  }
#endif

  return child_path;
}

#if defined(OS_WIN)
// static
void ChildProcessHost::PreCacheFont(LOGFONT font) {
  // If a child process is running in a sandbox, GetTextMetrics()
  // can sometimes fail. If a font has not been loaded
  // previously, GetTextMetrics() will try to load the font
  // from the font file. However, the sandboxed process does
  // not have permissions to access any font files and
  // the call fails. So we make the browser pre-load the
  // font for us by using a dummy call to GetTextMetrics of
  // the same font.

  // Maintain a circular queue for the fonts and DCs to be cached.
  // font_index maintains next available location in the queue.
  static const int kFontCacheSize = 32;
  static HFONT fonts[kFontCacheSize] = {0};
  static HDC hdcs[kFontCacheSize] = {0};
  static size_t font_index = 0;

  UMA_HISTOGRAM_COUNTS_100("Memory.CachedFontAndDC",
      fonts[kFontCacheSize-1] ? kFontCacheSize : static_cast<int>(font_index));

  HDC hdc = GetDC(NULL);
  HFONT font_handle = CreateFontIndirect(&font);
  DCHECK(NULL != font_handle);

  HGDIOBJ old_font = SelectObject(hdc, font_handle);
  DCHECK(NULL != old_font);

  TEXTMETRIC tm;
  BOOL ret = GetTextMetrics(hdc, &tm);
  DCHECK(ret);

  if (fonts[font_index] || hdcs[font_index]) {
    // We already have too many fonts, we will delete one and take it's place.
    DeleteObject(fonts[font_index]);
    ReleaseDC(NULL, hdcs[font_index]);
  }

  fonts[font_index] = font_handle;
  hdcs[font_index] = hdc;
  font_index = (font_index + 1) % kFontCacheSize;
}
#endif  // OS_WIN


bool ChildProcessHost::CreateChannel() {
  channel_id_ = ChildProcessInfo::GenerateRandomChannelID(this);
  channel_.reset(new IPC::Channel(
      channel_id_, IPC::Channel::MODE_SERVER, &listener_));
  if (!channel_->Connect())
    return false;

  for (size_t i = 0; i < filters_.size(); ++i)
    filters_[i]->OnFilterAdded(channel_.get());

  // Make sure these messages get sent first.
#if defined(IPC_MESSAGE_LOG_ENABLED)
  bool enabled = IPC::Logging::GetInstance()->Enabled();
  Send(new ChildProcessMsg_SetIPCLoggingEnabled(enabled));
#endif

  Send(new ChildProcessMsg_AskBeforeShutdown());

  opening_channel_ = true;

  return true;
}

void ChildProcessHost::InstanceCreated() {
  Notify(content::NOTIFICATION_CHILD_INSTANCE_CREATED);
}

bool ChildProcessHost::OnMessageReceived(const IPC::Message& msg) {
  return false;
}

void ChildProcessHost::OnChannelConnected(int32 peer_pid) {
}

void ChildProcessHost::OnChannelError() {
}

bool ChildProcessHost::Send(IPC::Message* message) {
  if (!channel_.get()) {
    delete message;
    return false;
  }
  return channel_->Send(message);
}

void ChildProcessHost::OnChildDied() {
  delete this;
}

void ChildProcessHost::ShutdownStarted() {
}

void ChildProcessHost::Notify(int type) {
}

ChildProcessHost::ListenerHook::ListenerHook(ChildProcessHost* host)
    : host_(host) {
}

void ChildProcessHost::ListenerHook::Shutdown() {
  host_ = NULL;
}

bool ChildProcessHost::ListenerHook::OnMessageReceived(
    const IPC::Message& msg) {
  if (!host_)
    return true;

#ifdef IPC_MESSAGE_LOG_ENABLED
  IPC::Logging* logger = IPC::Logging::GetInstance();
  if (msg.type() == IPC_LOGGING_ID) {
    logger->OnReceivedLoggingMessage(msg);
    return true;
  }

  if (logger->Enabled())
    logger->OnPreDispatchMessage(msg);
#endif

  bool handled = false;
  for (size_t i = 0; i < host_->filters_.size(); ++i) {
    if (host_->filters_[i]->OnMessageReceived(msg)) {
      handled = true;
      break;
    }
  }

  if (!handled && msg.type() == ChildProcessHostMsg_ShutdownRequest::ID) {
    if (host_->CanShutdown())
      host_->Send(new ChildProcessMsg_Shutdown());
    handled = true;
  }

  if (!handled)
    handled = host_->OnMessageReceived(msg);

#ifdef IPC_MESSAGE_LOG_ENABLED
  if (logger->Enabled())
    logger->OnPostDispatchMessage(msg, host_->channel_id_);
#endif
  return handled;
}

void ChildProcessHost::ListenerHook::OnChannelConnected(int32 peer_pid) {
  if (!host_)
    return;
  host_->opening_channel_ = false;
  host_->OnChannelConnected(peer_pid);
  // Notify in the main loop of the connection.
  host_->Notify(content::NOTIFICATION_CHILD_PROCESS_HOST_CONNECTED);

  for (size_t i = 0; i < host_->filters_.size(); ++i)
    host_->filters_[i]->OnChannelConnected(peer_pid);
}

void ChildProcessHost::ListenerHook::OnChannelError() {
  if (!host_)
    return;
  host_->opening_channel_ = false;
  host_->OnChannelError();

  for (size_t i = 0; i < host_->filters_.size(); ++i)
    host_->filters_[i]->OnChannelError();

  // This will delete host_, which will also destroy this!
  host_->OnChildDied();
}

void ChildProcessHost::ForceShutdown() {
  Send(new ChildProcessMsg_Shutdown());
}
