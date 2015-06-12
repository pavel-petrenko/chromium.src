// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/messaging_bindings.h"

#include <map>
#include <string>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/values.h"
#include "components/guest_view/common/guest_view_constants.h"
#include "content/public/child/v8_value_converter.h"
#include "content/public/common/child_process_host.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/manifest_handlers/externally_connectable.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/event_bindings.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScopedMicrotaskSuppression.h"
#include "third_party/WebKit/public/web/WebScopedUserGesture.h"
#include "third_party/WebKit/public/web/WebScopedWindowFocusAllowedIndicator.h"
#include "third_party/WebKit/public/web/WebUserGestureIndicator.h"
#include "v8/include/v8.h"

// Message passing API example (in a content script):
// var extension =
//    new chrome.Extension('00123456789abcdef0123456789abcdef0123456');
// var port = runtime.connect();
// port.postMessage('Can you hear me now?');
// port.onmessage.addListener(function(msg, port) {
//   alert('response=' + msg);
//   port.postMessage('I got your reponse');
// });

using content::RenderThread;
using content::V8ValueConverter;

namespace extensions {

namespace {

// Binds |callback| to run when |object| is garbage collected. So as to not
// re-entrantly call into v8, we execute this function asynchronously, at
// which point |context| may have been invalidated. If so, |callback| is not
// run, and |fallback| will be called instead.
//
// Deletes itself when the object args[0] is garbage collected or when the
// context is invalidated.
class GCCallback : public base::SupportsWeakPtr<GCCallback> {
 public:
  GCCallback(ScriptContext* context,
             const v8::Local<v8::Object>& object,
             const v8::Local<v8::Function>& callback,
             const base::Closure& fallback)
      : context_(context),
        object_(context->isolate(), object),
        callback_(context->isolate(), callback),
        fallback_(fallback) {
    object_.SetWeak(this, FirstWeakCallback, v8::WeakCallbackType::kParameter);
    context->AddInvalidationObserver(
        base::Bind(&GCCallback::OnContextInvalidated, AsWeakPtr()));
  }

 private:
  static void FirstWeakCallback(const v8::WeakCallbackInfo<GCCallback>& data) {
    // v8 says we need to explicitly reset weak handles from their callbacks.
    // It's not implicit as one might expect.
    data.GetParameter()->object_.Reset();
    data.SetSecondPassCallback(SecondWeakCallback);
  }

  static void SecondWeakCallback(const v8::WeakCallbackInfo<GCCallback>& data) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&GCCallback::RunCallback, data.GetParameter()->AsWeakPtr()));
  }

  void RunCallback() {
    CHECK(context_);
    v8::Isolate* isolate = context_->isolate();
    v8::HandleScope handle_scope(isolate);
    context_->CallFunction(v8::Local<v8::Function>::New(isolate, callback_));
    delete this;
  }

  void OnContextInvalidated() {
    fallback_.Run();
    context_ = NULL;
    delete this;
  }

  // ScriptContext which owns this GCCallback.
  ScriptContext* context_;

  // Holds a global handle to the object this GCCallback is bound to.
  v8::Global<v8::Object> object_;

  // Function to run when |object_| bound to this GCCallback is GC'd.
  v8::Global<v8::Function> callback_;

  // Function to run if context is invalidated before we have a chance
  // to execute |callback_|.
  base::Closure fallback_;

  DISALLOW_COPY_AND_ASSIGN(GCCallback);
};

// Tracks every reference between ScriptContexts and Ports, by ID.
class PortTracker {
 public:
  PortTracker() {}
  ~PortTracker() {}

  // Returns true if |context| references |port_id|.
  bool HasReference(ScriptContext* context, int port_id) const {
    auto ports = contexts_to_ports_.find(context);
    return ports != contexts_to_ports_.end() &&
           ports->second.count(port_id) > 0;
  }

