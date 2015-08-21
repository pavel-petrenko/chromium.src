// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/wake_event_page.h"

#include "base/atomic_sequence_num.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/renderer/render_thread.h"
#include "extensions/common/extension_messages.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"

namespace extensions {

using namespace v8_helpers;

namespace {

base::LazyInstance<WakeEventPage> g_instance = LAZY_INSTANCE_INITIALIZER;

}  // namespace

class WakeEventPage::WakeEventPageNativeHandler
    : public ObjectBackedNativeHandler {
 public:
  // Handles own lifetime.
  WakeEventPageNativeHandler(ScriptContext* context,
                             const std::string& name,
                             const MakeRequestCallback& make_request)
      : ObjectBackedNativeHandler(context),
        make_request_(make_request),
        weak_ptr_factory_(this) {
    // Use Unretained not a WeakPtr because RouteFunction is tied to the
    // lifetime of this, so there is no way for DoWakeEventPage to be called
    // after destruction.
    RouteFunction(name, base::Bind(&WakeEventPageNativeHandler::DoWakeEventPage,
                                   base::Unretained(this)));
    // Delete self on invalidation. base::Unretained because by definition this
    // can't be deleted before it's deleted.
    context->AddInvalidationObserver(base::Bind(
        &WakeEventPageNativeHandler::DeleteSelf, base::Unretained(this)));
  };

  ~WakeEventPageNativeHandler() override {}

 private:
  void DeleteSelf() {
    Invalidate();
    delete this;
  }

  // Called by JavaScript with a single argument, the function to call when the
  // event page has been woken.
  void DoWakeEventPage(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CHECK_EQ(1, args.Length());
    CHECK(args[0]->IsFunction());
    v8::Global<v8::Function> callback(args.GetIsolate(),
                                      args[0].As<v8::Function>());

    const std::string& extension_id = context()->GetExtensionID();
    CHECK(!extension_id.empty());

    make_request_.Run(
        extension_id,
        base::Bind(&WakeEventPageNativeHandler::OnEventPageIsAwake,
                   weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)));
  }

  void OnEventPageIsAwake(v8::Global<v8::Function> callback, bool success) {
    v8::Isolate* isolate = context()->isolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Value> args[] = {
        v8::Boolean::New(isolate, success),
    };
    context()->CallFunction(v8::Local<v8::Function>::New(isolate, callback),
                            arraysize(args), args);
  }

  MakeRequestCallback make_request_;
  base::WeakPtrFactory<WakeEventPageNativeHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WakeEventPageNativeHandler);
};

// static
WakeEventPage* WakeEventPage::Get() {
  return g_instance.Pointer();
}

void WakeEventPage::Init(content::RenderThread* render_thread) {
  DCHECK(render_thread);
  DCHECK_EQ(content::RenderThread::Get(), render_thread);
  DCHECK(!message_filter_);

  message_filter_ = render_thread->GetSyncMessageFilter();
  render_thread->AddObserver(this);
}

v8::Local<v8::Function> WakeEventPage::GetForContext(ScriptContext* context) {
  DCHECK(message_filter_);

  v8::Isolate* isolate = context->isolate();
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Handle<v8::Context> v8_context = context->v8_context();
  v8::Context::Scope context_scope(v8_context);

  // Cache the imported function as a hidden property on the global object of
  // |v8_context|. Creating it isn't free.
  v8::Local<v8::String> kWakeEventPageKey =
      ToV8StringUnsafe(isolate, "WakeEventPage");
  v8::Local<v8::Value> wake_event_page =
      v8_context->Global()->GetHiddenValue(kWakeEventPageKey);

  if (wake_event_page.IsEmpty()) {
    // Implement this using a NativeHandler, which requires a function name
    // (arbitrary in this case). Handles own lifetime.
    const char* kFunctionName = "WakeEventPage";
    WakeEventPageNativeHandler* native_handler = new WakeEventPageNativeHandler(
        context, kFunctionName, base::Bind(&WakeEventPage::MakeRequest,
                                           weak_ptr_factory_.GetWeakPtr()));

    // Extract and cache the wake-event-page function from the native handler.
    wake_event_page = GetPropertyUnsafe(
        v8_context, native_handler->NewInstance(), kFunctionName);
    v8_context->Global()->SetHiddenValue(kWakeEventPageKey, wake_event_page);
  }

  CHECK(wake_event_page->IsFunction());
  return handle_scope.Escape(wake_event_page.As<v8::Function>());
}

WakeEventPage::RequestData::RequestData(const OnResponseCallback& on_response)
    : on_response(on_response) {}

WakeEventPage::RequestData::~RequestData() {}

WakeEventPage::WakeEventPage() : weak_ptr_factory_(this) {}

WakeEventPage::~WakeEventPage() {}

void WakeEventPage::MakeRequest(const std::string& extension_id,
                                const OnResponseCallback& on_response) {
  static base::AtomicSequenceNumber sequence_number;
  int request_id = sequence_number.GetNext();
  requests_.set(request_id, make_scoped_ptr(new RequestData(on_response)));
  message_filter_->Send(
      new ExtensionHostMsg_WakeEventPage(request_id, extension_id));
}

bool WakeEventPage::OnControlMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WakeEventPage, message)
    IPC_MESSAGE_HANDLER(ExtensionMsg_WakeEventPageResponse,
                        OnWakeEventPageResponse)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void WakeEventPage::OnWakeEventPageResponse(int request_id, bool success) {
  scoped_ptr<RequestData> request_data = requests_.take(request_id);
  CHECK(request_data);
  request_data->on_response.Run(success);
}

}  //  namespace extensions
