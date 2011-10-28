// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// IPC messages for injected Java objects. See JavaBridgeDispatcher for details.

// Multiply-included message file, hence no include guard.

#include "content/public/common/webkit_param_traits.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message_macros.h"

#define IPC_MESSAGE_START JavaBridgeMsgStart

// Messages for handling Java objects injected into JavaScript -----------------

// Sent from browser to renderer to initialize the Java Bridge.
IPC_MESSAGE_ROUTED1(JavaBridgeMsg_Init,
                    IPC::ChannelHandle) /* channel handle */

// Sent from browser to renderer to add a Java object with the given name.
IPC_MESSAGE_ROUTED2(JavaBridgeMsg_AddNamedObject,
                    string16 /* name */,
                    NPVariant_Param) /* object */

// Sent from browser to renderer to remove a Java object with the given name.
IPC_MESSAGE_ROUTED1(JavaBridgeMsg_RemoveNamedObject,
                    string16 /* name */)
