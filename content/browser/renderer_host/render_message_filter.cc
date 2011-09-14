// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_message_filter.h"

#include <map>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/sys_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/worker_pool.h"
#include "base/utf_string_conversions.h"
#include "content/browser/browser_context.h"
#include "content/browser/browser_thread.h"
#include "content/browser/child_process_security_policy.h"
#include "content/browser/content_browser_client.h"
#include "content/browser/download/download_stats.h"
#include "content/browser/download/download_types.h"
#include "content/browser/plugin_process_host.h"
#include "content/browser/plugin_service.h"
#include "content/browser/ppapi_plugin_process_host.h"
#include "content/browser/ppapi_broker_process_host.h"
#include "content/browser/renderer_host/browser_render_process_host.h"
#include "content/browser/renderer_host/media/media_observer.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/renderer_host/render_widget_helper.h"
#include "content/browser/resource_context.h"
#include "content/browser/user_metrics.h"
#include "content/common/content_switches.h"
#include "content/common/desktop_notification_messages.h"
#include "content/common/notification_service.h"
#include "content/common/url_constants.h"
#include "content/common/view_messages.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_platform_file.h"
#include "media/audio/audio_util.h"
#include "media/base/media_log_event.h"
#include "net/base/cookie_monster.h"
#include "net/base/host_resolver_impl.h"
#include "net/base/io_buffer.h"
#include "net/base/keygen_handler.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_layer.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebNotificationPresenter.h"
#include "webkit/glue/context_menu.h"
#include "webkit/glue/webcookie.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/plugins/npapi/plugin_group.h"
#include "webkit/plugins/npapi/plugin_list.h"
#include "webkit/plugins/npapi/webplugin.h"
#include "webkit/plugins/webplugininfo.h"

#if defined(OS_MACOSX)
#include "content/common/mac/font_descriptor.h"
#include "content/common/mac/font_loader.h"
#endif
#if defined(OS_POSIX)
#include "base/file_descriptor_posix.h"
#endif
#if defined(OS_WIN)
#include "content/common/child_process_host.h"
#endif

using net::CookieStore;
using content::PluginServiceFilter;

namespace {

const int kPluginsRefreshThresholdInSeconds = 3;

// Common functionality for converting a sync renderer message to a callback
// function in the browser. Derive from this, create it on the heap when
// issuing your callback. When done, write your reply parameters into
// reply_msg(), and then call SendReplyAndDeleteThis().
class RenderMessageCompletionCallback {
 public:
  RenderMessageCompletionCallback(RenderMessageFilter* filter,
                                  IPC::Message* reply_msg)
      : filter_(filter),
        reply_msg_(reply_msg) {
  }

  virtual ~RenderMessageCompletionCallback() {
  }

  RenderMessageFilter* filter() { return filter_.get(); }
  IPC::Message* reply_msg() { return reply_msg_; }

  void SendReplyAndDeleteThis() {
    filter_->Send(reply_msg_);
    delete this;
  }

 private:
  scoped_refptr<RenderMessageFilter> filter_;
  IPC::Message* reply_msg_;
};

class ClearCacheCompletion : public RenderMessageCompletionCallback,
                             public net::CompletionCallback {
 public:
  ClearCacheCompletion(RenderMessageFilter* filter,
                       IPC::Message* reply_msg)
      : RenderMessageCompletionCallback(filter, reply_msg) {
  }

  virtual void RunWithParams(const Tuple1<int>& params) {
    ViewHostMsg_ClearCache::WriteReplyParams(reply_msg(), params.a);
    SendReplyAndDeleteThis();
  }
};

class OpenChannelToPpapiPluginCallback : public RenderMessageCompletionCallback,
                                         public PpapiPluginProcessHost::Client {
 public:
  OpenChannelToPpapiPluginCallback(RenderMessageFilter* filter,
                                   const content::ResourceContext* context,
                                   IPC::Message* reply_msg)
      : RenderMessageCompletionCallback(filter, reply_msg),
        context_(context) {
  }

  virtual void GetChannelInfo(base::ProcessHandle* renderer_handle,
                              int* renderer_id) {
    *renderer_handle = filter()->peer_handle();
    *renderer_id = filter()->render_process_id();
  }

  virtual void OnChannelOpened(base::ProcessHandle plugin_process_handle,
                               const IPC::ChannelHandle& channel_handle) {
    ViewHostMsg_OpenChannelToPepperPlugin::WriteReplyParams(
        reply_msg(), plugin_process_handle, channel_handle);
    SendReplyAndDeleteThis();
  }

  virtual const content::ResourceContext* GetResourceContext() {
    return context_;
  }

 private:
  const content::ResourceContext* context_;
};

class OpenChannelToPpapiBrokerCallback : public PpapiBrokerProcessHost::Client {
 public:
  OpenChannelToPpapiBrokerCallback(RenderMessageFilter* filter,
                                   int routing_id,
                                   int request_id)
      : filter_(filter),
        routing_id_(routing_id),
        request_id_(request_id) {
  }