  // Marks |context| and |port_id| as referencing each other.
  void AddReference(ScriptContext* context, int port_id) {
    contexts_to_ports_[context].insert(port_id);
  }

  // Removes the references between |context| and |port_id|.
  // Returns true if a reference was removed, false if the reference didn't
  // exist to be removed.
  bool RemoveReference(ScriptContext* context, int port_id) {
    auto ports = contexts_to_ports_.find(context);
    if (ports == contexts_to_ports_.end() ||
        ports->second.erase(port_id) == 0) {
      return false;
    }
    if (ports->second.empty())
      contexts_to_ports_.erase(context);
    return true;
  }

  // Returns true if this tracker has any reference to |port_id|.
  bool HasPort(int port_id) const {
    for (auto it : contexts_to_ports_) {
      if (it.second.count(port_id) > 0)
        return true;
    }
    return false;
  }

  // Deletes all references to |port_id|.
  void DeletePort(int port_id) {
    for (auto it = contexts_to_ports_.begin();
         it != contexts_to_ports_.end();) {
      if (it->second.erase(port_id) > 0 && it->second.empty())
        contexts_to_ports_.erase(it++);
      else
        ++it;
    }
  }

  // Gets every port ID that has a reference to |context|.
  std::set<int> GetPortsForContext(ScriptContext* context) const {
    auto ports = contexts_to_ports_.find(context);
    return ports == contexts_to_ports_.end() ? std::set<int>() : ports->second;
  }

 private:
  // Maps ScriptContexts to the port IDs that have a reference to it.
  std::map<ScriptContext*, std::set<int>> contexts_to_ports_;

  DISALLOW_COPY_AND_ASSIGN(PortTracker);
};

base::LazyInstance<PortTracker> g_port_tracker = LAZY_INSTANCE_INITIALIZER;

const char kPortClosedError[] = "Attempting to use a disconnected port object";
const char kReceivingEndDoesntExistError[] =
    "Could not establish connection. Receiving end does not exist.";

class ExtensionImpl : public ObjectBackedNativeHandler {
 public:
  ExtensionImpl(Dispatcher* dispatcher, ScriptContext* context)
      : ObjectBackedNativeHandler(context),
        dispatcher_(dispatcher),
        weak_ptr_factory_(this) {
    RouteFunction(
        "CloseChannel",
        base::Bind(&ExtensionImpl::CloseChannel, base::Unretained(this)));
    RouteFunction(
        "PortAddRef",
        base::Bind(&ExtensionImpl::PortAddRef, base::Unretained(this)));
    RouteFunction(
        "PortRelease",
        base::Bind(&ExtensionImpl::PortRelease, base::Unretained(this)));
    RouteFunction(
        "PostMessage",
        base::Bind(&ExtensionImpl::PostMessage, base::Unretained(this)));
    // TODO(fsamuel, kalman): Move BindToGC out of messaging natives.
    RouteFunction("BindToGC",
                  base::Bind(&ExtensionImpl::BindToGC, base::Unretained(this)));

    // Observe |context| so that port references to it can be cleared.
    context->AddInvalidationObserver(base::Bind(
        &ExtensionImpl::OnContextInvalidated, weak_ptr_factory_.GetWeakPtr()));
  }

  ~ExtensionImpl() override {}

 private:
  void OnContextInvalidated() {
    for (int port_id : g_port_tracker.Get().GetPortsForContext(context()))
      ReleasePort(port_id);
  }

  void ClearPortDataAndNotifyDispatcher(int port_id) {
    g_port_tracker.Get().DeletePort(port_id);
    dispatcher_->ClearPortData(port_id);
  }

