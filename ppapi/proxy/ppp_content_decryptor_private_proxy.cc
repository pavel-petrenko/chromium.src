// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/ppp_content_decryptor_private_proxy.h"

#include "base/platform_file.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/proxy/content_decryptor_private_serializer.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/plugin_globals.h"
#include "ppapi/proxy/plugin_resource_tracker.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/ppb_buffer_proxy.h"
#include "ppapi/proxy/serialized_var.h"
#include "ppapi/shared_impl/var_tracker.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_buffer_api.h"
#include "ppapi/thunk/ppb_buffer_trusted_api.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/thunk.h"

using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_Buffer_API;
using ppapi::thunk::PPB_BufferTrusted_API;
using ppapi::thunk::PPB_Instance_API;

namespace ppapi {
namespace proxy {

namespace {

PP_Bool DescribeHostBufferResource(PP_Resource resource, uint32_t* size) {
  EnterResourceNoLock<PPB_Buffer_API> enter(resource, true);
  if (enter.failed())
    return PP_FALSE;
  return enter.object()->Describe(size);
}

// TODO(dmichael): Refactor so this handle sharing code is in one place.
PP_Bool ShareHostBufferResourceToPlugin(
    HostDispatcher* dispatcher,
    PP_Resource resource,
    base::SharedMemoryHandle* shared_mem_handle) {
  if (!dispatcher || resource == 0 || !shared_mem_handle)
    return PP_FALSE;
  EnterResourceNoLock<PPB_BufferTrusted_API> enter(resource, true);
  if (enter.failed())
    return PP_FALSE;
  int handle;
  int32_t result = enter.object()->GetSharedMemory(&handle);
  if (result != PP_OK)
    return PP_FALSE;
  base::PlatformFile platform_file =
  #if defined(OS_WIN)
      reinterpret_cast<HANDLE>(static_cast<intptr_t>(handle));
  #elif defined(OS_POSIX)
      handle;
  #else
  #error Not implemented.
  #endif

  *shared_mem_handle = dispatcher->ShareHandleWithRemote(platform_file, false);
  return PP_TRUE;
}

// SerializedVarReceiveInput will decrement the reference count, but we want
// to give the recipient a reference. This utility function takes care of that
// work for the message handlers defined below.
PP_Var ExtractReceivedVarAndAddRef(Dispatcher* dispatcher,
                                   SerializedVarReceiveInput* serialized_var) {
  PP_Var var = serialized_var->Get(dispatcher);
  PpapiGlobals::Get()->GetVarTracker()->AddRefVar(var);
  return var;
}

// Increments the reference count on |resource| to ensure that it remains valid
// until the plugin receives the resource within the asynchronous message sent
// from the proxy.  The plugin side takes ownership of that reference. Returns
// PP_TRUE when the reference is successfully added, PP_FALSE otherwise.
PP_Bool AddRefResourceForPlugin(HostDispatcher* dispatcher,
                                PP_Resource resource) {
  const PPB_Core* core = static_cast<const PPB_Core*>(
      dispatcher->local_get_interface()(PPB_CORE_INTERFACE));
  if (!core) {
    NOTREACHED();
    return PP_FALSE;
  }
  core->AddRefResource(resource);
  return PP_TRUE;
}

bool InitializePppDecryptorBuffer(PP_Instance instance,
                                  HostDispatcher* dispatcher,
                                  PP_Resource resource,
                                  PPPDecryptor_Buffer* buffer) {
  if (!buffer) {
    NOTREACHED();
    return false;
  }

  if (!AddRefResourceForPlugin(dispatcher, resource))
    return false;

  HostResource host_resource;
  host_resource.SetHostResource(instance, resource);

  uint32_t size = 0;
  if (DescribeHostBufferResource(resource, &size) == PP_FALSE)
    return false;

  base::SharedMemoryHandle handle;
  if (ShareHostBufferResourceToPlugin(dispatcher,
                                      resource,
                                      &handle) == PP_FALSE)
    return false;

  buffer->resource = host_resource;
  buffer->handle = handle;
  buffer->size = size;
  return true;
}

void GenerateKeyRequest(PP_Instance instance,
                        PP_Var key_system,
                        PP_Var init_data) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_GenerateKeyRequest(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          SerializedVarSendInput(dispatcher, key_system),
          SerializedVarSendInput(dispatcher, init_data)));
}

void AddKey(PP_Instance instance,
            PP_Var session_id,
            PP_Var key,
            PP_Var init_data) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_AddKey(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          SerializedVarSendInput(dispatcher, session_id),
          SerializedVarSendInput(dispatcher, key),
          SerializedVarSendInput(dispatcher, init_data)));
}