  virtual ~OpenChannelToPpapiBrokerCallback() {}

  virtual void GetChannelInfo(base::ProcessHandle* renderer_handle,
                              int* renderer_id) {
    *renderer_handle = filter_->peer_handle();
    *renderer_id = filter_->render_process_id();
  }

  virtual void OnChannelOpened(base::ProcessHandle broker_process_handle,
                               const IPC::ChannelHandle& channel_handle) {
    filter_->Send(new ViewMsg_PpapiBrokerChannelCreated(routing_id_,
                                                        request_id_,
                                                        broker_process_handle,
                                                        channel_handle));
    delete this;
  }

 private:
  scoped_refptr<RenderMessageFilter> filter_;
  int routing_id_;
  int request_id_;
};

// Class to assist with clearing out the cache when we want to preserve
// the sslhostinfo entries.  It's not very efficient, but its just for debug.
class DoomEntriesHelper {
 public:
  explicit DoomEntriesHelper(disk_cache::Backend* backend)
      : backend_(backend),
        entry_(NULL),
        iter_(NULL),
        ALLOW_THIS_IN_INITIALIZER_LIST(callback_(this,
            &DoomEntriesHelper::CacheCallback)),
        user_callback_(NULL) {
  }

  void ClearCache(ClearCacheCompletion* callback) {
    user_callback_ = callback;
    return CacheCallback(net::OK);  // Start clearing the cache.
  }

 private:
  void CacheCallback(int result) {
    do {
      if (result != net::OK) {
        user_callback_->RunWithParams(Tuple1<int>(result));
        delete this;
        return;
      }

      if (entry_) {
        // Doom all entries except those with snapstart information.
        std::string key = entry_->GetKey();
        if (key.find("sslhostinfo:") != 0) {
          entry_->Doom();
          backend_->EndEnumeration(&iter_);
          iter_ = NULL;  // We invalidated our iterator - start from the top!
        }
        entry_->Close();
        entry_ = NULL;
      }
      result = backend_->OpenNextEntry(&iter_, &entry_, &callback_);
    } while (result != net::ERR_IO_PENDING);
  }

  disk_cache::Backend* backend_;
  disk_cache::Entry* entry_;
  void* iter_;
  net::CompletionCallbackImpl<DoomEntriesHelper> callback_;
  ClearCacheCompletion* user_callback_;
};

}  // namespace

class RenderMessageFilter::OpenChannelToNpapiPluginCallback
    : public RenderMessageCompletionCallback,
      public PluginProcessHost::Client {
 public:
  OpenChannelToNpapiPluginCallback(RenderMessageFilter* filter,
                                   const content::ResourceContext& context,
                                   IPC::Message* reply_msg)
      : RenderMessageCompletionCallback(filter, reply_msg),
        context_(context),
        host_(NULL),
        sent_plugin_channel_request_(false) {
  }

  virtual int ID() OVERRIDE {
    return filter()->render_process_id();
  }

  virtual const content::ResourceContext& GetResourceContext() OVERRIDE {
    return context_;
  }

  virtual bool OffTheRecord() OVERRIDE {
    return filter()->OffTheRecord();
  }

  virtual void SetPluginInfo(const webkit::WebPluginInfo& info) OVERRIDE {
    info_ = info;
  }

  virtual void OnFoundPluginProcessHost(PluginProcessHost* host) OVERRIDE {
    DCHECK(host);
    host_ = host;
  }

  virtual void OnSentPluginChannelRequest() OVERRIDE {
    sent_plugin_channel_request_ = true;
  }

  virtual void OnChannelOpened(const IPC::ChannelHandle& handle) OVERRIDE {
    WriteReplyAndDeleteThis(handle);
  }

  virtual void OnError() OVERRIDE {
    WriteReplyAndDeleteThis(IPC::ChannelHandle());
  }

  PluginProcessHost* host() const {
    return host_;
  }

  bool sent_plugin_channel_request() const {
    return sent_plugin_channel_request_;
  }

  void Cancel() {
    delete this;
  }

 private:
  void WriteReplyAndDeleteThis(const IPC::ChannelHandle& handle) {
    ViewHostMsg_OpenChannelToPlugin::WriteReplyParams(reply_msg(),
                                                      handle, info_);
    filter()->OnCompletedOpenChannelToNpapiPlugin(this);
    SendReplyAndDeleteThis();
  }

  const content::ResourceContext& context_;
  webkit::WebPluginInfo info_;
  PluginProcessHost* host_;
  bool sent_plugin_channel_request_;
};

