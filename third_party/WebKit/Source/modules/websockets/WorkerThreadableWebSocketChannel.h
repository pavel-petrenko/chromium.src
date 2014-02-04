/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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

#ifndef WorkerThreadableWebSocketChannel_h
#define WorkerThreadableWebSocketChannel_h

#include "core/frame/ConsoleTypes.h"
#include "core/workers/WorkerGlobalScope.h"
#include "modules/websockets/WebSocketChannel.h"
#include "modules/websockets/WebSocketChannelClient.h"

#include "wtf/PassOwnPtr.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Threading.h"
#include "wtf/Vector.h"
#include "wtf/WeakPtr.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class BlobDataHandle;
class KURL;
class ExecutionContext;
class ThreadableWebSocketChannelClientWrapper;
class WorkerGlobalScope;
class WorkerLoaderProxy;
class WorkerRunLoop;

class WorkerThreadableWebSocketChannel FINAL : public RefCounted<WorkerThreadableWebSocketChannel>, public WebSocketChannel {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<WebSocketChannel> create(WorkerGlobalScope* workerGlobalScope, WebSocketChannelClient* client, const String& taskMode, const String& sourceURL, unsigned lineNumber)
    {
        return adoptRef(new WorkerThreadableWebSocketChannel(workerGlobalScope, client, taskMode, sourceURL, lineNumber));
    }
    virtual ~WorkerThreadableWebSocketChannel();

    // WebSocketChannel functions.
    virtual void connect(const KURL&, const String& protocol) OVERRIDE;
    virtual String subprotocol() OVERRIDE;
    virtual String extensions() OVERRIDE;
    virtual WebSocketChannel::SendResult send(const String& message) OVERRIDE;
    virtual WebSocketChannel::SendResult send(const ArrayBuffer&, unsigned byteOffset, unsigned byteLength) OVERRIDE;
    virtual WebSocketChannel::SendResult send(PassRefPtr<BlobDataHandle>) OVERRIDE;
    virtual unsigned long bufferedAmount() const OVERRIDE;
    virtual void close(int code, const String& reason) OVERRIDE;
    virtual void fail(const String& reason, MessageLevel, const String&, unsigned) OVERRIDE;
    virtual void disconnect() OVERRIDE; // Will suppress didClose().
    virtual void suspend() OVERRIDE;
    virtual void resume() OVERRIDE;

    // Generated by the bridge. The Peer and its bridge should have identical
    // lifetimes. All methods of this class must be called on the main thread.
    class Peer FINAL : public WebSocketChannelClient {
        WTF_MAKE_NONCOPYABLE(Peer); WTF_MAKE_FAST_ALLOCATED;
    public:
        virtual ~Peer();

        // sourceURLAtConnection and lineNumberAtConnection parameters may
        // be shown when the connection fails.
        static void initialize(ExecutionContext*, PassRefPtr<WeakReference<Peer> >, WorkerLoaderProxy*, PassRefPtr<ThreadableWebSocketChannelClientWrapper>, const String& taskMode, const String& sourceURLAtConnection, unsigned lineNumberAtConnection);
        void destroy();

        void connect(const KURL&, const String& protocol);
        void send(const String& message);
        void sendArrayBuffer(PassOwnPtr<Vector<char> >);
        void sendBlob(PassRefPtr<BlobDataHandle>);
        void bufferedAmount();
        void close(int code, const String& reason);
        void fail(const String& reason, MessageLevel, const String& sourceURL, unsigned lineNumber);
        void disconnect();
        void suspend();
        void resume();

        // WebSocketChannelClient functions.
        virtual void didConnect() OVERRIDE;
        virtual void didReceiveMessage(const String& message) OVERRIDE;
        virtual void didReceiveBinaryData(PassOwnPtr<Vector<char> >) OVERRIDE;
        virtual void didUpdateBufferedAmount(unsigned long bufferedAmount) OVERRIDE;
        virtual void didStartClosingHandshake() OVERRIDE;
        virtual void didClose(unsigned long unhandledBufferedAmount, ClosingHandshakeCompletionStatus, unsigned short code, const String& reason) OVERRIDE;
        virtual void didReceiveMessageError() OVERRIDE;

    private:
        Peer(PassRefPtr<WeakReference<Peer> >, PassRefPtr<ThreadableWebSocketChannelClientWrapper>, WorkerLoaderProxy&, ExecutionContext*, const String& taskMode, const String& sourceURL, unsigned lineNumber);

        RefPtr<ThreadableWebSocketChannelClientWrapper> m_workerClientWrapper;
        WorkerLoaderProxy& m_loaderProxy;
        RefPtr<WebSocketChannel> m_mainWebSocketChannel;
        String m_taskMode;
        WeakPtrFactory<Peer> m_weakFactory;
    };

    using RefCounted<WorkerThreadableWebSocketChannel>::ref;
    using RefCounted<WorkerThreadableWebSocketChannel>::deref;

protected:
    // WebSocketChannel functions.
    virtual void refWebSocketChannel() OVERRIDE { ref(); }
    virtual void derefWebSocketChannel() OVERRIDE { deref(); }

private:
    // Bridge for Peer. Running on the worker thread.
    class Bridge : public RefCounted<Bridge> {
    public:
        static PassRefPtr<Bridge> create(PassRefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper, PassRefPtr<WorkerGlobalScope> workerGlobalScope, const String& taskMode)
        {
            return adoptRef(new Bridge(workerClientWrapper, workerGlobalScope, taskMode));
        }
        ~Bridge();
        // sourceURLAtConnection and lineNumberAtConnection parameters may
        // be shown when the connection fails.
        void initialize(const String& sourceURLAtConnection, unsigned lineNumberAtConnection);
        void connect(const KURL&, const String& protocol);
        WebSocketChannel::SendResult send(const String& message);
        WebSocketChannel::SendResult send(const ArrayBuffer&, unsigned byteOffset, unsigned byteLength);
        WebSocketChannel::SendResult send(PassRefPtr<BlobDataHandle>);
        unsigned long bufferedAmount();
        void close(int code, const String& reason);
        void fail(const String& reason, MessageLevel, const String& sourceURL, unsigned lineNumber);
        void disconnect();
        void suspend();
        void resume();

        using RefCounted<Bridge>::ref;
        using RefCounted<Bridge>::deref;

    private:
        Bridge(PassRefPtr<ThreadableWebSocketChannelClientWrapper>, PassRefPtr<WorkerGlobalScope>, const String& taskMode);

        static void setWebSocketChannel(ExecutionContext*, Bridge* thisPtr, Peer*, PassRefPtr<ThreadableWebSocketChannelClientWrapper>);

        // Executed on the worker context's thread.
        void clearClientWrapper();

        void setMethodNotCompleted();
        void waitForMethodCompletion();

        RefPtr<ThreadableWebSocketChannelClientWrapper> m_workerClientWrapper;
        RefPtr<WorkerGlobalScope> m_workerGlobalScope;
        WorkerLoaderProxy& m_loaderProxy;
        String m_taskMode;
        WeakPtr<Peer> m_peer;
    };

    WorkerThreadableWebSocketChannel(WorkerGlobalScope*, WebSocketChannelClient*, const String& taskMode, const String& sourceURL, unsigned lineNumber);

    RefPtr<WorkerGlobalScope> m_workerGlobalScope;
    RefPtr<ThreadableWebSocketChannelClientWrapper> m_workerClientWrapper;
    RefPtr<Bridge> m_bridge;
    String m_sourceURLAtConnection;
    unsigned m_lineNumberAtConnection;
};

} // namespace WebCore

#endif // WorkerThreadableWebSocketChannel_h
