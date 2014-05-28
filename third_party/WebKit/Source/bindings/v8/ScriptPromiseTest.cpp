/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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

#include "config.h"
#include "bindings/v8/ScriptPromise.h"

#include "bindings/v8/ScriptFunction.h"
#include "bindings/v8/ScriptPromiseResolver.h"
#include "bindings/v8/ScriptValue.h"
#include "bindings/v8/V8Binding.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"

#include <gtest/gtest.h>
#include <v8.h>

namespace WebCore {

namespace {

void callback(const v8::FunctionCallbackInfo<v8::Value>& info) { }

class Function : public ScriptFunction {
public:
    static PassOwnPtr<Function> create(v8::Isolate* isolate, String* value)
    {
        return adoptPtr(new Function(isolate, value));
    }

    virtual ScriptValue call(ScriptValue value) OVERRIDE
    {
        ASSERT(!value.isEmpty());
        *m_value = toCoreString(value.v8Value()->ToString());
        return value;
    }

private:
    Function(v8::Isolate* isolate, String* value) : ScriptFunction(isolate), m_value(value) { }

    String* m_value;
};

class ScriptPromiseTest : public testing::Test {
public:
    ScriptPromiseTest()
        : m_scope(V8TestingScope::create(v8::Isolate::GetCurrent()))
    {
    }

    ~ScriptPromiseTest()
    {
        // FIXME: We put this statement here to clear an exception from the isolate.
        createClosure(callback, v8::Undefined(m_scope->isolate()), m_scope->isolate());

        // Execute all pending microtasks
        isolate()->RunMicrotasks();
    }

    ScriptState* scriptState() const { return m_scope->scriptState(); }
    v8::Isolate* isolate() const { return m_scope->isolate(); }

protected:
    OwnPtr<V8TestingScope> m_scope;
};

TEST_F(ScriptPromiseTest, constructFromNonPromise)
{
    v8::TryCatch trycatch;
    ScriptPromise promise(scriptState(), v8::Undefined(isolate()));
    ASSERT_TRUE(trycatch.HasCaught());
    ASSERT_TRUE(promise.isEmpty());
}

TEST_F(ScriptPromiseTest, thenResolve)
{
    RefPtr<ScriptPromiseResolver> resolver = ScriptPromiseResolver::create(scriptState());
    ScriptPromise promise = resolver->promise();
    String onFulfilled, onRejected;
    promise.then(Function::create(isolate(), &onFulfilled), Function::create(isolate(), &onRejected));

    ASSERT_FALSE(promise.isEmpty());
    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();
    resolver->resolve("hello");

    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();

    EXPECT_EQ("hello", onFulfilled);
    EXPECT_EQ(String(), onRejected);
}

TEST_F(ScriptPromiseTest, resolveThen)
{
    RefPtr<ScriptPromiseResolver> resolver = ScriptPromiseResolver::create(scriptState());
    ScriptPromise promise = resolver->promise();
    String onFulfilled, onRejected;
    resolver->resolve("hello");
    promise.then(Function::create(isolate(), &onFulfilled), Function::create(isolate(), &onRejected));

    ASSERT_FALSE(promise.isEmpty());
    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();

    EXPECT_EQ("hello", onFulfilled);
    EXPECT_EQ(String(), onRejected);
}

TEST_F(ScriptPromiseTest, thenReject)
{
    RefPtr<ScriptPromiseResolver> resolver = ScriptPromiseResolver::create(scriptState());
    ScriptPromise promise = resolver->promise();
    String onFulfilled, onRejected;
    promise.then(Function::create(isolate(), &onFulfilled), Function::create(isolate(), &onRejected));

    ASSERT_FALSE(promise.isEmpty());
    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();
    resolver->reject("hello");

    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();

    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ("hello", onRejected);
}

TEST_F(ScriptPromiseTest, rejectThen)
{
    RefPtr<ScriptPromiseResolver> resolver = ScriptPromiseResolver::create(scriptState());
    ScriptPromise promise = resolver->promise();
    String onFulfilled, onRejected;
    resolver->reject("hello");
    promise.then(Function::create(isolate(), &onFulfilled), Function::create(isolate(), &onRejected));

    ASSERT_FALSE(promise.isEmpty());
    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();

    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ("hello", onRejected);
}

TEST_F(ScriptPromiseTest, castPromise)
{
    ScriptPromise promise = ScriptPromiseResolver::create(scriptState())->promise();
    ScriptPromise newPromise = ScriptPromise::cast(scriptState(), promise.v8Value());

    ASSERT_FALSE(promise.isEmpty());
    EXPECT_EQ(promise.v8Value(), newPromise.v8Value());
}

TEST_F(ScriptPromiseTest, castNonPromise)
{
    String onFulfilled1, onFulfilled2, onRejected1, onRejected2;

    ScriptValue value = ScriptValue(scriptState(), v8String(isolate(), "hello"));
    ScriptPromise promise1 = ScriptPromise::cast(ScriptValue(value));
    ScriptPromise promise2 = ScriptPromise::cast(ScriptValue(value));
    promise1.then(Function::create(isolate(), &onFulfilled1), Function::create(isolate(), &onRejected1));
    promise2.then(Function::create(isolate(), &onFulfilled2), Function::create(isolate(), &onRejected2));

    ASSERT_FALSE(promise1.isEmpty());
    ASSERT_FALSE(promise2.isEmpty());
    EXPECT_NE(promise1.v8Value(), promise2.v8Value());

    ASSERT_TRUE(promise1.v8Value()->IsPromise());
    ASSERT_TRUE(promise2.v8Value()->IsPromise());

    EXPECT_EQ(String(), onFulfilled1);
    EXPECT_EQ(String(), onFulfilled2);
    EXPECT_EQ(String(), onRejected1);
    EXPECT_EQ(String(), onRejected2);

    isolate()->RunMicrotasks();

    EXPECT_EQ("hello", onFulfilled1);
    EXPECT_EQ("hello", onFulfilled2);
    EXPECT_EQ(String(), onRejected1);
    EXPECT_EQ(String(), onRejected2);
}

TEST_F(ScriptPromiseTest, reject)
{
    String onFulfilled, onRejected;

    ScriptValue value = ScriptValue(scriptState(), v8String(isolate(), "hello"));
    ScriptPromise promise = ScriptPromise::reject(ScriptValue(value));
    promise.then(Function::create(isolate(), &onFulfilled), Function::create(isolate(), &onRejected));

    ASSERT_FALSE(promise.isEmpty());
    ASSERT_TRUE(promise.v8Value()->IsPromise());

    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();

    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ("hello", onRejected);
}

TEST_F(ScriptPromiseTest, rejectWithExceptionState)
{
    String onFulfilled, onRejected;
    ScriptPromise promise = ScriptPromise::rejectWithDOMException(scriptState(), DOMException::create(SyntaxError, "some syntax error"));
    promise.then(Function::create(isolate(), &onFulfilled), Function::create(isolate(), &onRejected));

    ASSERT_FALSE(promise.isEmpty());
    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ(String(), onRejected);

    isolate()->RunMicrotasks();

    EXPECT_EQ(String(), onFulfilled);
    EXPECT_EQ("SyntaxError: some syntax error", onRejected);
}

} // namespace

} // namespace WebCore