RenderMessageFilter::RenderMessageFilter(
    int render_process_id,
    PluginService* plugin_service,
    content::BrowserContext* browser_context,
    net::URLRequestContextGetter* request_context,
    RenderWidgetHelper* render_widget_helper)
    : resource_dispatcher_host_(
          content::GetContentClient()->browser()->GetResourceDispatcherHost()),
      plugin_service_(plugin_service),
      browser_context_(browser_context),
      request_context_(request_context),
      resource_context_(browser_context->GetResourceContext()),
      render_widget_helper_(render_widget_helper),
      incognito_(browser_context->IsOffTheRecord()),
      webkit_context_(browser_context->GetWebKitContext()),
      render_process_id_(render_process_id) {
  DCHECK(request_context_);

  render_widget_helper_->Init(render_process_id_, resource_dispatcher_host_);
}

RenderMessageFilter::~RenderMessageFilter() {
  // This function should be called on the IO thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(plugin_host_clients_.empty());
}

void RenderMessageFilter::OnChannelClosing() {
  BrowserMessageFilter::OnChannelClosing();
  for (std::set<OpenChannelToNpapiPluginCallback*>::iterator it =
       plugin_host_clients_.begin(); it != plugin_host_clients_.end(); ++it) {
    OpenChannelToNpapiPluginCallback* client = *it;
    if (client->host()) {
      if (client->sent_plugin_channel_request()) {
        client->host()->CancelSentRequest(client);
      } else {
        client->host()->CancelPendingRequest(client);
      }
    } else {
      plugin_service_->CancelOpenChannelToNpapiPlugin(client);
    }
    client->Cancel();
  }
  plugin_host_clients_.clear();
}

void RenderMessageFilter::OverrideThreadForMessage(const IPC::Message& message,
                                                   BrowserThread::ID* thread) {
  switch (message.type()) {
    // Can't load plugins on IO thread.
    case ViewHostMsg_GetPlugins::ID:
    // The PluginService::GetPluginInfo may need to load the plugins.  Don't do
    // it on the IO thread.
    case ViewHostMsg_GetPluginInfo::ID:
      *thread = BrowserThread::FILE;
      break;
    default:
      break;
  }
}

bool RenderMessageFilter::OnMessageReceived(const IPC::Message& message,
                                            bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(RenderMessageFilter, message, *message_was_ok)
#if defined(OS_WIN)
    // On Windows, we handle these on the IO thread to avoid a deadlock with
    // plugins.  On non-Windows systems, we need to handle them on the UI
    // thread.
    IPC_MESSAGE_HANDLER(ViewHostMsg_GetScreenInfo, OnGetScreenInfo)
    IPC_MESSAGE_HANDLER(ViewHostMsg_GetWindowRect, OnGetWindowRect)
    IPC_MESSAGE_HANDLER(ViewHostMsg_GetRootWindowRect, OnGetRootWindowRect)

    // This hack is Windows-specific.
    IPC_MESSAGE_HANDLER(ViewHostMsg_PreCacheFont, OnPreCacheFont)
#endif

    IPC_MESSAGE_HANDLER(ViewHostMsg_GenerateRoutingID, OnGenerateRoutingID)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CreateWindow, OnMsgCreateWindow)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CreateWidget, OnMsgCreateWidget)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CreateFullscreenWidget,
                        OnMsgCreateFullscreenWidget)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SetCookie, OnSetCookie)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_GetCookies, OnGetCookies)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_GetRawCookies, OnGetRawCookies)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DeleteCookie, OnDeleteCookie)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CookiesEnabled, OnCookiesEnabled)
#if defined(OS_MACOSX)
    IPC_MESSAGE_HANDLER(ViewHostMsg_LoadFont, OnLoadFont)