void CancelKeyRequest(PP_Instance instance, PP_Var session_id) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_CancelKeyRequest(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          SerializedVarSendInput(dispatcher, session_id)));
}

void Decrypt(PP_Instance instance,
             PP_Resource encrypted_block,
             const PP_EncryptedBlockInfo* encrypted_block_info) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  PPPDecryptor_Buffer buffer;
  if (!InitializePppDecryptorBuffer(instance,
                                    dispatcher,
                                    encrypted_block,
                                    &buffer)) {
    NOTREACHED();
    return;
  }

  std::string serialized_block_info;
  if (!SerializeBlockInfo(*encrypted_block_info, &serialized_block_info)) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_Decrypt(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          buffer,
          serialized_block_info));
}

void InitializeVideoDecoder(
    PP_Instance instance,
    const PP_VideoDecoderConfig* decoder_config,
    PP_Resource extra_data_buffer) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  std::string serialized_decoder_config;
  if (!SerializeBlockInfo(*decoder_config, &serialized_decoder_config)) {
    NOTREACHED();
    return;
  }

  PPPDecryptor_Buffer buffer;
  if (!InitializePppDecryptorBuffer(instance,
                                    dispatcher,
                                    extra_data_buffer,
                                    &buffer)) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_InitializeVideoDecoder(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          serialized_decoder_config,
          buffer));
}


void DeinitializeDecoder(PP_Instance instance,
                         PP_DecryptorStreamType decoder_type,
                         uint32_t request_id) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_DeinitializeDecoder(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          decoder_type,
          request_id));
}

void ResetDecoder(PP_Instance instance,
                  PP_DecryptorStreamType decoder_type,
                  uint32_t request_id) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_ResetDecoder(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          decoder_type,
          request_id));
}

void DecryptAndDecode(PP_Instance instance,
                      PP_DecryptorStreamType decoder_type,
                      PP_Resource encrypted_buffer,
                      const PP_EncryptedBlockInfo* encrypted_block_info) {
  HostDispatcher* dispatcher = HostDispatcher::GetForInstance(instance);
  if (!dispatcher) {
    NOTREACHED();
    return;
  }

  PPPDecryptor_Buffer buffer;
  if (!InitializePppDecryptorBuffer(instance,
                                    dispatcher,
                                    encrypted_buffer,
                                    &buffer)) {
    NOTREACHED();
    return;
  }

  std::string serialized_block_info;
  if (!SerializeBlockInfo(*encrypted_block_info, &serialized_block_info)) {
    NOTREACHED();
    return;
  }

  dispatcher->Send(
      new PpapiMsg_PPPContentDecryptor_DecryptAndDecode(
          API_ID_PPP_CONTENT_DECRYPTOR_PRIVATE,
          instance,
          decoder_type,
          buffer,
          serialized_block_info));
}

static const PPP_ContentDecryptor_Private content_decryptor_interface = {
  &GenerateKeyRequest,
  &AddKey,
  &CancelKeyRequest,
  &Decrypt,
  &InitializeVideoDecoder,
  &DeinitializeDecoder,
  &ResetDecoder,
  &DecryptAndDecode
};

}  // namespace

PPP_ContentDecryptor_Private_Proxy::PPP_ContentDecryptor_Private_Proxy(
    Dispatcher* dispatcher)
    : InterfaceProxy(dispatcher),
      ppp_decryptor_impl_(NULL) {
  if (dispatcher->IsPlugin()) {
    ppp_decryptor_impl_ = static_cast<const PPP_ContentDecryptor_Private*>(
        dispatcher->local_get_interface()(
            PPP_CONTENTDECRYPTOR_PRIVATE_INTERFACE));
  }
}

PPP_ContentDecryptor_Private_Proxy::~PPP_ContentDecryptor_Private_Proxy() {
}

// static
const PPP_ContentDecryptor_Private*
    PPP_ContentDecryptor_Private_Proxy::GetProxyInterface() {
  return &content_decryptor_interface;
}