  // Sends a message along the given channel.
  void PostMessage(const v8::FunctionCallbackInfo<v8::Value>& args) {
    content::RenderFrame* renderframe = context()->GetRenderFrame();
    if (!renderframe)
      return;

    // Arguments are (int32 port_id, string message).
    CHECK(args.Length() == 2 && args[0]->IsInt32() && args[1]->IsString());

    int port_id = args[0]->Int32Value();
    if (!g_port_tracker.Get().HasPort(port_id)) {
      args.GetIsolate()->ThrowException(v8::Exception::Error(
          v8::String::NewFromUtf8(args.GetIsolate(), kPortClosedError)));
      return;
    }

    renderframe->Send(new ExtensionHostMsg_PostMessage(
        renderframe->GetRoutingID(), port_id,
        Message(*v8::String::Utf8Value(args[1]),
                blink::WebUserGestureIndicator::isProcessingUserGesture())));
  }

  // Forcefully disconnects a port.
  void CloseChannel(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Arguments are (int32 port_id, boolean notify_browser).
    CHECK_EQ(2, args.Length());
    CHECK(args[0]->IsInt32());
    CHECK(args[1]->IsBoolean());

    int port_id = args[0]->Int32Value();
    if (!g_port_tracker.Get().HasPort(port_id))
      return;

    // Send via the RenderThread because the RenderFrame might be closing.
    bool notify_browser = args[1]->BooleanValue();
    if (notify_browser) {
      content::RenderThread::Get()->Send(
          new ExtensionHostMsg_CloseChannel(port_id, std::string()));
    }

    ClearPortDataAndNotifyDispatcher(port_id);
  }

  // A new port has been created for a context.  This occurs both when script
  // opens a connection, and when a connection is opened to this script.
  void PortAddRef(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Arguments are (int32 port_id).
    CHECK_EQ(1, args.Length());
    CHECK(args[0]->IsInt32());

    int port_id = args[0]->Int32Value();
    g_port_tracker.Get().AddReference(context(), port_id);
  }

  // The frame a port lived in has been destroyed.  When there are no more
  // frames with a reference to a given port, we will disconnect it and notify
  // the other end of the channel.
  void PortRelease(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Arguments are (int32 port_id).
    CHECK(args.Length() == 1 && args[0]->IsInt32());
    ReleasePort(args[0]->Int32Value());
  }

  // Releases the reference to |port_id| for this context, and clears all port
  // data if there are no more references.
  void ReleasePort(int port_id) {
    if (g_port_tracker.Get().RemoveReference(context(), port_id) &&
        !g_port_tracker.Get().HasPort(port_id)) {
      // Send via the RenderThread because the RenderFrame might be closing.
      content::RenderThread::Get()->Send(
          new ExtensionHostMsg_CloseChannel(port_id, std::string()));
      ClearPortDataAndNotifyDispatcher(port_id);
    }
  }

  // void BindToGC(object, callback, port_id)
  //
  // Binds |callback| to be invoked *sometime after* |object| is garbage
  // collected. We don't call the method re-entrantly so as to avoid executing
  // JS in some bizarro undefined mid-GC state, nor do we then call into the
  // script context if it's been invalidated.
  //
  // If the script context *is* invalidated in the meantime, as a slight hack,
  // release the port with ID |port_id| if it's >= 0.
  void BindToGC(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CHECK(args.Length() == 3 && args[0]->IsObject() && args[1]->IsFunction() &&
          args[2]->IsInt32());
    int port_id = args[2]->Int32Value();
    base::Closure fallback = base::Bind(&base::DoNothing);
    if (port_id >= 0) {
      fallback = base::Bind(&ExtensionImpl::ReleasePort,
                            weak_ptr_factory_.GetWeakPtr(), port_id);
    }
    // Destroys itself when the object is GC'd or context is invalidated.
    new GCCallback(context(), args[0].As<v8::Object>(),
                   args[1].As<v8::Function>(), fallback);
  }

  // Dispatcher handle. Not owned.
  Dispatcher* dispatcher_;

  base::WeakPtrFactory<ExtensionImpl> weak_ptr_factory_;
};