#endif
    IPC_MESSAGE_HANDLER(ViewHostMsg_GetPlugins, OnGetPlugins)
    IPC_MESSAGE_HANDLER(ViewHostMsg_GetPluginInfo, OnGetPluginInfo)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DownloadUrl, OnDownloadUrl)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_OpenChannelToPlugin,
                                    OnOpenChannelToPlugin)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_OpenChannelToPepperPlugin,
                                    OnOpenChannelToPepperPlugin)
    IPC_MESSAGE_HANDLER(ViewHostMsg_OpenChannelToPpapiBroker,
                        OnOpenChannelToPpapiBroker)
    IPC_MESSAGE_HANDLER_GENERIC(ViewHostMsg_UpdateRect,
        render_widget_helper_->DidReceiveUpdateMsg(message))
    IPC_MESSAGE_HANDLER(DesktopNotificationHostMsg_CheckPermission,
                        OnCheckNotificationPermission)
    IPC_MESSAGE_HANDLER(ViewHostMsg_AllocateSharedMemoryBuffer,
                        OnAllocateSharedMemoryBuffer)
#if defined(OS_MACOSX)
    IPC_MESSAGE_HANDLER(ViewHostMsg_AllocTransportDIB, OnAllocTransportDIB)
    IPC_MESSAGE_HANDLER(ViewHostMsg_FreeTransportDIB, OnFreeTransportDIB)
#endif
    IPC_MESSAGE_HANDLER(ViewHostMsg_CloseCurrentConnections,
                        OnCloseCurrentConnections)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SetCacheMode, OnSetCacheMode)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_ClearCache, OnClearCache)
    IPC_MESSAGE_HANDLER(ViewHostMsg_ClearHostResolverCache,
                        OnClearHostResolverCache)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DidGenerateCacheableMetadata,
                        OnCacheableMetadataAvailable)
    IPC_MESSAGE_HANDLER(ViewHostMsg_EnableSpdy, OnEnableSpdy)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_Keygen, OnKeygen)
    IPC_MESSAGE_HANDLER(ViewHostMsg_AsyncOpenFile, OnAsyncOpenFile)
    IPC_MESSAGE_HANDLER(ViewHostMsg_GetHardwareSampleRate,
                        OnGetHardwareSampleRate)
    IPC_MESSAGE_HANDLER(ViewHostMsg_MediaLogEvent, OnMediaLogEvent)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()

  return handled;
}

void RenderMessageFilter::OnDestruct() const {
  BrowserThread::DeleteOnIOThread::Destruct(this);
}

bool RenderMessageFilter::OffTheRecord() const {
  return incognito_ ||
         !content::GetContentClient()->browser()->AllowSaveLocalState(
             resource_context_);
}

void RenderMessageFilter::OnMsgCreateWindow(
    const ViewHostMsg_CreateWindow_Params& params,
    int* route_id, int64* cloned_session_storage_namespace_id) {
  if (!content::GetContentClient()->browser()->CanCreateWindow(
          params.opener_url, params.window_container_type, resource_context_)) {
    *route_id = MSG_ROUTING_NONE;
    return;
  }

  *cloned_session_storage_namespace_id =
      webkit_context_->dom_storage_context()->CloneSessionStorage(
          params.session_storage_namespace_id);
  render_widget_helper_->CreateNewWindow(params,
                                         peer_handle(),
                                         route_id);
}

void RenderMessageFilter::OnMsgCreateWidget(int opener_id,
                                            WebKit::WebPopupType popup_type,
                                            int* route_id) {
  render_widget_helper_->CreateNewWidget(opener_id, popup_type, route_id);
}

void RenderMessageFilter::OnMsgCreateFullscreenWidget(int opener_id,
                                                      int* route_id) {
  render_widget_helper_->CreateNewFullscreenWidget(opener_id, route_id);
}

void RenderMessageFilter::OnSetCookie(const IPC::Message& message,
                                      const GURL& url,
                                      const GURL& first_party_for_cookies,
                                      const std::string& cookie) {
  net::CookieOptions options;
  if (content::GetContentClient()->browser()->AllowSetCookie(
          url, first_party_for_cookies, cookie,
          resource_context_, render_process_id_, message.routing_id(),
          &options)) {
    net::URLRequestContext* context = GetRequestContextForURL(url);
    // Pass a null callback since we don't care about when the 'set' completes.
    context->cookie_store()->SetCookieWithOptionsAsync(
        url, cookie, options, net::CookieMonster::SetCookiesCallback());
  }
}

void RenderMessageFilter::OnGetCookies(const GURL& url,
                                       const GURL& first_party_for_cookies,
                                       IPC::Message* reply_msg) {
  net::URLRequestContext* context = GetRequestContextForURL(url);
  net::CookieMonster* cookie_monster =
      context->cookie_store()->GetCookieMonster();
  cookie_monster->GetAllCookiesForURLAsync(
      url, base::Bind(&RenderMessageFilter::CheckPolicyForCookies, this, url,
                      first_party_for_cookies, reply_msg));
}

