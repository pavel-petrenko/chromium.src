/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "heap/Handle.h"
#include "heap/Heap.h"
#include "heap/ThreadState.h"
#include "heap/Visitor.h"

#include <gtest/gtest.h>

namespace WebCore {

class TestGCScope {
public:
    explicit TestGCScope(ThreadState::StackState state)
        : m_state(ThreadState::current())
        , m_safePointScope(state)
    {
        m_state->checkThread();
        ASSERT(!m_state->isInGC());
        ThreadState::stopThreads();
        m_state->enterGC();
    }

    ~TestGCScope()
    {
        m_state->leaveGC();
        ASSERT(!m_state->isInGC());
        ThreadState::resumeThreads();
    }

private:
    ThreadState* m_state;
    ThreadState::SafePointScope m_safePointScope;
};

static void getHeapStats(HeapStats* stats)
{
    TestGCScope scope(ThreadState::NoHeapPointersOnStack);
    Heap::getStats(stats);
}

#define DEFINE_VISITOR_METHODS(Type)                                       \
    virtual void mark(const Type* object, TraceCallback callback) OVERRIDE \
    {                                                                      \
        if (object)                                                        \
            m_count++;                                                     \
    }                                                                      \
    virtual bool isMarked(const Type*) OVERRIDE { return false; }

class CountingVisitor : public Visitor {
public:
    CountingVisitor()
        : m_count(0)
    {
    }

    virtual void mark(const void* object, TraceCallback) OVERRIDE
    {
        if (object)
            m_count++;
    }

    virtual void mark(HeapObjectHeader* header, TraceCallback callback) OVERRIDE
    {
        ASSERT(header->payload());
        m_count++;
    }

    virtual void mark(FinalizedHeapObjectHeader* header, TraceCallback callback) OVERRIDE
    {
        ASSERT(header->payload());
        m_count++;
    }

    virtual void registerWeakMembers(const void*, WeakPointerCallback) OVERRIDE { }
    virtual bool isMarked(const void*) OVERRIDE { return false; }

    FOR_EACH_TYPED_HEAP(DEFINE_VISITOR_METHODS)

    size_t count() { return m_count; }
    void reset() { m_count = 0; }

private:
    size_t m_count;
};

class SimpleObject : public GarbageCollected<SimpleObject> {
    DECLARE_GC_INFO;
public:
    static SimpleObject* create() { return new SimpleObject(); }
    void trace(Visitor*) { }
    char getPayload(int i) { return payload[i]; }
private:
    SimpleObject() { }
    char payload[64];
};

#undef DEFINE_VISITOR_METHODS

class HeapTestSuperClass : public GarbageCollectedFinalized<HeapTestSuperClass> {
    DECLARE_GC_INFO
public:
    static HeapTestSuperClass* create()
    {
        return new HeapTestSuperClass();
    }

    virtual ~HeapTestSuperClass()
    {
        ++s_destructorCalls;
    }

    static int s_destructorCalls;
    void trace(Visitor*) { }

protected:
    HeapTestSuperClass() { }
};

int HeapTestSuperClass::s_destructorCalls = 0;

class HeapTestOtherSuperClass {
public:
    int payload;
};

static const size_t classMagic = 0xABCDDBCA;

class HeapTestSubClass : public HeapTestOtherSuperClass, public HeapTestSuperClass {
public:
    static HeapTestSubClass* create()
    {
        return new HeapTestSubClass();
    }

    virtual ~HeapTestSubClass()
    {
        EXPECT_EQ(classMagic, m_magic);
        ++s_destructorCalls;
    }

    static int s_destructorCalls;

private:

    HeapTestSubClass() : m_magic(classMagic) { }

    const size_t m_magic;
};

int HeapTestSubClass::s_destructorCalls = 0;

class HeapAllocatedArray : public GarbageCollected<HeapAllocatedArray> {
    DECLARE_GC_INFO
public:
    HeapAllocatedArray()
    {
        for (int i = 0; i < s_arraySize; ++i) {
            m_array[i] = i % 128;
        }
    }

    int8_t at(size_t i) { return m_array[i]; }
    void trace(Visitor*) { }
private:
    static const int s_arraySize = 1000;
    int8_t m_array[s_arraySize];
};

// Do several GCs to make sure that later GCs don't free up old memory from
// previously run tests in this process.
static void clearOutOldGarbage(HeapStats* heapStats)
{
    while (true) {
        getHeapStats(heapStats);
        size_t used = heapStats->totalObjectSpace();
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        getHeapStats(heapStats);
        if (heapStats->totalObjectSpace() >= used)
            break;
    }
}

class IntWrapper : public GarbageCollectedFinalized<IntWrapper> {
    DECLARE_GC_INFO
public:
    static IntWrapper* create(int x)
    {
        return new IntWrapper(x);
    }

    virtual ~IntWrapper()
    {
        ++s_destructorCalls;
    }

    static int s_destructorCalls;
    static void trace(Visitor*) { }

    int value() const { return m_x; }

    bool operator==(const IntWrapper& other) const { return other.value() == value(); }

    unsigned hash() { return IntHash<int>::hash(m_x); }

protected:
    IntWrapper(int x) : m_x(x) { }

private:
    IntWrapper();
    int m_x;
};

USED_FROM_MULTIPLE_THREADS(IntWrapper);

int IntWrapper::s_destructorCalls = 0;

class ThreadedHeapTester {
public:
    static void test()
    {
        ThreadedHeapTester* tester = new ThreadedHeapTester();
        for (int i = 0; i < numberOfThreads; i++)
            createThread(&threadFunc, tester, "testing thread");
        while (tester->m_threadsToFinish) {
            ThreadState::current()->safePoint(ThreadState::NoHeapPointersOnStack);
            yield();
        }
    }

private:
    static const int numberOfThreads = 10;
    static const int gcPerThread = 5;
    static const int numberOfAllocations = 50;