void DispatchOnConnectToScriptContext(
    int target_port_id,
    const std::string& channel_name,
    const ExtensionMsg_TabConnectionInfo* source,
    const ExtensionMsg_ExternalConnectionInfo& info,
    const std::string& tls_channel_id,
    bool* port_created,
    ScriptContext* script_context) {
  // Only dispatch the events if this is the requested target frame (0 = main
  // frame; positive = child frame).
  content::RenderFrame* renderframe = script_context->GetRenderFrame();
  if (info.target_frame_id == 0 && renderframe->GetWebFrame()->parent() != NULL)
    return;
  if (info.target_frame_id > 0 &&
      renderframe->GetRoutingID() != info.target_frame_id)
    return;
  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);

  scoped_ptr<V8ValueConverter> converter(V8ValueConverter::create());

  const std::string& source_url_spec = info.source_url.spec();
  std::string target_extension_id = script_context->GetExtensionID();
  const Extension* extension = script_context->extension();

  v8::Local<v8::Value> tab = v8::Null(isolate);
  v8::Local<v8::Value> tls_channel_id_value = v8::Undefined(isolate);
  v8::Local<v8::Value> guest_process_id = v8::Undefined(isolate);

  if (extension) {
    if (!source->tab.empty() && !extension->is_platform_app())
      tab = converter->ToV8Value(&source->tab, script_context->v8_context());

    ExternallyConnectableInfo* externally_connectable =
        ExternallyConnectableInfo::Get(extension);
    if (externally_connectable &&
        externally_connectable->accepts_tls_channel_id) {
      tls_channel_id_value = v8::String::NewFromUtf8(isolate,
                                                     tls_channel_id.c_str(),
                                                     v8::String::kNormalString,
                                                     tls_channel_id.size());
    }

    if (info.guest_process_id != content::ChildProcessHost::kInvalidUniqueID)
      guest_process_id = v8::Integer::New(isolate, info.guest_process_id);
  }

  v8::Local<v8::Value> arguments[] = {
      // portId
      v8::Integer::New(isolate, target_port_id),
      // channelName
      v8::String::NewFromUtf8(isolate, channel_name.c_str(),
                              v8::String::kNormalString, channel_name.size()),
      // sourceTab
      tab,
      // source_frame_id
      v8::Integer::New(isolate, source->frame_id),
      // guestProcessId
      guest_process_id,
      // sourceExtensionId
      v8::String::NewFromUtf8(isolate, info.source_id.c_str(),
                              v8::String::kNormalString, info.source_id.size()),
      // targetExtensionId
      v8::String::NewFromUtf8(isolate, target_extension_id.c_str(),
                              v8::String::kNormalString,
                              target_extension_id.size()),
      // sourceUrl
      v8::String::NewFromUtf8(isolate, source_url_spec.c_str(),
                              v8::String::kNormalString,
                              source_url_spec.size()),
      // tlsChannelId
      tls_channel_id_value,
  };

  v8::Local<v8::Value> retval =
      script_context->module_system()->CallModuleMethod(
          "messaging", "dispatchOnConnect", arraysize(arguments), arguments);

  if (!retval.IsEmpty()) {
    CHECK(retval->IsBoolean());
    *port_created |= retval->BooleanValue();
  } else {
    LOG(ERROR) << "Empty return value from dispatchOnConnect.";
  }
}

