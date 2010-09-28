/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#if ENABLE(PLUGIN_PROCESS)

#include "WebProcessConnection.h"

#include "RunLoop.h"

namespace WebKit {

PassRefPtr<WebProcessConnection> WebProcessConnection::create(CoreIPC::Connection::Identifier connectionIdentifier)
{
    return adoptRef(new WebProcessConnection(connectionIdentifier));
}

WebProcessConnection::~WebProcessConnection()
{
}
    
WebProcessConnection::WebProcessConnection(CoreIPC::Connection::Identifier connectionIdentifier)
{
    m_connection = CoreIPC::Connection::createServerConnection(connectionIdentifier, this, RunLoop::main());
    m_connection->open();
}

void WebProcessConnection::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    // FIXME: Implement.
}

CoreIPC::SyncReplyMode WebProcessConnection::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments, CoreIPC::ArgumentEncoder* reply)
{
    return didReceiveSyncWebProcessConnectionMessage(connection, messageID, arguments, reply);
}

void WebProcessConnection::didClose(CoreIPC::Connection*)
{
    // FIXME: Implement.
}

void WebProcessConnection::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::MessageID)
{
    // FIXME: Implement.
}

void WebProcessConnection::createPlugin(uint64_t pluginInstanceID, const Plugin::Parameters&, bool& result)
{
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