    inline bool done() const { return m_gcCount >= numberOfThreads * gcPerThread; }

    ThreadedHeapTester() : m_gcCount(0), m_threadsToFinish(numberOfThreads)
    {
    }

    static void threadFunc(void* data)
    {
        reinterpret_cast<ThreadedHeapTester*>(data)->runThread();
    }

    void runThread()
    {
        ThreadState::attach();

        int gcCount = 0;
        while (!done()) {
            ThreadState::current()->safePoint(ThreadState::NoHeapPointersOnStack);
            {
                IntWrapper* wrapper;

                for (int i = 0; i < numberOfAllocations; i++) {
                    wrapper = IntWrapper::create(0x0bbac0de);
                    if (!(i % 10))
                        ThreadState::current()->safePoint(ThreadState::HeapPointersOnStack);
                    yield();
                }

                if (gcCount < gcPerThread) {
                    Heap::collectGarbage(ThreadState::HeapPointersOnStack);
                    gcCount++;
                    atomicIncrement(&m_gcCount);
                }

                EXPECT_EQ(wrapper->value(), 0x0bbac0de);
            }
            yield();
        }
        ThreadState::detach();
        atomicDecrement(&m_threadsToFinish);
    }

    volatile int m_gcCount;
    volatile int m_threadsToFinish;
};

// The accounting for memory includes the memory used by rounding up object
// sizes. This is done in a different way on 32 bit and 64 bit, so we have to
// have some slack in the tests.
template<typename T>
void CheckWithSlack(T expected, T actual, int slack)
{
    EXPECT_LE(expected, actual);
    EXPECT_GE((intptr_t)expected + slack, (intptr_t)actual);
}

class TraceCounter : public GarbageCollectedFinalized<TraceCounter> {
    DECLARE_GC_INFO
public:
    static TraceCounter* create()
    {
        return new TraceCounter();
    }

    void trace(Visitor*) { m_traceCount++; }

    int traceCount() { return m_traceCount; }

private:
    TraceCounter()
        : m_traceCount(0)
    {
    }

    int m_traceCount;
};

class ClassWithMember : public GarbageCollected<ClassWithMember> {
    DECLARE_GC_INFO
public:
    static ClassWithMember* create()
    {
        return new ClassWithMember();
    }

    void trace(Visitor* visitor)
    {
        EXPECT_TRUE(visitor->isMarked(this));
        if (!traceCount())
            EXPECT_FALSE(visitor->isMarked(m_traceCounter));
        else
            EXPECT_TRUE(visitor->isMarked(m_traceCounter));

        visitor->trace(m_traceCounter);
    }

    int traceCount() { return m_traceCounter->traceCount(); }

private:
    ClassWithMember()
        : m_traceCounter(TraceCounter::create())
    { }

    Member<TraceCounter> m_traceCounter;
};

class SimpleFinalizedObject : public GarbageCollectedFinalized<SimpleFinalizedObject> {
    DECLARE_GC_INFO
public:
    static SimpleFinalizedObject* create()
    {
        return new SimpleFinalizedObject();
    }

    ~SimpleFinalizedObject()
    {
        ++s_destructorCalls;
    }

    static int s_destructorCalls;

    void trace(Visitor*) { }

private:
    SimpleFinalizedObject() { }
};

int SimpleFinalizedObject::s_destructorCalls = 0;

class TestTypedHeapClass : public GarbageCollected<TestTypedHeapClass> {
    DECLARE_GC_INFO
public:
    static TestTypedHeapClass* create()
    {
        return new TestTypedHeapClass();
    }

    void trace(Visitor*) { }

private:
    TestTypedHeapClass() { }
};

class Bar : public GarbageCollectedFinalized<Bar> {
    DECLARE_GC_INFO
public:
    static Bar* create()
    {
        return new Bar();
    }

    void finalize()
    {
        EXPECT_TRUE(m_magic == magic);
        m_magic = 0;
        s_live--;
    }

    virtual void trace(Visitor* visitor) { }
    static unsigned s_live;

protected:
    static const int magic = 1337;
    int m_magic;

    Bar()
        : m_magic(magic)
    {
        s_live++;
    }
};

unsigned Bar::s_live = 0;

class Baz : public GarbageCollected<Baz> {
    DECLARE_GC_INFO
public:
    static Baz* create(Bar* bar)
    {
        return new Baz(bar);
    }

    void trace(Visitor* visitor)
    {
        visitor->trace(m_bar);
    }

    void clear() { m_bar.release(); }

private:
    explicit Baz(Bar* bar)
        : m_bar(bar)
    {
    }

    Member<Bar> m_bar;
};

class Foo : public Bar {
public:
    static Foo* create(Bar* bar)
    {
        return new Foo(bar);
    }

    static Foo* create(Foo* foo)
    {
        return new Foo(foo);
    }

    virtual void trace(Visitor* visitor) OVERRIDE
    {
        if (m_pointsToFoo)
            visitor->mark(static_cast<Foo*>(m_bar));
        else
            visitor->mark(m_bar);
    }

private:
    Foo(Bar* bar)
        : Bar()
        , m_bar(bar)
        , m_pointsToFoo(false)
    {
    }

    Foo(Foo* foo)
        : Bar()
        , m_bar(foo)
        , m_pointsToFoo(true)
    {
    }

    Bar* m_bar;
    bool m_pointsToFoo;
};

class Bars : public Bar {
public:
    static Bars* create()
    {
        return new Bars();
    }

    virtual void trace(Visitor* visitor) OVERRIDE
    {
        for (unsigned i = 0; i < m_width; i++)
            visitor->trace(m_bars[i]);
    }

    unsigned getWidth() const
    {
        return m_width;
    }