void RenderMessageFilter::OnGetRawCookies(
    const GURL& url,
    const GURL& first_party_for_cookies,
    IPC::Message* reply_msg) {
  // Only return raw cookies to trusted renderers or if this request is
  // not targeted to an an external host like ChromeFrame.
  // TODO(ananta) We need to support retreiving raw cookies from external
  // hosts.
  if (!ChildProcessSecurityPolicy::GetInstance()->CanReadRawCookies(
          render_process_id_)) {
    SendGetRawCookiesResponse(reply_msg, net::CookieList());
  }

  // We check policy here to avoid sending back cookies that would not normally
  // be applied to outbound requests for the given URL.  Since this cookie info
  // is visible in the developer tools, it is helpful to make it match reality.
  net::URLRequestContext* context = GetRequestContextForURL(url);
  net::CookieMonster* cookie_monster =
      context->cookie_store()->GetCookieMonster();
  cookie_monster->GetAllCookiesForURLAsync(
      url, base::Bind(&RenderMessageFilter::SendGetRawCookiesResponse,
                      this, reply_msg));
}

void RenderMessageFilter::OnDeleteCookie(const GURL& url,
                                         const std::string& cookie_name) {
  net::URLRequestContext* context = GetRequestContextForURL(url);
  context->cookie_store()->DeleteCookieAsync(url, cookie_name, base::Closure());
}

void RenderMessageFilter::OnCookiesEnabled(
    const GURL& url,
    const GURL& first_party_for_cookies,
    bool* cookies_enabled) {
  // TODO(ananta): If this render view is associated with an automation channel,
  // aka ChromeFrame then we need to retrieve cookie settings from the external
  // host.
  *cookies_enabled = content::GetContentClient()->browser()->AllowGetCookie(
      url, first_party_for_cookies, net::CookieList(), resource_context_,
      render_process_id_, MSG_ROUTING_CONTROL);
}

#if defined(OS_MACOSX)
void RenderMessageFilter::OnLoadFont(const FontDescriptor& font,
                                     uint32* handle_size,
                                     base::SharedMemoryHandle* handle,
                                     uint32* font_id) {
  base::SharedMemory font_data;
  uint32 font_data_size = 0;
  bool ok = FontLoader::LoadFontIntoBuffer(font.ToNSFont(), &font_data,
                &font_data_size, font_id);
  if (!ok || font_data_size == 0 || *font_id == 0) {
    LOG(ERROR) << "Couldn't load font data for " << font.font_name <<
        " ok=" << ok << " font_data_size=" << font_data_size <<
        " font id=" << *font_id;
    *handle_size = 0;
    *handle = base::SharedMemory::NULLHandle();
    *font_id = 0;
    return;
  }

  *handle_size = font_data_size;
  font_data.GiveToProcess(base::GetCurrentProcessHandle(), handle);
}
#endif  // OS_MACOSX

#if defined(OS_WIN)  // This hack is Windows-specific.
void RenderMessageFilter::OnPreCacheFont(const LOGFONT& font) {
  ChildProcessHost::PreCacheFont(font);
}
#endif  // OS_WIN

void RenderMessageFilter::OnGetPlugins(
    bool refresh,
    std::vector<webkit::WebPluginInfo>* plugins) {
  // Don't refresh if the specified threshold has not been passed.  Note that
  // this check is performed before off-loading to the file thread.  The reason
  // we do this is that some pages tend to request that the list of plugins be
  // refreshed at an excessive rate.  This instigates disk scanning, as the list
  // is accumulated by doing multiple reads from disk.  This effect is
  // multiplied when we have several pages requesting this operation.
  if (refresh) {
    const base::TimeDelta threshold = base::TimeDelta::FromSeconds(
        kPluginsRefreshThresholdInSeconds);
    const base::TimeTicks now = base::TimeTicks::Now();
    if (now - last_plugin_refresh_time_ >= threshold) {
      // Only refresh if the threshold hasn't been exceeded yet.
      webkit::npapi::PluginList::Singleton()->RefreshPlugins();
      last_plugin_refresh_time_ = now;
    }
  }

  PluginService::GetInstance()->GetPlugins(resource_context_,
                                           plugins);
}

void RenderMessageFilter::OnGetPluginInfo(
    int routing_id,
    const GURL& url,
    const GURL& page_url,
    const std::string& mime_type,
    bool* found,
    webkit::WebPluginInfo* info,
    std::string* actual_mime_type) {
  bool allow_wildcard = true;
  *found = plugin_service_->GetPluginInfo(
      render_process_id_, routing_id, resource_context_,
      url, page_url, mime_type, allow_wildcard,
      NULL, info, actual_mime_type);
}

