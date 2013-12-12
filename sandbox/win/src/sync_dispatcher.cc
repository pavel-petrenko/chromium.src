// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/win/src/sync_dispatcher.h"

#include "base/win/windows_version.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/interception.h"
#include "sandbox/win/src/interceptors.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_broker.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sync_interception.h"
#include "sandbox/win/src/sync_policy.h"

namespace sandbox {

SyncDispatcher::SyncDispatcher(PolicyBase* policy_base)
    : policy_base_(policy_base) {
  static const IPCCall create_params = {
    {IPC_CREATEEVENT_TAG, WCHAR_TYPE, ULONG_TYPE, ULONG_TYPE},
    reinterpret_cast<CallbackGeneric>(&SyncDispatcher::CreateEvent)
  };

  static const IPCCall open_params = {
    {IPC_OPENEVENT_TAG, WCHAR_TYPE, ULONG_TYPE},
    reinterpret_cast<CallbackGeneric>(&SyncDispatcher::OpenEvent)
  };

  ipc_calls_.push_back(create_params);
  ipc_calls_.push_back(open_params);
}

bool SyncDispatcher::SetupService(InterceptionManager* manager,
                                  int service) {
  if (IPC_CREATEEVENT_TAG == service) {
    return INTERCEPT_NT(manager, NtCreateEvent, CREATE_EVENT_ID, 24);
  } else if (IPC_OPENEVENT_TAG == service) {
    return INTERCEPT_NT(manager, NtOpenEvent, OPEN_EVENT_ID, 16);
  }
  return false;
}

bool SyncDispatcher::CreateEvent(IPCInfo* ipc, base::string16* name,
                                 DWORD event_type, DWORD initial_state) {
  const wchar_t* event_name = name->c_str();
  CountedParameterSet<NameBased> params;
  params[NameBased::NAME] = ParamPickerMake(event_name);

  EvalResult result = policy_base_->EvalPolicy(IPC_CREATEEVENT_TAG,
                                               params.GetBase());
  HANDLE handle = NULL;
  DWORD ret = SyncPolicy::CreateEventAction(result, *ipc->client_info, *name,
                                            event_type, initial_state,
                                            &handle);
  // Return operation status on the IPC.
  ipc->return_info.nt_status = ret;
  ipc->return_info.handle = handle;
  return true;
}

bool SyncDispatcher::OpenEvent(IPCInfo* ipc, base::string16* name,
                               DWORD desired_access) {
  const wchar_t* event_name = name->c_str();

  CountedParameterSet<OpenEventParams> params;
  params[OpenEventParams::NAME] = ParamPickerMake(event_name);
  params[OpenEventParams::ACCESS] = ParamPickerMake(desired_access);

  EvalResult result = policy_base_->EvalPolicy(IPC_OPENEVENT_TAG,
                                               params.GetBase());
  HANDLE handle = NULL;
  DWORD ret = SyncPolicy::OpenEventAction(result, *ipc->client_info, *name,
                                          desired_access, &handle);
  // Return operation status on the IPC.
  ipc->return_info.win32_result = ret;
  ipc->return_info.handle = handle;
  return true;
}

}  // namespace sandbox