    static const unsigned width = 7500;
private:
    Bars() : m_width(0)
    {
        for (unsigned i = 0; i < width; i++) {
            m_bars[i] = Bar::create();
            m_width++;
        }
    }

    unsigned m_width;
    Member<Bar> m_bars[width];
};

class ConstructorAllocation : public GarbageCollected<ConstructorAllocation> {
    DECLARE_GC_INFO
public:
    static ConstructorAllocation* create() { return new ConstructorAllocation(); }

    void trace(Visitor* visitor) { visitor->trace(m_intWrapper); }

private:
    ConstructorAllocation()
    {
        m_intWrapper = IntWrapper::create(42);
    }

    Member<IntWrapper> m_intWrapper;
};

class LargeObject : public GarbageCollectedFinalized<LargeObject> {
    DECLARE_GC_INFO
public:
    ~LargeObject()
    {
        s_destructorCalls++;
    }
    static LargeObject* create() { return new LargeObject(); }
    char get(size_t i) { return m_data[i]; }
    void set(size_t i, char c) { m_data[i] = c; }
    size_t length() { return s_length; }
    void trace(Visitor* visitor)
    {
        visitor->trace(m_intWrapper);
    }
    static int s_destructorCalls;

private:
    static const size_t s_length = 1024*1024;
    LargeObject()
    {
        m_intWrapper = IntWrapper::create(23);
    }
    Member<IntWrapper> m_intWrapper;
    char m_data[s_length];
};

int LargeObject::s_destructorCalls = 0;

class RefCountedAndGarbageCollected : public RefCountedGarbageCollected<RefCountedAndGarbageCollected> {
    DECLARE_GC_INFO
public:
    static PassRefPtr<RefCountedAndGarbageCollected> create()
    {
        return adoptRef(new RefCountedAndGarbageCollected());
    }

    ~RefCountedAndGarbageCollected()
    {
        ++s_destructorCalls;
    }

    void trace(Visitor*) { }

    static int s_destructorCalls;

private:
    RefCountedAndGarbageCollected()
    {
    }
};

int RefCountedAndGarbageCollected::s_destructorCalls = 0;

class RefCountedAndGarbageCollected2 : public HeapTestOtherSuperClass, public RefCountedGarbageCollected<RefCountedAndGarbageCollected2> {
    DECLARE_GC_INFO
public:
    static PassRefPtr<RefCountedAndGarbageCollected2> create()
    {
        return adoptRef(new RefCountedAndGarbageCollected2());
    }

    ~RefCountedAndGarbageCollected2()
    {
        ++s_destructorCalls;
    }

    void trace(Visitor*) { }

    static int s_destructorCalls;

private:
    RefCountedAndGarbageCollected2()
    {
    }
};

int RefCountedAndGarbageCollected2::s_destructorCalls = 0;

#define DEFINE_VISITOR_METHODS(Type)                                       \
    virtual void mark(const Type* object, TraceCallback callback) OVERRIDE \
    {                                                                      \
        mark(object);                                                      \
    }                                                                      \

class RefCountedGarbageCollectedVisitor : public CountingVisitor {
public:
    RefCountedGarbageCollectedVisitor(int expected, void** objects)
        : m_count(0)
        , m_expectedCount(expected)
        , m_expectedObjects(objects)
    {
    }

    void mark(const void* ptr) { markNoTrace(ptr); }

    virtual void markNoTrace(const void* ptr)
    {
        if (!ptr)
            return;
        if (m_count < m_expectedCount)
            EXPECT_TRUE(expectedObject(ptr));
        else
            EXPECT_FALSE(expectedObject(ptr));
        m_count++;
    }

    virtual void mark(const void* ptr, TraceCallback) OVERRIDE
    {
        mark(ptr);
    }

    virtual void mark(HeapObjectHeader* header, TraceCallback callback) OVERRIDE
    {
        mark(header->payload());
    }

    virtual void mark(FinalizedHeapObjectHeader* header, TraceCallback callback) OVERRIDE
    {
        mark(header->payload());
    }

    bool validate() { return m_count >= m_expectedCount; }
    void reset() { m_count = 0; }

    FOR_EACH_TYPED_HEAP(DEFINE_VISITOR_METHODS)

private:
    bool expectedObject(const void* ptr)
    {
        for (int i = 0; i < m_expectedCount; i++) {
            if (m_expectedObjects[i] == ptr)
                return true;
        }
        return false;
    }

    int m_count;
    int m_expectedCount;
    void** m_expectedObjects;
};

#undef DEFINE_VISITOR_METHODS

class Weak : public Bar {
public:
    static Weak* create(Bar* strong, Bar* weak)
    {
        return new Weak(strong, weak);
    }

    virtual void trace(Visitor* visitor) OVERRIDE
    {
        visitor->trace(m_strongBar);
        visitor->registerWeakMembers(this, zapWeakMembers);
    }

    static void zapWeakMembers(Visitor* visitor, void* self)
    {
        reinterpret_cast<Weak*>(self)->zapWeakMembers(visitor);
    }

    bool strongIsThere() { return !!m_strongBar; }
    bool weakIsThere() { return !!m_weakBar; }

private:
    Weak(Bar* strongBar, Bar* weakBar)
        : Bar()
        , m_strongBar(strongBar)
        , m_weakBar(weakBar)
    {
    }

    void zapWeakMembers(Visitor* visitor)
    {
        if (m_weakBar && !visitor->isAlive(m_weakBar))
            m_weakBar = 0;
    }

    Member<Bar> m_strongBar;
    Bar* m_weakBar;
};

class WithWeakMember : public Bar {
public:
    static WithWeakMember* create(Bar* strong, Bar* weak)
    {
        return new WithWeakMember(strong, weak);
    }

    virtual void trace(Visitor* visitor) OVERRIDE
    {
        visitor->trace(m_strongBar);
        visitor->trace(m_weakBar);
    }