bool PPP_ContentDecryptor_Private_Proxy::OnMessageReceived(
    const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PPP_ContentDecryptor_Private_Proxy, msg)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_GenerateKeyRequest,
                        OnMsgGenerateKeyRequest)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_AddKey,
                        OnMsgAddKey)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_CancelKeyRequest,
                        OnMsgCancelKeyRequest)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_Decrypt,
                        OnMsgDecrypt)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_InitializeVideoDecoder,
                        OnMsgInitializeVideoDecoder)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_DeinitializeDecoder,
                        OnMsgDeinitializeDecoder)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_ResetDecoder,
                        OnMsgResetDecoder)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPPContentDecryptor_DecryptAndDecode,
                        OnMsgDecryptAndDecode)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  DCHECK(handled);
  return handled;
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgGenerateKeyRequest(
    PP_Instance instance,
    SerializedVarReceiveInput key_system,
    SerializedVarReceiveInput init_data) {
  if (ppp_decryptor_impl_) {
    CallWhileUnlocked(ppp_decryptor_impl_->GenerateKeyRequest,
                      instance,
                      ExtractReceivedVarAndAddRef(dispatcher(), &key_system),
                      ExtractReceivedVarAndAddRef(dispatcher(), &init_data));
  }
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgAddKey(
    PP_Instance instance,
    SerializedVarReceiveInput session_id,
    SerializedVarReceiveInput key,
    SerializedVarReceiveInput init_data) {
  if (ppp_decryptor_impl_) {
    CallWhileUnlocked(ppp_decryptor_impl_->AddKey,
                      instance,
                      ExtractReceivedVarAndAddRef(dispatcher(), &session_id),
                      ExtractReceivedVarAndAddRef(dispatcher(), &key),
                      ExtractReceivedVarAndAddRef(dispatcher(), &init_data));
  }
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgCancelKeyRequest(
    PP_Instance instance,
    SerializedVarReceiveInput session_id) {
  if (ppp_decryptor_impl_) {
    CallWhileUnlocked(ppp_decryptor_impl_->CancelKeyRequest,
                      instance,
                      ExtractReceivedVarAndAddRef(dispatcher(), &session_id));
  }
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgDecrypt(
    PP_Instance instance,
    const PPPDecryptor_Buffer& encrypted_buffer,
    const std::string& serialized_block_info) {
  if (ppp_decryptor_impl_) {
    PP_Resource plugin_resource =
        PPB_Buffer_Proxy::AddProxyResource(encrypted_buffer.resource,
                                           encrypted_buffer.handle,
                                           encrypted_buffer.size);
    PP_EncryptedBlockInfo block_info;
    if (!DeserializeBlockInfo(serialized_block_info, &block_info))
      return;
    CallWhileUnlocked(ppp_decryptor_impl_->Decrypt,
                      instance,
                      plugin_resource,
                      const_cast<const PP_EncryptedBlockInfo*>(&block_info));
  }
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgInitializeVideoDecoder(
    PP_Instance instance,
    const std::string& serialized_decoder_config,
    const PPPDecryptor_Buffer& extra_data_buffer) {

  PP_VideoDecoderConfig decoder_config;
  if (!DeserializeBlockInfo(serialized_decoder_config, &decoder_config))
      return;

  if (ppp_decryptor_impl_) {
    PP_Resource plugin_resource = 0;
    if (extra_data_buffer.size > 0) {
      plugin_resource =
          PPB_Buffer_Proxy::AddProxyResource(extra_data_buffer.resource,
                                             extra_data_buffer.handle,
                                             extra_data_buffer.size);
    }

    CallWhileUnlocked(
        ppp_decryptor_impl_->InitializeVideoDecoder,
        instance,
        const_cast<const PP_VideoDecoderConfig*>(&decoder_config),
        plugin_resource);
  }
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgDeinitializeDecoder(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    uint32_t request_id) {
  if (ppp_decryptor_impl_) {
    CallWhileUnlocked(
        ppp_decryptor_impl_->DeinitializeDecoder,
        instance,
        decoder_type,
        request_id);
  }
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgResetDecoder(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    uint32_t request_id) {
  if (ppp_decryptor_impl_) {
    CallWhileUnlocked(
        ppp_decryptor_impl_->ResetDecoder,
        instance,
        decoder_type,
        request_id);
  }
}

void PPP_ContentDecryptor_Private_Proxy::OnMsgDecryptAndDecode(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    const PPPDecryptor_Buffer& encrypted_buffer,
    const std::string& serialized_block_info) {
  if (ppp_decryptor_impl_) {
    PP_Resource plugin_resource =
        PPB_Buffer_Proxy::AddProxyResource(encrypted_buffer.resource,
                                           encrypted_buffer.handle,
                                           encrypted_buffer.size);
    PP_EncryptedBlockInfo block_info;
    if (!DeserializeBlockInfo(serialized_block_info, &block_info))
      return;
    CallWhileUnlocked(
        ppp_decryptor_impl_->DecryptAndDecode,
        instance,
        decoder_type,
        plugin_resource,
        const_cast<const PP_EncryptedBlockInfo*>(&block_info));
  }
}

}  // namespace proxy
}  // namespace ppapi