void DeliverMessageToScriptContext(const Message& message,
                                   int target_port_id,
                                   ScriptContext* script_context) {
  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);

  // Check to see whether the context has this port before bothering to create
  // the message.
  v8::Local<v8::Value> port_id_handle =
      v8::Integer::New(isolate, target_port_id);
  v8::Local<v8::Value> has_port =
      script_context->module_system()->CallModuleMethod("messaging", "hasPort",
                                                        1, &port_id_handle);

  CHECK(!has_port.IsEmpty());
  if (!has_port->BooleanValue())
    return;

  std::vector<v8::Local<v8::Value>> arguments;
  arguments.push_back(v8::String::NewFromUtf8(isolate,
                                              message.data.c_str(),
                                              v8::String::kNormalString,
                                              message.data.size()));
  arguments.push_back(port_id_handle);

  scoped_ptr<blink::WebScopedUserGesture> web_user_gesture;
  scoped_ptr<blink::WebScopedWindowFocusAllowedIndicator> allow_window_focus;
  if (message.user_gesture) {
    web_user_gesture.reset(new blink::WebScopedUserGesture);

    if (script_context->web_frame()) {
      blink::WebDocument document = script_context->web_frame()->document();
      allow_window_focus.reset(new blink::WebScopedWindowFocusAllowedIndicator(
          &document));
    }
  }

  script_context->module_system()->CallModuleMethod(
      "messaging", "dispatchOnMessage", &arguments);
}

void DispatchOnDisconnectToScriptContext(int port_id,
                                         const std::string& error_message,
                                         ScriptContext* script_context) {
  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);

  std::vector<v8::Local<v8::Value>> arguments;
  arguments.push_back(v8::Integer::New(isolate, port_id));
  if (!error_message.empty()) {
    arguments.push_back(
        v8::String::NewFromUtf8(isolate, error_message.c_str()));
  } else {
    arguments.push_back(v8::Null(isolate));
  }

  script_context->module_system()->CallModuleMethod(
      "messaging", "dispatchOnDisconnect", &arguments);
}

}  // namespace

ObjectBackedNativeHandler* MessagingBindings::Get(Dispatcher* dispatcher,
                                                  ScriptContext* context) {
  return new ExtensionImpl(dispatcher, context);
}

// static
void MessagingBindings::DispatchOnConnect(
    const ScriptContextSet& context_set,
    int target_port_id,
    const std::string& channel_name,
    const ExtensionMsg_TabConnectionInfo& source,
    const ExtensionMsg_ExternalConnectionInfo& info,
    const std::string& tls_channel_id,
    content::RenderFrame* restrict_to_render_frame) {
  // TODO(robwu): ScriptContextSet.ForEach should accept RenderFrame*.
  content::RenderView* restrict_to_render_view =
      restrict_to_render_frame ? restrict_to_render_frame->GetRenderView()
                               : NULL;
  bool port_created = false;
  context_set.ForEach(
      info.target_id, restrict_to_render_view,
      base::Bind(&DispatchOnConnectToScriptContext, target_port_id,
                 channel_name, &source, info, tls_channel_id, &port_created));

  // If we didn't create a port, notify the other end of the channel (treat it
  // as a disconnect).
  if (!port_created) {
    content::RenderThread::Get()->Send(new ExtensionHostMsg_CloseChannel(
        target_port_id, kReceivingEndDoesntExistError));
  }
}

// static
void MessagingBindings::DeliverMessage(
    const ScriptContextSet& context_set,
    int target_port_id,
    const Message& message,
    content::RenderFrame* restrict_to_render_frame) {
  // TODO(robwu): ScriptContextSet.ForEach should accept RenderFrame*.
  content::RenderView* restrict_to_render_view =
      restrict_to_render_frame ? restrict_to_render_frame->GetRenderView()
                               : NULL;
  context_set.ForEach(
      restrict_to_render_view,
      base::Bind(&DeliverMessageToScriptContext, message, target_port_id));
}

// static
void MessagingBindings::DispatchOnDisconnect(
    const ScriptContextSet& context_set,
    int port_id,
    const std::string& error_message,
    content::RenderFrame* restrict_to_render_frame) {
  // TODO(robwu): ScriptContextSet.ForEach should accept RenderFrame*.
  content::RenderView* restrict_to_render_view =
      restrict_to_render_frame ? restrict_to_render_frame->GetRenderView()
                               : NULL;
  context_set.ForEach(
      restrict_to_render_view,
      base::Bind(&DispatchOnDisconnectToScriptContext, port_id, error_message));
}

}  // namespace extensions