void RenderMessageFilter::OnOpenChannelToPlugin(int routing_id,
                                                const GURL& url,
                                                const GURL& policy_url,
                                                const std::string& mime_type,
                                                IPC::Message* reply_msg) {
  OpenChannelToNpapiPluginCallback* client =
      new OpenChannelToNpapiPluginCallback(this, resource_context_, reply_msg);
  DCHECK(!ContainsKey(plugin_host_clients_, client));
  plugin_host_clients_.insert(client);
  plugin_service_->OpenChannelToNpapiPlugin(
      render_process_id_, routing_id,
      url, policy_url, mime_type, client);
}

void RenderMessageFilter::OnOpenChannelToPepperPlugin(
    const FilePath& path,
    IPC::Message* reply_msg) {
  plugin_service_->OpenChannelToPpapiPlugin(
      path,
      new OpenChannelToPpapiPluginCallback(
          this, &resource_context_, reply_msg));
}

void RenderMessageFilter::OnOpenChannelToPpapiBroker(int routing_id,
                                                     int request_id,
                                                     const FilePath& path) {
  plugin_service_->OpenChannelToPpapiBroker(
      path, new OpenChannelToPpapiBrokerCallback(this, routing_id, request_id));
}

void RenderMessageFilter::OnGenerateRoutingID(int* route_id) {
  *route_id = render_widget_helper_->GetNextRoutingID();
}

void RenderMessageFilter::OnGetHardwareSampleRate(double* sample_rate) {
  *sample_rate = media::GetAudioHardwareSampleRate();
}

void RenderMessageFilter::OnDownloadUrl(const IPC::Message& message,
                                        const GURL& url,
                                        const GURL& referrer,
                                        const string16& suggested_name) {
  // Don't show "Save As" UI.
  bool prompt_for_save_location = false;
  DownloadSaveInfo save_info;
  save_info.suggested_name = suggested_name;
  resource_dispatcher_host_->BeginDownload(url,
                                           referrer,
                                           save_info,
                                           prompt_for_save_location,
                                           render_process_id_,
                                           message.routing_id(),
                                           resource_context_);
  download_stats::RecordDownloadCount(
      download_stats::INITIATED_BY_RENDERER_COUNT);
}

void RenderMessageFilter::OnCheckNotificationPermission(
    const GURL& source_url, int* result) {
  *result = content::GetContentClient()->browser()->
      CheckDesktopNotificationPermission(source_url, resource_context_);
}

void RenderMessageFilter::OnAllocateSharedMemoryBuffer(
    uint32 buffer_size,
    base::SharedMemoryHandle* handle) {
  base::SharedMemory shared_buf;
  if (!shared_buf.CreateAndMapAnonymous(buffer_size)) {
    *handle = base::SharedMemory::NULLHandle();
    NOTREACHED() << "Cannot map shared memory buffer";
    return;
  }
  shared_buf.GiveToProcess(peer_handle(), handle);
}

net::URLRequestContext* RenderMessageFilter::GetRequestContextForURL(
    const GURL& url) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  net::URLRequestContext* context =
      content::GetContentClient()->browser()->OverrideRequestContextForURL(
          url, resource_context_);
  if (!context)
    context = request_context_->GetURLRequestContext();

  return context;
}

#if defined(OS_MACOSX)
void RenderMessageFilter::OnAllocTransportDIB(
    size_t size, bool cache_in_browser, TransportDIB::Handle* handle) {
  render_widget_helper_->AllocTransportDIB(size, cache_in_browser, handle);
}

void RenderMessageFilter::OnFreeTransportDIB(
    TransportDIB::Id dib_id) {
  render_widget_helper_->FreeTransportDIB(dib_id);
}
#endif

bool RenderMessageFilter::CheckBenchmarkingEnabled() const {
  static bool checked = false;
  static bool result = false;
  if (!checked) {
    const CommandLine& command_line = *CommandLine::ForCurrentProcess();
    result = command_line.HasSwitch(switches::kEnableBenchmarking);
    checked = true;
  }
  return result;
}

void RenderMessageFilter::OnCloseCurrentConnections() {
  // This function is disabled unless the user has enabled
  // benchmarking extensions.
  if (!CheckBenchmarkingEnabled())
    return;
  request_context_->GetURLRequestContext()->
      http_transaction_factory()->GetCache()->CloseAllConnections();
}

