// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_API_H_
#define EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_API_H_

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "content/public/common/resource_type.h"
#include "extensions/browser/api/declarative/rules_registry.h"
#include "extensions/browser/api/declarative_webrequest/request_stage.h"
#include "extensions/browser/api/web_request/web_request_api_helpers.h"
#include "extensions/browser/api/web_request/web_request_permissions.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/url_pattern_set.h"
#include "ipc/ipc_sender.h"
#include "net/base/completion_callback.h"
#include "net/base/network_delegate.h"
#include "net/http/http_request_headers.h"

class ExtensionWebRequestTimeTracker;
class GURL;

namespace base {
class DictionaryValue;
class ListValue;
class StringValue;
}

namespace content {
class BrowserContext;
}

namespace net {
class AuthCredentials;
class AuthChallengeInfo;
class HttpRequestHeaders;
class HttpResponseHeaders;
class URLRequest;
}

namespace extensions {

class InfoMap;
class WebRequestRulesRegistry;
class WebRequestEventRouterDelegate;

// Support class for the WebRequest API. Lives on the UI thread. Most of the
// work is done by ExtensionWebRequestEventRouter below. This class observes
// extensions::EventRouter to deal with event listeners. There is one instance
// per BrowserContext which is shared with incognito.
class WebRequestAPI : public BrowserContextKeyedAPI,
                      public EventRouter::Observer {
 public:
  explicit WebRequestAPI(content::BrowserContext* context);
  ~WebRequestAPI() override;

  // BrowserContextKeyedAPI support:
  static BrowserContextKeyedAPIFactory<WebRequestAPI>* GetFactoryInstance();

  // EventRouter::Observer overrides:
  void OnListenerRemoved(const EventListenerInfo& details) override;

 private:
  friend class BrowserContextKeyedAPIFactory<WebRequestAPI>;

  // BrowserContextKeyedAPI support:
  static const char* service_name() { return "WebRequestAPI"; }
  static const bool kServiceRedirectedInIncognito = true;
  static const bool kServiceIsNULLWhileTesting = true;

  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(WebRequestAPI);
};

// This class observes network events and routes them to the appropriate
// extensions listening to those events. All methods must be called on the IO
// thread unless otherwise specified.
class ExtensionWebRequestEventRouter
    : public base::SupportsWeakPtr<ExtensionWebRequestEventRouter> {
 public:
  struct BlockedRequest;

  enum EventTypes {
    kInvalidEvent = 0,
    kOnBeforeRequest = 1 << 0,
    kOnBeforeSendHeaders = 1 << 1,
    kOnSendHeaders = 1 << 2,
    kOnHeadersReceived = 1 << 3,
    kOnBeforeRedirect = 1 << 4,
    kOnAuthRequired = 1 << 5,
    kOnResponseStarted = 1 << 6,
    kOnErrorOccurred = 1 << 7,
    kOnCompleted = 1 << 8,
  };

  // Internal representation of the webRequest.RequestFilter type, used to
  // filter what network events an extension cares about.
  struct RequestFilter {
    RequestFilter();
    ~RequestFilter();

    // Returns false if there was an error initializing. If it is a user error,
    // an error message is provided, otherwise the error is internal (and
    // unexpected).
    bool InitFromValue(const base::DictionaryValue& value, std::string* error);

    extensions::URLPatternSet urls;
    std::vector<content::ResourceType> types;
    int tab_id;
    int window_id;
  };

  // Internal representation of the extraInfoSpec parameter on webRequest
  // events, used to specify extra information to be included with network
  // events.
  struct ExtraInfoSpec {
    enum Flags {
      REQUEST_HEADERS = 1<<0,
      RESPONSE_HEADERS = 1<<1,
      BLOCKING = 1<<2,
      ASYNC_BLOCKING = 1<<3,
      REQUEST_BODY = 1<<4,
    };

    static bool InitFromValue(const base::ListValue& value,
                              int* extra_info_spec);
  };

  // Contains an extension's response to a blocking event.
  struct EventResponse {
    EventResponse(const std::string& extension_id,
                  const base::Time& extension_install_time);
    ~EventResponse();

    // ID of the extension that sent this response.
    std::string extension_id;

    // The time that the extension was installed. Used for deciding order of
    // precedence in case multiple extensions respond with conflicting
    // decisions.
    base::Time extension_install_time;

    // Response values. These are mutually exclusive.
    bool cancel;
    GURL new_url;
    scoped_ptr<net::HttpRequestHeaders> request_headers;
    scoped_ptr<extension_web_request_api_helpers::ResponseHeaders>
        response_headers;

    scoped_ptr<net::AuthCredentials> auth_credentials;

    DISALLOW_COPY_AND_ASSIGN(EventResponse);
  };

  static ExtensionWebRequestEventRouter* GetInstance();

  // Registers a rule registry. Pass null for |rules_registry| to unregister
  // the rule registry for |browser_context|.
  void RegisterRulesRegistry(
      void* browser_context,
      int rules_registry_id,
      scoped_refptr<extensions::WebRequestRulesRegistry> rules_registry);

  // Dispatches the OnBeforeRequest event to any extensions whose filters match
  // the given request. Returns net::ERR_IO_PENDING if an extension is
  // intercepting the request, OK otherwise.
  int OnBeforeRequest(void* browser_context,
                      extensions::InfoMap* extension_info_map,
                      net::URLRequest* request,
                      const net::CompletionCallback& callback,
                      GURL* new_url);

  // Dispatches the onBeforeSendHeaders event. This is fired for HTTP(s)
  // requests only, and allows modification of the outgoing request headers.
  // Returns net::ERR_IO_PENDING if an extension is intercepting the request, OK
  // otherwise.
  int OnBeforeSendHeaders(void* browser_context,
                          extensions::InfoMap* extension_info_map,
                          net::URLRequest* request,
                          const net::CompletionCallback& callback,
                          net::HttpRequestHeaders* headers);

  // Dispatches the onSendHeaders event. This is fired for HTTP(s) requests
  // only.
  void OnSendHeaders(void* browser_context,
                     extensions::InfoMap* extension_info_map,
                     net::URLRequest* request,
                     const net::HttpRequestHeaders& headers);

  // Dispatches the onHeadersReceived event. This is fired for HTTP(s)
  // requests only, and allows modification of incoming response headers.
  // Returns net::ERR_IO_PENDING if an extension is intercepting the request,
  // OK otherwise. |original_response_headers| is reference counted. |callback|
  // |override_response_headers| and |allowed_unsafe_redirect_url| are owned by
  // a URLRequestJob. They are guaranteed to be valid until |callback| is called
  // or OnURLRequestDestroyed is called (whatever comes first).
  // Do not modify |original_response_headers| directly but write new ones
  // into |override_response_headers|.
  int OnHeadersReceived(
      void* browser_context,
      extensions::InfoMap* extension_info_map,
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url);

  // Dispatches the OnAuthRequired event to any extensions whose filters match
  // the given request. If the listener is not registered as "blocking", then
  // AUTH_REQUIRED_RESPONSE_OK is returned. Otherwise,
  // AUTH_REQUIRED_RESPONSE_IO_PENDING is returned and |callback| will be
  // invoked later.
  net::NetworkDelegate::AuthRequiredResponse OnAuthRequired(
      void* browser_context,
      extensions::InfoMap* extension_info_map,
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      const net::NetworkDelegate::AuthCallback& callback,
      net::AuthCredentials* credentials);

  // Dispatches the onBeforeRedirect event. This is fired for HTTP(s) requests
  // only.
  void OnBeforeRedirect(void* browser_context,
                        extensions::InfoMap* extension_info_map,
                        net::URLRequest* request,
                        const GURL& new_location);

  // Dispatches the onResponseStarted event indicating that the first bytes of
  // the response have arrived.
  void OnResponseStarted(void* browser_context,
                         extensions::InfoMap* extension_info_map,
                         net::URLRequest* request);

  // Dispatches the onComplete event.
  void OnCompleted(void* browser_context,
                   extensions::InfoMap* extension_info_map,
                   net::URLRequest* request);

  // Dispatches an onErrorOccurred event.
  void OnErrorOccurred(void* browser_context,
                       extensions::InfoMap* extension_info_map,
                       net::URLRequest* request,
                       bool started);

  // Notifications when objects are going away.
  void OnURLRequestDestroyed(void* browser_context, net::URLRequest* request);

  // Called when an event listener handles a blocking event and responds.
  void OnEventHandled(
      void* browser_context,
      const std::string& extension_id,
      const std::string& event_name,
      const std::string& sub_event_name,
      uint64 request_id,
      EventResponse* response);

  // Adds a listener to the given event. |event_name| specifies the event being
  // listened to. |sub_event_name| is an internal event uniquely generated in
  // the extension process to correspond to the given filter and
  // extra_info_spec. It returns true on success, false on failure.
  bool AddEventListener(
      void* browser_context,
      const std::string& extension_id,
      const std::string& extension_name,
      const std::string& event_name,
      const std::string& sub_event_name,
      const RequestFilter& filter,
      int extra_info_spec,
      int embedder_process_id,
      int web_view_instance_id,
      base::WeakPtr<IPC::Sender> ipc_sender);

  // Removes the listener for the given sub-event.
  void RemoveEventListener(
      void* browser_context,
      const std::string& extension_id,
      const std::string& sub_event_name,
      int embedder_process_id,
      int web_view_instance_id);

  // Removes the listeners for a given <webview>.
  void RemoveWebViewEventListeners(
      void* browser_context,
      int embedder_process_id,
      int web_view_instance_id);

  // Called when an incognito browser_context is created or destroyed.
  void OnOTRBrowserContextCreated(void* original_browser_context,
                                  void* otr_browser_context);
  void OnOTRBrowserContextDestroyed(void* original_browser_context,
                                    void* otr_browser_context);

  // Registers a |callback| that is executed when the next page load happens.
  // The callback is then deleted.
  void AddCallbackForPageLoad(const base::Closure& callback);

 private:
  friend struct DefaultSingletonTraits<ExtensionWebRequestEventRouter>;

  struct EventListener;
  typedef std::map<std::string, std::set<EventListener> >
      ListenerMapForBrowserContext;
  typedef std::map<void*, ListenerMapForBrowserContext> ListenerMap;
  typedef std::map<uint64, BlockedRequest> BlockedRequestMap;
  // Map of request_id -> bit vector of EventTypes already signaled
  typedef std::map<uint64, int> SignaledRequestMap;
  // For each browser_context: a bool indicating whether it is an incognito
  // browser_context, and a pointer to the corresponding (non-)incognito
  // browser_context.
  typedef std::map<void*, std::pair<bool, void*> > CrossBrowserContextMap;
  typedef std::list<base::Closure> CallbacksForPageLoad;

  ExtensionWebRequestEventRouter();
  ~ExtensionWebRequestEventRouter();

  // Ensures that future callbacks for |request| are ignored so that it can be
  // destroyed safely.
  void ClearPendingCallbacks(net::URLRequest* request);

  bool DispatchEvent(
      void* browser_context,
      net::URLRequest* request,
      const std::vector<const EventListener*>& listeners,
      const base::ListValue& args);

  // Returns a list of event listeners that care about the given event, based
  // on their filter parameters. |extra_info_spec| will contain the combined
  // set of extra_info_spec flags that every matching listener asked for.
  std::vector<const EventListener*> GetMatchingListeners(
      void* browser_context,
      extensions::InfoMap* extension_info_map,
      const std::string& event_name,
      net::URLRequest* request,
      int* extra_info_spec);

  // Helper for the above functions. This is called twice: once for the
  // browser_context of the event, the next time for the "cross" browser_context
  // (i.e. the incognito browser_context if the event is originally for the
  // normal browser_context, or vice versa).
  void GetMatchingListenersImpl(
      void* browser_context,
      net::URLRequest* request,
      extensions::InfoMap* extension_info_map,
      bool crosses_incognito,
      const std::string& event_name,
      const GURL& url,
      int render_process_host_id,
      int routing_id,
      content::ResourceType resource_type,
      bool is_async_request,
      bool is_request_from_extension,
      int* extra_info_spec,
      std::vector<const ExtensionWebRequestEventRouter::EventListener*>*
          matching_listeners);

  // Decrements the count of event handlers blocking the given request. When the
  // count reaches 0, we stop blocking the request and proceed it using the
  // method requested by the extension with the highest precedence. Precedence
  // is decided by extension install time. If |response| is non-NULL, this
  // method assumes ownership.
  void DecrementBlockCount(
      void* browser_context,
      const std::string& extension_id,
      const std::string& event_name,
      uint64 request_id,
      EventResponse* response);

  // Logs an extension action.
  void LogExtensionActivity(
      void* browser_context_id,
      bool is_incognito,
      const std::string& extension_id,
      const GURL& url,
      const std::string& api_call,
      scoped_ptr<base::DictionaryValue> details);

  // Processes the generated deltas from blocked_requests_ on the specified
  // request. If |call_back| is true, the callback registered in
  // |blocked_requests_| is called.
  // The function returns the error code for the network request. This is
  // mostly relevant in case the caller passes |call_callback| = false
  // and wants to return the correct network error code himself.
  int ExecuteDeltas(
      void* browser_context, uint64 request_id, bool call_callback);

  // Evaluates the rules of the declarative webrequest API and stores
  // modifications to the request that result from WebRequestActions as
  // deltas in |blocked_requests_|. |original_response_headers| should only be
  // set for the OnHeadersReceived stage and NULL otherwise. Returns whether any
  // deltas were generated.
  bool ProcessDeclarativeRules(
      void* browser_context,
      extensions::InfoMap* extension_info_map,
      const std::string& event_name,
      net::URLRequest* request,
      extensions::RequestStage request_stage,
      const net::HttpResponseHeaders* original_response_headers);

  // If the BlockedRequest contains messages_to_extension entries in the event
  // deltas, we send them to subscribers of
  // chrome.declarativeWebRequest.onMessage.
  void SendMessages(
      void* browser_context, const BlockedRequest& blocked_request);

  // Called when the RulesRegistry is ready to unblock a request that was
  // waiting for said event.
  void OnRulesRegistryReady(
      void* browser_context,
      const std::string& event_name,
      uint64 request_id,
      extensions::RequestStage request_stage);

  // Extracts from |request| information for the keys requestId, url, method,
  // frameId, tabId, type, and timeStamp and writes these into |out| to be
  // passed on to extensions.
  void ExtractRequestInfo(net::URLRequest* request, base::DictionaryValue* out);

  // Sets the flag that |event_type| has been signaled for |request_id|.
  // Returns the value of the flag before setting it.
  bool GetAndSetSignaled(uint64 request_id, EventTypes event_type);

  // Clears the flag that |event_type| has been signaled for |request_id|.
  void ClearSignaled(uint64 request_id, EventTypes event_type);

  // Returns whether |request| represents a top level window navigation.
  bool IsPageLoad(net::URLRequest* request) const;

  // Called on a page load to process all registered callbacks.
  void NotifyPageLoad();

  // Returns the matching cross browser_context (the regular browser_context if
  // |browser_context| is OTR and vice versa).
  void* GetCrossBrowserContext(void* browser_context) const;

  // Determines whether the specified browser_context is an incognito
  // browser_context (based on the contents of the cross-browser_context table
  // and without dereferencing the browser_context pointer).
  bool IsIncognitoBrowserContext(void* browser_context) const;

  // Returns true if |request| was already signaled to some event handlers.
  bool WasSignaled(const net::URLRequest& request) const;

  // A map for each browser_context that maps an event name to a set of
  // extensions that are listening to that event.
  ListenerMap listeners_;

  // A map of network requests that are waiting for at least one event handler
  // to respond.
  BlockedRequestMap blocked_requests_;

  // A map of request ids to a bitvector indicating which events have been
  // signaled and should not be sent again.
  SignaledRequestMap signaled_requests_;

  // A map of original browser_context -> corresponding incognito
  // browser_context (and vice versa).
  CrossBrowserContextMap cross_browser_context_map_;

  // Keeps track of time spent waiting on extensions using the blocking
  // webRequest API.
  scoped_ptr<ExtensionWebRequestTimeTracker> request_time_tracker_;

  CallbacksForPageLoad callbacks_for_page_load_;

  typedef std::pair<void*, int> RulesRegistryKey;
  // Maps each browser_context (and OTRBrowserContext) and a webview key to its
  // respective rules registry.
  std::map<RulesRegistryKey,
      scoped_refptr<extensions::WebRequestRulesRegistry> > rules_registries_;

  scoped_ptr<extensions::WebRequestEventRouterDelegate>
      web_request_event_router_delegate_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionWebRequestEventRouter);
};