    bool strongIsThere() { return !!m_strongBar; }
    bool weakIsThere() { return !!m_weakBar; }

private:
    WithWeakMember(Bar* strongBar, Bar* weakBar)
        : Bar()
        , m_strongBar(strongBar)
        , m_weakBar(weakBar)
    {
    }

    Member<Bar> m_strongBar;
    WeakMember<Bar> m_weakBar;
};

TEST(HeapTest, Threading)
{
    ThreadedHeapTester::test();
}

TEST(HeapTest, BasicFunctionality)
{
    HeapStats heapStats;
    clearOutOldGarbage(&heapStats);
    {
        size_t slack = 0;

        // When the test starts there may already have been leaked some memory
        // on the heap, so we establish a base line.
        size_t baseLevel = heapStats.totalObjectSpace();
        bool testPagesAllocated = !baseLevel;
        if (testPagesAllocated)
            EXPECT_EQ(heapStats.totalAllocatedSpace(), 0ul);

        // This allocates objects on the general heap which should add a page of memory.
        uint8_t* alloc32(Heap::allocate<uint8_t>(32));
        slack += 4;
        memset(alloc32, 40, 32);
        uint8_t* alloc64(Heap::allocate<uint8_t>(64));
        slack += 4;
        memset(alloc64, 27, 64);

        size_t total = 96;

        getHeapStats(&heapStats);
        CheckWithSlack(baseLevel + total, heapStats.totalObjectSpace(), slack);
        if (testPagesAllocated)
            EXPECT_EQ(heapStats.totalAllocatedSpace(), blinkPageSize);

        CheckWithSlack(alloc32 + 32 + sizeof(HeapObjectHeader), alloc64, slack);

        EXPECT_EQ(alloc32[0], 40);
        EXPECT_EQ(alloc32[31], 40);
        EXPECT_EQ(alloc64[0], 27);
        EXPECT_EQ(alloc64[63], 27);

        Heap::collectGarbage(ThreadState::HeapPointersOnStack);

        EXPECT_EQ(alloc32[0], 40);
        EXPECT_EQ(alloc32[31], 40);
        EXPECT_EQ(alloc64[0], 27);
        EXPECT_EQ(alloc64[63], 27);
    }

    clearOutOldGarbage(&heapStats);
    size_t total = 0;
    size_t slack = 0;
    size_t baseLevel = heapStats.totalObjectSpace();
    bool testPagesAllocated = !baseLevel;
    if (testPagesAllocated)
        EXPECT_EQ(heapStats.totalAllocatedSpace(), 0ul);

    const size_t big = 1008;
    Persistent<uint8_t> bigArea = Heap::allocate<uint8_t>(big);
    total += big;
    slack += 4;

    size_t persistentCount = 0;
    const size_t numPersistents = 100000;
    Persistent<uint8_t>* persistents[numPersistents];

    for (int i = 0; i < 1000; i++) {
        size_t size = 128 + i * 8;
        total += size;
        persistents[persistentCount++] = new Persistent<uint8_t>(Heap::allocate<uint8_t>(size));
        slack += 4;
        getHeapStats(&heapStats);
        CheckWithSlack(baseLevel + total, heapStats.totalObjectSpace(), slack);
        if (testPagesAllocated)
            EXPECT_EQ(0ul, heapStats.totalAllocatedSpace() & (blinkPageSize - 1));
    }

    {
        uint8_t* alloc32b(Heap::allocate<uint8_t>(32));
        slack += 4;
        memset(alloc32b, 40, 32);
        uint8_t* alloc64b(Heap::allocate<uint8_t>(64));
        slack += 4;
        memset(alloc64b, 27, 64);
        EXPECT_TRUE(alloc32b != alloc64b);

        total += 96;
        getHeapStats(&heapStats);
        CheckWithSlack(baseLevel + total, heapStats.totalObjectSpace(), slack);
        if (testPagesAllocated)
            EXPECT_EQ(0ul, heapStats.totalAllocatedSpace() & (blinkPageSize - 1));
    }

    clearOutOldGarbage(&heapStats);
    total -= 96;
    slack -= 8;
    if (testPagesAllocated)
        EXPECT_EQ(0ul, heapStats.totalAllocatedSpace() & (blinkPageSize - 1));

    Address bigAreaRaw = bigArea;
    // Clear the persistent, so that the big area will be garbage collected.
    bigArea.release();
    clearOutOldGarbage(&heapStats);

    total -= big;
    slack -= 4;
    getHeapStats(&heapStats);
    CheckWithSlack(baseLevel + total, heapStats.totalObjectSpace(), slack);
    if (testPagesAllocated)
        EXPECT_EQ(0ul, heapStats.totalAllocatedSpace() & (blinkPageSize - 1));

    // Endless loop unless we eventually get the memory back that we just freed.
    while (true) {
        Persistent<uint8_t>* alloc = new Persistent<uint8_t>(Heap::allocate<uint8_t>(big / 2));
        slack += 4;
        persistents[persistentCount++] = alloc;
        EXPECT_LT(persistentCount, numPersistents);
        total += big / 2;
        if (bigAreaRaw == alloc->get())
            break;
    }

    getHeapStats(&heapStats);
    CheckWithSlack(baseLevel + total, heapStats.totalObjectSpace(), slack);
    if (testPagesAllocated)
        EXPECT_EQ(0ul, heapStats.totalAllocatedSpace() & (blinkPageSize - 1));

    for (size_t i = 0; i < persistentCount; i++)
        persistents[i]->release();

    uint8_t* address = Heap::reallocate<uint8_t>(0, 100);
    for (int i = 0; i < 100; i++)
        address[i] = i;
    address = Heap::reallocate<uint8_t>(address, 100000);
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(address[i], i);
    address = Heap::reallocate<uint8_t>(address, 50);
    for (int i = 0; i < 50; i++)
        EXPECT_EQ(address[i], i);
    // This should be equivalent to free(address).
    EXPECT_EQ(reinterpret_cast<uintptr_t>(Heap::reallocate<uint8_t>(address, 0)), 0ul);
    // This should be equivalent to malloc(0).
    EXPECT_EQ(reinterpret_cast<uintptr_t>(Heap::reallocate<uint8_t>(0, 0)), 0ul);
}

TEST(HeapTest, SimpleAllocation)
{
    HeapStats initialHeapStats;
    clearOutOldGarbage(&initialHeapStats);
    EXPECT_EQ(0ul, initialHeapStats.totalObjectSpace());

    // Allocate an object in the heap.
    HeapAllocatedArray* array = new HeapAllocatedArray();
    HeapStats statsAfterAllocation;
    getHeapStats(&statsAfterAllocation);
    EXPECT_TRUE(statsAfterAllocation.totalObjectSpace() >= sizeof(HeapAllocatedArray));

    // Sanity check of the contents in the heap.
    EXPECT_EQ(0, array->at(0));
    EXPECT_EQ(42, array->at(42));
    EXPECT_EQ(0, array->at(128));
    EXPECT_EQ(999 % 128, array->at(999));
}

TEST(HeapTest, SimplePersistent)
{
    Persistent<TraceCounter> traceCounter = TraceCounter::create();
    EXPECT_EQ(0, traceCounter->traceCount());

    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(1, traceCounter->traceCount());

    Persistent<ClassWithMember> classWithMember = ClassWithMember::create();
    EXPECT_EQ(0, classWithMember->traceCount());

    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(1, classWithMember->traceCount());
    EXPECT_EQ(2, traceCounter->traceCount());
}

TEST(HeapTest, SimpleFinalization)
{
    {
        Persistent<SimpleFinalizedObject> finalized = SimpleFinalizedObject::create();
        EXPECT_EQ(0, SimpleFinalizedObject::s_destructorCalls);
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        EXPECT_EQ(0, SimpleFinalizedObject::s_destructorCalls);
    }

    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(1, SimpleFinalizedObject::s_destructorCalls);
}

TEST(HeapTest, Finalization)
{
    {
        HeapTestSubClass* t1 = HeapTestSubClass::create();
        HeapTestSubClass* t2 = HeapTestSubClass::create();
        HeapTestSuperClass* t3 = HeapTestSuperClass::create();
        // FIXME(oilpan): Ignore unused variables.
        (void)t1;
        (void)t2;
        (void)t3;
    }
    // Nothing is marked so the GC should free everything and call
    // the finalizer on all three objects.
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(2, HeapTestSubClass::s_destructorCalls);
    EXPECT_EQ(3, HeapTestSuperClass::s_destructorCalls);
    // Destructors not called again when GCing again.
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(2, HeapTestSubClass::s_destructorCalls);
    EXPECT_EQ(3, HeapTestSuperClass::s_destructorCalls);
}

TEST(HeapTest, TypedHeapSanity)
{
    // We use TraceCounter for allocating an object on the general heap.
    Persistent<TraceCounter> generalHeapObject = TraceCounter::create();
    Persistent<TestTypedHeapClass> typedHeapObject = TestTypedHeapClass::create();
    EXPECT_NE(pageHeaderAddress(reinterpret_cast<Address>(generalHeapObject.get())),
        pageHeaderAddress(reinterpret_cast<Address>(typedHeapObject.get())));
}

TEST(HeapTest, NoAllocation)
{
    EXPECT_TRUE(ThreadState::current()->isAllocationAllowed());
    {
        // Disallow allocation
        NoAllocationScope<AnyThread> noAllocationScope;
        EXPECT_FALSE(ThreadState::current()->isAllocationAllowed());
    }
    EXPECT_TRUE(ThreadState::current()->isAllocationAllowed());
}

TEST(HeapTest, Members)
{
    Bar::s_live = 0;
    {
        Persistent<Baz> h1;
        Persistent<Baz> h2;
        {
            h1 = Baz::create(Bar::create());
            Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
            EXPECT_EQ(1u, Bar::s_live);
            h2 = Baz::create(Bar::create());
            Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
            EXPECT_EQ(2u, Bar::s_live);
        }
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        EXPECT_EQ(2u, Bar::s_live);
        h1->clear();
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        EXPECT_EQ(1u, Bar::s_live);
    }
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(0u, Bar::s_live);
}

TEST(HeapTest, MarkTest)
{
    {
        Bar::s_live = 0;
        Persistent<Bar> bar = Bar::create();
        EXPECT_TRUE(ThreadState::current()->contains(bar));
        EXPECT_EQ(1u, Bar::s_live);
        {
            Foo* foo = Foo::create(bar);
            EXPECT_TRUE(ThreadState::current()->contains(foo));
            EXPECT_EQ(2u, Bar::s_live);
            EXPECT_TRUE(reinterpret_cast<Address>(foo) != reinterpret_cast<Address>(bar.get()));
            Heap::collectGarbage(ThreadState::HeapPointersOnStack);
            EXPECT_EQ(2u, Bar::s_live);
        }
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        EXPECT_EQ(1u, Bar::s_live);
    }
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(0u, Bar::s_live);
}

TEST(HeapTest, DeepTest)
{
    const unsigned depth = 100000;
    Bar::s_live = 0;
    {
        Bar* bar = Bar::create();
        EXPECT_TRUE(ThreadState::current()->contains(bar));
        Foo* foo = Foo::create(bar);
        EXPECT_TRUE(ThreadState::current()->contains(foo));
        EXPECT_EQ(2u, Bar::s_live);
        for (unsigned i = 0; i < depth; i++) {
            Foo* foo2 = Foo::create(foo);
            foo = foo2;
            EXPECT_TRUE(ThreadState::current()->contains(foo));
        }
        EXPECT_EQ(depth + 2, Bar::s_live);
        Heap::collectGarbage(ThreadState::HeapPointersOnStack);
        EXPECT_EQ(depth + 2, Bar::s_live);
    }
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(0u, Bar::s_live);
}

TEST(HeapTest, WideTest)
{
    Bar::s_live = 0;
    {
        Bars* bars = Bars::create();
        unsigned width = Bars::width;
        EXPECT_EQ(width + 1, Bar::s_live);
        Heap::collectGarbage(ThreadState::HeapPointersOnStack);
        EXPECT_EQ(width + 1, Bar::s_live);
        // Use bars here to make sure that it will be on the stack
        // for the conservative stack scan to find.
        EXPECT_EQ(width, bars->getWidth());
    }
    EXPECT_EQ(Bars::width + 1, Bar::s_live);
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(0u, Bar::s_live);
}

TEST(HeapTest, HashMapOfMembers)
{
    HeapStats initialHeapSize;
    IntWrapper::s_destructorCalls = 0;

    clearOutOldGarbage(&initialHeapSize);
    {
        typedef HashMap<
            Member<IntWrapper>,
            Member<IntWrapper>,
            DefaultHash<Member<IntWrapper> >::Hash,
            HashTraits<Member<IntWrapper> >,
            HashTraits<Member<IntWrapper> >,
            HeapAllocator> HeapObjectIdentityMap;

        HeapObjectIdentityMap* map(new HeapObjectIdentityMap());

        map->clear();
        HeapStats afterSetWasCreated;
        getHeapStats(&afterSetWasCreated);
        EXPECT_TRUE(afterSetWasCreated.totalObjectSpace() > initialHeapSize.totalObjectSpace());

        Heap::collectGarbage(ThreadState::HeapPointersOnStack);
        HeapStats afterGC;
        getHeapStats(&afterGC);
        EXPECT_EQ(afterGC.totalObjectSpace(), afterSetWasCreated.totalObjectSpace());

        IntWrapper* one(IntWrapper::create(1));
        IntWrapper* anotherOne(IntWrapper::create(1));
        map->add(one, one);
        HeapStats afterOneAdd;
        getHeapStats(&afterOneAdd);
        EXPECT_TRUE(afterOneAdd.totalObjectSpace() > afterGC.totalObjectSpace());

        HeapObjectIdentityMap::iterator it(map->begin());
        HeapObjectIdentityMap::iterator it2(map->begin());
        ++it;
        ++it2;

        map->add(anotherOne, one);

        EXPECT_EQ(map->size(), 2u); // Two different wrappings of '1' are distinct.

        Heap::collectGarbage(ThreadState::HeapPointersOnStack);
        EXPECT_TRUE(map->contains(one));
        EXPECT_TRUE(map->contains(anotherOne));

        IntWrapper* gotten(map->get(one));
        EXPECT_EQ(gotten->value(), one->value());
        EXPECT_EQ(gotten, one);

        HeapStats afterGC2;
        getHeapStats(&afterGC2);
        EXPECT_EQ(afterGC2.totalObjectSpace(), afterOneAdd.totalObjectSpace());

        IntWrapper* dozen = 0;

        for (int i = 1; i < 1000; i++) { // 999 iterations.
            IntWrapper* iWrapper(IntWrapper::create(i));
            IntWrapper* iSquared(IntWrapper::create(i * i));
            map->add(iWrapper, iSquared);
            if (i == 12)
                dozen = iWrapper;
        }
        HeapStats afterAdding1000;
        getHeapStats(&afterAdding1000);
        EXPECT_TRUE(afterAdding1000.totalObjectSpace() > afterGC2.totalObjectSpace());

        IntWrapper* gross(map->get(dozen));
        EXPECT_EQ(gross->value(), 144);

        // This should clear out junk created by all the adds.
        Heap::collectGarbage(ThreadState::HeapPointersOnStack);
        HeapStats afterGC3;
        getHeapStats(&afterGC3);
        EXPECT_TRUE(afterGC3.totalObjectSpace() < afterAdding1000.totalObjectSpace());
    }

    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    // The objects 'one', anotherOne, and the 999 other pairs.
    EXPECT_EQ(IntWrapper::s_destructorCalls, 2000);
    HeapStats afterGC4;
    getHeapStats(&afterGC4);
    EXPECT_EQ(afterGC4.totalObjectSpace(), initialHeapSize.totalObjectSpace());
}

TEST(HeapTest, NestedAllocation)
{
    HeapStats initialHeapSize;
    clearOutOldGarbage(&initialHeapSize);
    {
        Persistent<ConstructorAllocation> constructorAllocation = ConstructorAllocation::create();
    }
    HeapStats afterFree;
    clearOutOldGarbage(&afterFree);
    EXPECT_TRUE(initialHeapSize == afterFree);
}

TEST(HeapTest, LargeObjects)
{
    HeapStats initialHeapSize;
    clearOutOldGarbage(&initialHeapSize);
    IntWrapper::s_destructorCalls = 0;
    LargeObject::s_destructorCalls = 0;
    {
        int slack = 8; // LargeObject points to an IntWrapper that is also allocated.
        Persistent<LargeObject> object = LargeObject::create();
        HeapStats afterAllocation;
        clearOutOldGarbage(&afterAllocation);
        {
            object->set(0, 'a');
            EXPECT_EQ('a', object->get(0));
            object->set(object->length() - 1, 'b');
            EXPECT_EQ('b', object->get(object->length() - 1));
            size_t expectedObjectSpace = sizeof(LargeObject) + sizeof(IntWrapper);
            size_t actualObjectSpace =
                afterAllocation.totalObjectSpace() - initialHeapSize.totalObjectSpace();
            CheckWithSlack(expectedObjectSpace, actualObjectSpace, slack);
            // There is probably space for the IntWrapper in a heap page without
            // allocating extra pages. However, the IntWrapper allocation might cause
            // the addition of a heap page.
            size_t largeObjectAllocationSize =
                sizeof(LargeObject) + sizeof(LargeHeapObject<FinalizedHeapObjectHeader>) + sizeof(FinalizedHeapObjectHeader);
            size_t allocatedSpaceLowerBound =
                initialHeapSize.totalAllocatedSpace() + largeObjectAllocationSize;
            size_t allocatedSpaceUpperBound = allocatedSpaceLowerBound + slack + blinkPageSize;
            EXPECT_LE(allocatedSpaceLowerBound, afterAllocation.totalAllocatedSpace());
            EXPECT_LE(afterAllocation.totalAllocatedSpace(), allocatedSpaceUpperBound);
            EXPECT_EQ(0, IntWrapper::s_destructorCalls);
            EXPECT_EQ(0, LargeObject::s_destructorCalls);
            for (int i = 0; i < 10; i++)
                object = LargeObject::create();
        }
        HeapStats oneLargeObject;
        clearOutOldGarbage(&oneLargeObject);
        EXPECT_TRUE(oneLargeObject == afterAllocation);
        EXPECT_EQ(10, IntWrapper::s_destructorCalls);
        EXPECT_EQ(10, LargeObject::s_destructorCalls);
    }
    HeapStats backToInitial;
    clearOutOldGarbage(&backToInitial);
    EXPECT_TRUE(initialHeapSize == backToInitial);
    EXPECT_EQ(11, IntWrapper::s_destructorCalls);
    EXPECT_EQ(11, LargeObject::s_destructorCalls);
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
}

TEST(HeapTest, RefCountedGarbageCollected)
{
    RefCountedAndGarbageCollected::s_destructorCalls = 0;
    {
        RefPtr<RefCountedAndGarbageCollected> refPtr3;
        {
            Persistent<RefCountedAndGarbageCollected> persistent;
            {
                RefPtr<RefCountedAndGarbageCollected> refPtr1 = RefCountedAndGarbageCollected::create();
                RefPtr<RefCountedAndGarbageCollected> refPtr2 = RefCountedAndGarbageCollected::create();
                Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
                EXPECT_EQ(0, RefCountedAndGarbageCollected::s_destructorCalls);
                persistent = refPtr1.get();
            }
            // Reference count is zero for both objects but one of
            // them is kept alive by a persistent handle.
            Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
            EXPECT_EQ(1, RefCountedAndGarbageCollected::s_destructorCalls);
            refPtr3 = persistent;
        }
        // The persistent handle is gone but the ref count has been
        // increased to 1.
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        EXPECT_EQ(1, RefCountedAndGarbageCollected::s_destructorCalls);
    }
    // Both persistent handle is gone and ref count is zero so the
    // object can be collected.
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(2, RefCountedAndGarbageCollected::s_destructorCalls);
}

TEST(HeapTest, RefCountedGarbageCollectedWithStackPointers)
{
    RefCountedAndGarbageCollected::s_destructorCalls = 0;
    RefCountedAndGarbageCollected2::s_destructorCalls = 0;
    {
        RefCountedAndGarbageCollected* pointer1 = 0;
        RefCountedAndGarbageCollected* pointer2 = 0;
        {
            RefPtr<RefCountedAndGarbageCollected> object1 = RefCountedAndGarbageCollected::create();
            RefPtr<RefCountedAndGarbageCollected> object2 = RefCountedAndGarbageCollected::create();
            pointer1 = object1.get();
            pointer2 = object2.get();
            void* objects[2] = { object1.get(), object2.get() };
            RefCountedGarbageCollectedVisitor visitor(2, objects);
            ThreadState::current()->visitPersistents(&visitor);
            EXPECT_TRUE(visitor.validate());

            Heap::collectGarbage(ThreadState::HeapPointersOnStack);
            EXPECT_EQ(0, RefCountedAndGarbageCollected::s_destructorCalls);
        }
        Heap::collectGarbage(ThreadState::HeapPointersOnStack);
        EXPECT_EQ(0, RefCountedAndGarbageCollected::s_destructorCalls);

        // At this point, the reference counts of object1 and object2 are 0.
        // Only pointer1 and pointer2 keep references to object1 and object2.
        void* objects[] = { 0 };
        RefCountedGarbageCollectedVisitor visitor(0, objects);
        ThreadState::current()->visitPersistents(&visitor);
        EXPECT_TRUE(visitor.validate());

        {
            RefPtr<RefCountedAndGarbageCollected> object1(pointer1);
            RefPtr<RefCountedAndGarbageCollected> object2(pointer2);
            void* objects[2] = { object1.get(), object2.get() };
            RefCountedGarbageCollectedVisitor visitor(2, objects);
            ThreadState::current()->visitPersistents(&visitor);
            EXPECT_TRUE(visitor.validate());

            Heap::collectGarbage(ThreadState::HeapPointersOnStack);
            EXPECT_EQ(0, RefCountedAndGarbageCollected::s_destructorCalls);
        }

        Heap::collectGarbage(ThreadState::HeapPointersOnStack);
        EXPECT_EQ(0, RefCountedAndGarbageCollected::s_destructorCalls);
    }

    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(2, RefCountedAndGarbageCollected::s_destructorCalls);
}

TEST(HeapTest, WeakMembers)
{
    Bar::s_live = 0;
    {
        Persistent<Bar> h1 = Bar::create();
        Persistent<Weak> h4;
        Persistent<WithWeakMember> h5;
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        ASSERT_EQ(1u, Bar::s_live); // h1 is live.
        {
            Bar* h2 = Bar::create();
            Bar* h3 = Bar::create();
            h4 = Weak::create(h2, h3);
            h5 = WithWeakMember::create(h2, h3);
            Heap::collectGarbage(ThreadState::HeapPointersOnStack);
            EXPECT_EQ(5u, Bar::s_live); // The on-stack pointer keeps h3 alive.
            EXPECT_TRUE(h4->strongIsThere());
            EXPECT_TRUE(h4->weakIsThere());
            EXPECT_TRUE(h5->strongIsThere());
            EXPECT_TRUE(h5->weakIsThere());
        }
        // h3 is collected, weak pointers from h4 and h5 don't keep it alive.
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        EXPECT_EQ(4u, Bar::s_live);
        EXPECT_TRUE(h4->strongIsThere());
        EXPECT_FALSE(h4->weakIsThere()); // h3 is gone from weak pointer.
        EXPECT_TRUE(h5->strongIsThere());
        EXPECT_FALSE(h5->weakIsThere()); // h3 is gone from weak pointer.
        h1.release(); // Zero out h1.
        Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
        EXPECT_EQ(3u, Bar::s_live); // Only h4, h5 and h2 are left.
        EXPECT_TRUE(h4->strongIsThere()); // h2 is still pointed to from h4.
        EXPECT_TRUE(h5->strongIsThere()); // h2 is still pointed to from h5.
    }
    // h4 and h5 have gone out of scope now and they were keeping h2 alive.
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack);
    EXPECT_EQ(0u, Bar::s_live); // All gone.
}