void RenderMessageFilter::OnSetCacheMode(bool enabled) {
  // This function is disabled unless the user has enabled
  // benchmarking extensions.
  if (!CheckBenchmarkingEnabled())
    return;

  net::HttpCache::Mode mode = enabled ?
      net::HttpCache::NORMAL : net::HttpCache::DISABLE;
  net::HttpCache* http_cache = request_context_->GetURLRequestContext()->
      http_transaction_factory()->GetCache();
  http_cache->set_mode(mode);
}

void RenderMessageFilter::OnClearCache(bool preserve_ssl_host_info,
                                       IPC::Message* reply_msg) {
  // This function is disabled unless the user has enabled
  // benchmarking extensions.
  int rv = -1;
  if (CheckBenchmarkingEnabled()) {
    disk_cache::Backend* backend = request_context_->GetURLRequestContext()->
        http_transaction_factory()->GetCache()->GetCurrentBackend();
    if (backend) {
      ClearCacheCompletion* callback =
          new ClearCacheCompletion(this, reply_msg);
      if (preserve_ssl_host_info) {
        DoomEntriesHelper* helper = new DoomEntriesHelper(backend);
        helper->ClearCache(callback);  // Will self clean.
        return;
      } else {
        rv = backend->DoomAllEntries(callback);
        if (rv == net::ERR_IO_PENDING) {
          // The callback will send the reply.
          return;
        }
        // Completed synchronously, no need for the callback.
        delete callback;
      }
    }
  }
  ViewHostMsg_ClearCache::WriteReplyParams(reply_msg, rv);
  Send(reply_msg);
}

void RenderMessageFilter::OnClearHostResolverCache(int* result) {
  // This function is disabled unless the user has enabled
  // benchmarking extensions.
  *result = -1;
  DCHECK(CheckBenchmarkingEnabled());
  net::HostResolverImpl* host_resolver_impl =
      request_context_->GetURLRequestContext()->
      host_resolver()->GetAsHostResolverImpl();
  if (host_resolver_impl) {
    net::HostCache* cache = host_resolver_impl->cache();
    DCHECK(cache);
    cache->clear();
    *result = 0;
  }
}

bool RenderMessageFilter::CheckPreparsedJsCachingEnabled() const {
  static bool checked = false;
  static bool result = false;
  if (!checked) {
    const CommandLine& command_line = *CommandLine::ForCurrentProcess();
    result = command_line.HasSwitch(switches::kEnablePreparsedJsCaching);
    checked = true;
  }
  return result;
}

void RenderMessageFilter::OnCacheableMetadataAvailable(
    const GURL& url,
    double expected_response_time,
    const std::vector<char>& data) {
  if (!CheckPreparsedJsCachingEnabled())
    return;

  net::HttpCache* cache = request_context_->GetURLRequestContext()->
      http_transaction_factory()->GetCache();
  DCHECK(cache);

  scoped_refptr<net::IOBuffer> buf(new net::IOBuffer(data.size()));
  memcpy(buf->data(), &data.front(), data.size());
  cache->WriteMetadata(
      url, base::Time::FromDoubleT(expected_response_time), buf, data.size());
}

// TODO(lzheng): This only enables spdy over ssl. Enable spdy for http
// when needed.
void RenderMessageFilter::OnEnableSpdy(bool enable) {
  if (enable) {
    net::HttpNetworkLayer::EnableSpdy("npn,force-alt-protocols");
  } else {
    net::HttpNetworkLayer::EnableSpdy("npn-http");
  }
}

void RenderMessageFilter::OnKeygen(uint32 key_size_index,
                                   const std::string& challenge_string,
                                   const GURL& url,
                                   IPC::Message* reply_msg) {
  // Map displayed strings indicating level of keysecurity in the <keygen>
  // menu to the key size in bits. (See SSLKeyGeneratorChromium.cpp in WebCore.)
  int key_size_in_bits;
  switch (key_size_index) {
    case 0:
      key_size_in_bits = 2048;
      break;
    case 1:
      key_size_in_bits = 1024;
      break;
    default:
      DCHECK(false) << "Illegal key_size_index " << key_size_index;
      ViewHostMsg_Keygen::WriteReplyParams(reply_msg, std::string());
      Send(reply_msg);
      return;
  }

  VLOG(1) << "Dispatching keygen task to worker pool.";
  // Dispatch to worker pool, so we do not block the IO thread.
  if (!base::WorkerPool::PostTask(
           FROM_HERE,
           NewRunnableMethod(
               this, &RenderMessageFilter::OnKeygenOnWorkerThread,
               key_size_in_bits, challenge_string, url, reply_msg),
           true)) {
    NOTREACHED() << "Failed to dispatch keygen task to worker pool";
    ViewHostMsg_Keygen::WriteReplyParams(reply_msg, std::string());
    Send(reply_msg);
    return;
  }
}