class WebRequestInternalFunction : public SyncIOThreadExtensionFunction {
 public:
  WebRequestInternalFunction() {}

 protected:
  ~WebRequestInternalFunction() override {}

  const std::string& extension_id_safe() const {
    return extension() ? extension_id() : base::EmptyString();
  }
};

class WebRequestInternalAddEventListenerFunction
    : public WebRequestInternalFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("webRequestInternal.addEventListener",
                             WEBREQUESTINTERNAL_ADDEVENTLISTENER)

 protected:
  ~WebRequestInternalAddEventListenerFunction() override {}

  // ExtensionFunction:
  bool RunSync() override;
};

class WebRequestInternalEventHandledFunction
    : public WebRequestInternalFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("webRequestInternal.eventHandled",
                             WEBREQUESTINTERNAL_EVENTHANDLED)

 protected:
  ~WebRequestInternalEventHandledFunction() override {}

  // Unblocks the network request and sets |error_| such that the developer
  // console will show the respective error message. Use this function to handle
  // incorrect requests from the extension that cannot be detected by the schema
  // validator.
  void RespondWithError(
      const std::string& event_name,
      const std::string& sub_event_name,
      uint64 request_id,
      scoped_ptr<ExtensionWebRequestEventRouter::EventResponse> response,
      const std::string& error);

  // ExtensionFunction:
  bool RunSync() override;
};

class WebRequestHandlerBehaviorChangedFunction
    : public WebRequestInternalFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("webRequest.handlerBehaviorChanged",
                             WEBREQUEST_HANDLERBEHAVIORCHANGED)

 protected:
  ~WebRequestHandlerBehaviorChangedFunction() override {}

  // ExtensionFunction:
  void GetQuotaLimitHeuristics(
      extensions::QuotaLimitHeuristics* heuristics) const override;
  // Handle quota exceeded gracefully: Only warn the user but still execute the
  // function.
  void OnQuotaExceeded(const std::string& error) override;
  bool RunSync() override;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_API_H_