TEST(HeapTest, Comparisons)
{
    Persistent<Bar> barPersistent = Bar::create();
    Persistent<Foo> fooPersistent = Foo::create(barPersistent);
    EXPECT_TRUE(barPersistent != fooPersistent);
    barPersistent = fooPersistent;
    EXPECT_TRUE(barPersistent == fooPersistent);
}

TEST(HeapTest, CheckAndMarkPointer)
{
    HeapStats initialHeapStats;
    clearOutOldGarbage(&initialHeapStats);

    Vector<Address> objectAddresses;
    Vector<Address> endAddresses;
    Address largeObjectAddress;
    Address largeObjectEndAddress;
    CountingVisitor visitor;
    for (int i = 0; i < 10; i++) {
        SimpleObject* object = SimpleObject::create();
        Address objectAddress = reinterpret_cast<Address>(object);
        objectAddresses.append(objectAddress);
        endAddresses.append(objectAddress + sizeof(SimpleObject) - 1);
    }
    LargeObject* largeObject = LargeObject::create();
    largeObjectAddress = reinterpret_cast<Address>(largeObject);
    largeObjectEndAddress = largeObjectAddress + sizeof(LargeObject) - 1;

    // This is a low-level test where we call checkAndMarkPointer. This method
    // causes the object start bitmap to be computed which requires the heap
    // to be in a consistent state (e.g. the free allocation area must be put
    // into a free list header). However when we call makeConsistentForGC it
    // also clears out the freelists so we have to rebuild those before trying
    // to allocate anything again. We do this by forcing a GC after doing the
    // checkAndMarkPointer tests.
    {
        TestGCScope scope(ThreadState::HeapPointersOnStack);
        Heap::makeConsistentForGC();
        for (size_t i = 0; i < objectAddresses.size(); i++) {
            EXPECT_TRUE(Heap::checkAndMarkPointer(&visitor, objectAddresses[i]));
            EXPECT_TRUE(Heap::checkAndMarkPointer(&visitor, endAddresses[i]));
        }
        EXPECT_EQ(objectAddresses.size() * 2, visitor.count());
        visitor.reset();
        EXPECT_TRUE(Heap::checkAndMarkPointer(&visitor, largeObjectAddress));
        EXPECT_TRUE(Heap::checkAndMarkPointer(&visitor, largeObjectEndAddress));
        EXPECT_EQ(2ul, visitor.count());
        visitor.reset();
    }
    // This forces a GC without stack scanning which results in the objects
    // being collected. This will also rebuild the above mentioned freelists,
    // however we don't rely on that below since we don't have any allocations.
    clearOutOldGarbage(&initialHeapStats);
    {
        TestGCScope scope(ThreadState::HeapPointersOnStack);
        Heap::makeConsistentForGC();
        for (size_t i = 0; i < objectAddresses.size(); i++) {
            EXPECT_FALSE(Heap::checkAndMarkPointer(&visitor, objectAddresses[i]));
            EXPECT_FALSE(Heap::checkAndMarkPointer(&visitor, endAddresses[i]));
        }
        EXPECT_EQ(0ul, visitor.count());
        EXPECT_FALSE(Heap::checkAndMarkPointer(&visitor, largeObjectAddress));
        EXPECT_FALSE(Heap::checkAndMarkPointer(&visitor, largeObjectEndAddress));
        EXPECT_EQ(0ul, visitor.count());
    }
    // This round of GC is important to make sure that the object start
    // bitmap are cleared out and that the free lists are rebuild.
    clearOutOldGarbage(&initialHeapStats);
}

DEFINE_GC_INFO(Bar);
DEFINE_GC_INFO(Baz);
DEFINE_GC_INFO(ClassWithMember);
DEFINE_GC_INFO(ConstructorAllocation);
DEFINE_GC_INFO(HeapAllocatedArray);
DEFINE_GC_INFO(HeapTestSuperClass);
DEFINE_GC_INFO(IntWrapper);
DEFINE_GC_INFO(LargeObject);
DEFINE_GC_INFO(RefCountedAndGarbageCollected);
DEFINE_GC_INFO(RefCountedAndGarbageCollected2);
DEFINE_GC_INFO(SimpleFinalizedObject);
DEFINE_GC_INFO(SimpleObject);
DEFINE_GC_INFO(TestTypedHeapClass);
DEFINE_GC_INFO(TraceCounter);

} // namespace