void RenderMessageFilter::OnKeygenOnWorkerThread(
    int key_size_in_bits,
    const std::string& challenge_string,
    const GURL& url,
    IPC::Message* reply_msg) {
  DCHECK(reply_msg);

  // Generate a signed public key and challenge, then send it back.
  net::KeygenHandler keygen_handler(key_size_in_bits, challenge_string, url);

#if defined(USE_NSS)
  // Attach a password delegate so we can authenticate.
  keygen_handler.set_crypto_module_password_delegate(
      content::GetContentClient()->browser()->GetCryptoPasswordDelegate(url));
#endif  // defined(USE_NSS)

  ViewHostMsg_Keygen::WriteReplyParams(
      reply_msg,
      keygen_handler.GenKeyAndSignChallenge());
  Send(reply_msg);
}

void RenderMessageFilter::OnAsyncOpenFile(const IPC::Message& msg,
                                          const FilePath& path,
                                          int flags,
                                          int message_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!ChildProcessSecurityPolicy::GetInstance()->HasPermissionsForFile(
          render_process_id_, path, flags)) {
    DLOG(ERROR) << "Bad flags in ViewMsgHost_AsyncOpenFile message: " << flags;
    UserMetrics::RecordAction(UserMetricsAction("BadMessageTerminate_AOF"));
    BadMessageReceived();
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE, NewRunnableMethod(
          this, &RenderMessageFilter::AsyncOpenFileOnFileThread,
          path, flags, message_id, msg.routing_id()));
}

void RenderMessageFilter::AsyncOpenFileOnFileThread(const FilePath& path,
                                                    int flags,
                                                    int message_id,
                                                    int routing_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  base::PlatformFileError error_code = base::PLATFORM_FILE_OK;
  base::PlatformFile file = base::CreatePlatformFile(
      path, flags, NULL, &error_code);
  IPC::PlatformFileForTransit file_for_transit =
      file != base::kInvalidPlatformFileValue ?
          IPC::GetFileHandleForProcess(file, peer_handle(), true) :
          IPC::InvalidPlatformFileForTransit();

  IPC::Message* reply = new ViewMsg_AsyncOpenFile_ACK(
      routing_id, error_code, file_for_transit, message_id);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE, NewRunnableMethod(
          this, &RenderMessageFilter::Send, reply));
}

void RenderMessageFilter::OnMediaLogEvent(const media::MediaLogEvent& event) {
  resource_context_.media_observer()->OnMediaEvent(render_process_id_, event);
}

void RenderMessageFilter::CheckPolicyForCookies(
    const GURL& url,
    const GURL& first_party_for_cookies,
    IPC::Message* reply_msg,
    const net::CookieList& cookie_list) {
  net::URLRequestContext* context = GetRequestContextForURL(url);
  // Check the policy for get cookies, and pass cookie_list to the
  // TabSpecificContentSetting for logging purose.
  if (content::GetContentClient()->browser()->AllowGetCookie(
          url, first_party_for_cookies, cookie_list, resource_context_,
          render_process_id_, reply_msg->routing_id())) {
    // Gets the cookies from cookie store if allowed.
    context->cookie_store()->GetCookiesWithOptionsAsync(
        url, net::CookieOptions(),
        base::Bind(&RenderMessageFilter::SendGetCookiesResponse,
                   this, reply_msg));
  } else {
    SendGetCookiesResponse(reply_msg, std::string());
  }
}

void RenderMessageFilter::SendGetCookiesResponse(IPC::Message* reply_msg,
                                                 const std::string& cookies) {
  ViewHostMsg_GetCookies::WriteReplyParams(reply_msg, cookies);
  Send(reply_msg);
}

void RenderMessageFilter::SendGetRawCookiesResponse(
    IPC::Message* reply_msg,
    const net::CookieList& cookie_list) {
  std::vector<webkit_glue::WebCookie> cookies;
  for (size_t i = 0; i < cookie_list.size(); ++i)
    cookies.push_back(webkit_glue::WebCookie(cookie_list[i]));
  ViewHostMsg_GetRawCookies::WriteReplyParams(reply_msg, cookies);
  Send(reply_msg);
}

void RenderMessageFilter::OnCompletedOpenChannelToNpapiPlugin(
    OpenChannelToNpapiPluginCallback* client) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(ContainsKey(plugin_host_clients_, client));
  plugin_host_clients_.erase(client);
}
