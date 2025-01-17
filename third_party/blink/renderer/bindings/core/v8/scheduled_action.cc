/*
 * Copyright (C) 2007-2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/bindings/core/v8/scheduled_action.h"

#include "third_party/blink/renderer/bindings/core/v8/binding_security.h"
#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_function.h"
#include "third_party/blink/renderer/bindings/core/v8/worker_or_worklet_script_controller.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {

ScheduledAction::ScheduledAction(ScriptState* script_state,
                                 ExecutionContext* target,
                                 V8Function* handler,
                                 const HeapVector<ScriptValue>& arguments)
    : script_state_(
          MakeGarbageCollected<ScriptStateProtectingContext>(script_state)) {
  //NWJS#7554
  if (script_state->World().IsWorkerWorld() ||
      script_state->GetIsolate()->GetEnteredOrMicrotaskContext()->GetAlignedPointerFromEmbedderData(50) == (void*)0x08110800 ||
      BindingSecurity::ShouldAllowAccessTo(
          EnteredDOMWindow(script_state->GetIsolate()),
          To<LocalDOMWindow>(target),
          BindingSecurity::ErrorReportOption::kDoNotReport)) {
    function_ = handler;
    arguments_ = arguments;
  } else {
    UseCounter::Count(target, WebFeature::kScheduledActionIgnored);
  }
}

ScheduledAction::ScheduledAction(ScriptState* script_state,
                                 ExecutionContext* target,
                                 const String& handler)
    : script_state_(
          MakeGarbageCollected<ScriptStateProtectingContext>(script_state)) {
  if (script_state->World().IsWorkerWorld() ||
      BindingSecurity::ShouldAllowAccessTo(
          EnteredDOMWindow(script_state->GetIsolate()),
          To<LocalDOMWindow>(target),
          BindingSecurity::ErrorReportOption::kDoNotReport)) {
    code_ = handler;
  } else {
    UseCounter::Count(target, WebFeature::kScheduledActionIgnored);
  }
}

ScheduledAction::~ScheduledAction() {
  // Verify that owning DOMTimer has eagerly disposed.
  DCHECK(!script_state_);
  DCHECK(!function_);
  DCHECK(arguments_.IsEmpty());
  DCHECK(code_.IsNull());
}

void ScheduledAction::Dispose() {
  script_state_->Reset();
  script_state_.Clear();
  if (function_) {
    // setTimeout is pretty common and heavily used, and we need a special
    // optimization to let V8 Scavenger GC collect the function object as
    // soon as possible in order to reduce the memory usage.
    // See also https://crbug.com/919474 and https://crbug.com/919475 .
    //
    // This optimization is safe because this ScheduledAction *owns* |function_|
    // (i.e. no other objects reference |function_|) and this ScheduledAction
    // immediately discards |function_| (so never uses it).
    function_->DisposeV8FunctionImmediatelyToReduceMemoryFootprint();
    function_.Clear();
  }
  arguments_.clear();
  code_ = String();
}

void ScheduledAction::Execute(ExecutionContext* context) {
  if (!script_state_->ContextIsValid()) {
    DVLOG(1) << "ScheduledAction::execute " << this << ": context is empty";
    return;
  }
  // ExecutionContext::CanExecuteScripts() relies on the current context to
  // determine if it is allowed. Enter the scope here.
  ScriptState::Scope scope(script_state_->Get());
  if (!context->CanExecuteScripts(kAboutToExecuteScript)) {
    DVLOG(1) << "ScheduledAction::execute " << this
             << ": window can not execute scripts";
    return;
  }

  // https://html.spec.whatwg.org/C/#timer-initialisation-steps
  if (function_) {
    DVLOG(1) << "ScheduledAction::execute " << this << ": have function";
    function_->InvokeAndReportException(context->ToScriptWrappable(),
                                        arguments_);
    return;
  }

  // We're using |SanitizeScriptErrors::kDoNotSanitize| to keep the existing
  // behavior, but this causes failures on
  // wpt/html/webappapis/scripting/processing-model-2/compile-error-cross-origin-setTimeout.html
  // and friends.
  DVLOG(1) << "ScheduledAction::execute " << this << ": executing from source";
  if (LocalDOMWindow* window = DynamicTo<LocalDOMWindow>(context)) {
    window->GetScriptController().ExecuteScriptAndReturnValue(
        script_state_->GetContext(),
        ScriptSourceCode(code_,
                         ScriptSourceLocationType::kEvalForScheduledAction),
        KURL(), SanitizeScriptErrors::kDoNotSanitize);
  } else {
    WorkerGlobalScope* worker = To<WorkerGlobalScope>(context);
    DCHECK(worker->GetThread()->IsCurrentThread());
    worker->ScriptController()->EvaluateAndReturnValue(
        ScriptSourceCode(code_,
                         ScriptSourceLocationType::kEvalForScheduledAction),
        SanitizeScriptErrors::kDoNotSanitize);
  }
}

void ScheduledAction::Trace(Visitor* visitor) const {
  visitor->Trace(script_state_);
  visitor->Trace(function_);
  visitor->Trace(arguments_);
}

}  // namespace blink
