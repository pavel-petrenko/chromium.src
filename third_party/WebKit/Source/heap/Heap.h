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

#ifndef Heap_h
#define Heap_h

#include "heap/HeapExport.h"
#include "heap/ThreadState.h"
#include "heap/Visitor.h"

#include "wtf/Assertions.h"
#include "wtf/OwnPtr.h"

#include <stdint.h>

namespace WebCore {

// ASAN integration defintions
#if COMPILER(CLANG)
#define USE_ASAN (__has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__))
#else
#define USE_ASAN 0
#endif

#if USE_ASAN
extern "C" {
    // Marks memory region [addr, addr+size) as unaddressable.
    // This memory must be previously allocated by the user program. Accessing
    // addresses in this region from instrumented code is forbidden until
    // this region is unpoisoned. This function is not guaranteed to poison
    // the whole region - it may poison only subregion of [addr, addr+size) due
    // to ASan alignment restrictions.
    // Method is NOT thread-safe in the sense that no two threads can
    // (un)poison memory in the same memory region simultaneously.
    void __asan_poison_memory_region(void const volatile*, size_t);
    // Marks memory region [addr, addr+size) as addressable.
    // This memory must be previously allocated by the user program. Accessing
    // addresses in this region is allowed until this region is poisoned again.
    // This function may unpoison a superregion of [addr, addr+size) due to
    // ASan alignment restrictions.
    // Method is NOT thread-safe in the sense that no two threads can
    // (un)poison memory in the same memory region simultaneously.
    void __asan_unpoison_memory_region(void const volatile*, size_t);

    // User code should use macros instead of functions.
#define ASAN_POISON_MEMORY_REGION(addr, size)   \
    __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
    __asan_unpoison_memory_region((addr), (size))
#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
    const size_t asanMagic = 0xabefeed0;
    const size_t asanDeferMemoryReuseCount = 2;
    const size_t asanDeferMemoryReuseMask = 0x3;
}
#else
#define ASAN_POISON_MEMORY_REGION(addr, size)   \
    ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
    ((void)(addr), (void)(size))
#define NO_SANITIZE_ADDRESS
#endif

const size_t blinkPageSizeLog2 = 17;
const size_t blinkPageSize = 1 << blinkPageSizeLog2;
const size_t blinkPageOffsetMask = blinkPageSize - 1;
const size_t blinkPageBaseMask = ~blinkPageOffsetMask;
// Double precision floats are more efficient when 8 byte aligned, so we 8 byte
// align all allocations even on 32 bit.
const size_t allocationGranularity = 8;
const size_t allocationMask = allocationGranularity - 1;
const size_t objectStartBitMapSize = (blinkPageSize + ((8 * allocationGranularity) - 1)) / (8 * allocationGranularity);
const size_t reservedForObjectBitMap = ((objectStartBitMapSize + allocationMask) & ~allocationMask);
const size_t maxHeapObjectSize = 1 << 27;

const size_t markBitMask = 1;
const size_t freeListMask = 2;
const size_t debugBitMask = 4;
const size_t sizeMask = ~7;
const uint8_t freelistZapValue = 42;
const uint8_t finalizedZapValue = 24;

class HeapStats;
class PageMemory;

HEAP_EXPORT size_t osPageSize();

// Blink heap pages are set up with a guard page before and after the
// payload.
inline size_t blinkPagePayloadSize()
{
    return blinkPageSize - 2 * osPageSize();
}

// Blink heap pages are aligned to the Blink heap page size.
// Therefore, the start of a Blink page can be obtained by
// rounding down to the Blink page size.
inline Address roundToBlinkPageStart(Address address)
{
    return reinterpret_cast<Address>(reinterpret_cast<uintptr_t>(address) & blinkPageBaseMask);
}

// Compute the amount of padding we have to add to a header to make
// the size of the header plus the padding a multiple of 8 bytes.
template<typename Header>
inline size_t headerPadding()
{
    return (allocationGranularity - (sizeof(Header) % allocationGranularity)) % allocationGranularity;
}

// Masks an address down to the enclosing blink page base address.
inline Address blinkPageAddress(Address address)
{
    return reinterpret_cast<Address>(reinterpret_cast<uintptr_t>(address) & blinkPageBaseMask);
}

// Sanity check for a page header address: the address of the page
// header should be OS page size away from being Blink page size
// aligned.
inline bool isPageHeaderAddress(Address address)
{
    return !((reinterpret_cast<uintptr_t>(address) & blinkPageOffsetMask) - osPageSize());
}

// Mask an address down to the enclosing oilpan heap page base address.
// All oilpan heap pages are aligned at blinkPageBase plus an OS page size.
// FIXME: Remove HEAP_EXPORT once we get a proper public interface to our typed heaps.
// This is only exported to enable tests in HeapTest.cpp.
HEAP_EXPORT inline Address pageHeaderAddress(Address address)
{
    return blinkPageAddress(address) + osPageSize();
}

// Common header for heap pages.
class BaseHeapPage {
public:
    BaseHeapPage(PageMemory* storage, const GCInfo* gcInfo)
        : m_storage(storage)
        , m_gcInfo(gcInfo)
    {
        ASSERT(isPageHeaderAddress(reinterpret_cast<Address>(this)));
    }

    // Check if the given address could point to an object in this
    // heap page. If so, find the start of that object and mark it
    // using the given Visitor.
    //
    // Returns true if the object was found and marked, returns false
    // otherwise.
    //
    // This is used during conservative stack scanning to
    // conservatively mark all objects that could be referenced from
    // the stack.
    virtual bool checkAndMarkPointer(Visitor*, Address) = 0;

    Address address() { return reinterpret_cast<Address>(this); }
    PageMemory* storage() const { return m_storage; }
    const GCInfo* gcInfo() { return m_gcInfo; }

private:
    PageMemory* m_storage;
    const GCInfo* m_gcInfo;
};

// Large allocations are allocated as separate objects and linked in a
// list.
//
// In order to use the same memory allocation routines for everything
// allocated in the heap, large objects are considered heap pages
// containing only one object.
//
// The layout of a large heap object is as follows:
//
// | BaseHeapPage | next pointer | FinalizedHeapObjectHeader or HeapObjectHeader | payload |
template<typename Header>
class LargeHeapObject : public BaseHeapPage {
public:
    LargeHeapObject(PageMemory* storage, const GCInfo* gcInfo) : BaseHeapPage(storage, gcInfo)
    {
        COMPILE_ASSERT(!(sizeof(LargeHeapObject<Header>) & allocationMask), large_heap_object_header_misaligned);
    }

    virtual bool checkAndMarkPointer(Visitor*, Address);

    void link(LargeHeapObject<Header>** previousNext)
    {
        m_next = *previousNext;
        *previousNext = this;
    }

    void unlink(LargeHeapObject<Header>** previousNext)
    {
        *previousNext = m_next;
    }

    bool contains(Address object)
    {
        return (address() <= object) && (object <= (address() + size()));
    }

    LargeHeapObject<Header>* next()
    {
        return m_next;
    }

    size_t size()
    {
        return heapObjectHeader()->size() + sizeof(LargeHeapObject<Header>) + headerPadding<Header>();
    }

    Address payload() { return heapObjectHeader()->payload(); }
    size_t payloadSize() { return heapObjectHeader()->payloadSize(); }

    Header* heapObjectHeader()
    {
        Address headerAddress = address() + sizeof(LargeHeapObject<Header>) + headerPadding<Header>();
        return reinterpret_cast<Header*>(headerAddress);
    }

    bool isMarked();
    void unmark();
    void getStats(HeapStats&);
    void mark(Visitor*);
    void finalize();

private:
    friend class Heap;
    friend class ThreadHeap<Header>;

    LargeHeapObject<Header>* m_next;
};

// The BasicObjectHeader is the minimal object header. It is used when
// encountering heap space of size allocationGranularity to mark it as
// as freelist entry.
class BasicObjectHeader {
public:
    NO_SANITIZE_ADDRESS
    explicit BasicObjectHeader(size_t encodedSize)
        : m_size(encodedSize) { }

    static size_t freeListEncodedSize(size_t size) { return size | freeListMask; }

    NO_SANITIZE_ADDRESS
    bool isFree() { return m_size & freeListMask; }

    NO_SANITIZE_ADDRESS
    size_t size() const { return m_size & sizeMask; }

protected:
    size_t m_size;
};

// Our heap object layout is layered with the HeapObjectHeader closest
// to the payload, this can be wrapped in a FinalizedObjectHeader if the
// object is on the GeneralHeap and not on a specific TypedHeap.
// Finally if the object is a large object (> blinkPageSize/2) then it is
// wrapped with a LargeObjectHeader.
//
// Object memory layout:
// [ LargeObjectHeader | ] [ FinalizedObjectHeader | ] HeapObjectHeader | payload
// The [ ] notation denotes that the LargeObjectHeader and the FinalizedObjectHeader
// are independently optional.
class HeapObjectHeader : public BasicObjectHeader {
public:
    NO_SANITIZE_ADDRESS
    explicit HeapObjectHeader(size_t encodedSize)
        : BasicObjectHeader(encodedSize)
#ifndef NDEBUG
        , m_magic(magic)
#endif
    { }

    NO_SANITIZE_ADDRESS
    HeapObjectHeader(size_t encodedSize, const GCInfo*)
        : BasicObjectHeader(encodedSize)
#ifndef NDEBUG
        , m_magic(magic)
#endif
    { }

    inline void checkHeader() const;
    inline bool isMarked() const;

    inline void mark();
    inline void unmark();

    inline Address payload();
    inline size_t payloadSize();
    inline Address payloadEnd();

    inline void setDebugMark();
    inline void clearDebugMark();
    inline bool hasDebugMark() const;

    // Zap magic number with a new magic number that means there was once an
    // object allocated here, but it was freed because nobody marked it during
    // GC.
    void zapMagic();

    static void finalize(const GCInfo*, Address, size_t);
    static HeapObjectHeader* fromPayload(const void*);

    static const intptr_t magic = 0xc0de247;
    static const intptr_t zappedMagic = 0xC0DEdead;
    // The zap value for vtables should be < 4K to ensure it cannot be
    // used for dispatch.
    static const intptr_t zappedVTable = 0xd0d;

private:
#ifndef NDEBUG
    intptr_t m_magic;
#endif
};

const size_t objectHeaderSize = sizeof(HeapObjectHeader);

// Each object on the GeneralHeap needs to carry a pointer to its
// own GCInfo structure for tracing and potential finalization.
class FinalizedHeapObjectHeader : public HeapObjectHeader {
public:
    NO_SANITIZE_ADDRESS
    FinalizedHeapObjectHeader(size_t encodedSize, const GCInfo* gcInfo)
        : HeapObjectHeader(encodedSize)
        , m_gcInfo(gcInfo)
    {
    }

    inline Address payload();
    inline size_t payloadSize();

    NO_SANITIZE_ADDRESS
    const GCInfo* gcInfo() { return m_gcInfo; }

    NO_SANITIZE_ADDRESS
    const char* typeMarker() { return m_gcInfo->m_typeMarker; }

    NO_SANITIZE_ADDRESS
    TraceCallback traceCallback() { return m_gcInfo->m_trace; }

    void finalize();

    NO_SANITIZE_ADDRESS
    inline bool hasFinalizer() { return m_gcInfo->hasFinalizer(); }

    static FinalizedHeapObjectHeader* fromPayload(const void*);

private:
    const GCInfo* m_gcInfo;
};

const size_t finalizedHeaderSize = sizeof(FinalizedHeapObjectHeader);

class FreeListEntry : public HeapObjectHeader {
public:
    NO_SANITIZE_ADDRESS
    explicit FreeListEntry(size_t size)
        : HeapObjectHeader(freeListEncodedSize(size))
        , m_next(0)
    {
#if !defined(NDEBUG) && !ASAN
        // Zap free area with asterisks, aka 0x2a2a2a2a.
        // For ASAN don't zap since we keep accounting in the freelist entry.
        for (size_t i = sizeof(*this); i < size; i++)
            reinterpret_cast<Address>(this)[i] = freelistZapValue;
        ASSERT(size >= objectHeaderSize);
        zapMagic();
#endif
    }

    Address address() { return reinterpret_cast<Address>(this); }

    NO_SANITIZE_ADDRESS
    void unlink(FreeListEntry** prevNext)
    {
        *prevNext = m_next;
        m_next = 0;
    }

    NO_SANITIZE_ADDRESS
    void link(FreeListEntry** prevNext)
    {
        m_next = *prevNext;
        *prevNext = this;
    }

#if USE_ASAN
    NO_SANITIZE_ADDRESS
    bool shouldAddToFreeList()
    {
        // Init if not already magic.
        if ((m_asanMagic & ~asanDeferMemoryReuseMask) != asanMagic) {
            m_asanMagic = asanMagic | asanDeferMemoryReuseCount;
            return false;
        }
        // Decrement if count part of asanMagic > 0.
        if (m_asanMagic & asanDeferMemoryReuseMask)
            m_asanMagic--;
        return !(m_asanMagic & asanDeferMemoryReuseMask);
    }
#endif

private:
    FreeListEntry* m_next;
#if USE_ASAN
    unsigned m_asanMagic;
#endif
};

// Representation of Blink heap pages.
//
// Pages are specialized on the type of header on the object they
// contain. If a heap page only contains a certain type of object all
// of the objects will have the same GCInfo pointer and therefore that
// pointer can be stored in the HeapPage instead of in the header of
// each object. In that case objects have only a HeapObjectHeader and
// not a FinalizedHeapObjectHeader saving a word per object.
template<typename Header>
class HeapPage : public BaseHeapPage {
public:
    HeapPage(PageMemory*, ThreadHeap<Header>*, const GCInfo*);

    void link(HeapPage**);
    static void unlink(HeapPage*, HeapPage**);

    bool isEmpty();

    bool contains(Address addr)
    {
        Address blinkPageStart = roundToBlinkPageStart(address());
        return blinkPageStart <= addr && (blinkPageStart + blinkPageSize) > addr;
    }

    HeapPage* next() { return m_next; }

    Address payload()
    {
        return address() + sizeof(*this) + headerPadding<Header>();
    }

    static size_t payloadSize()
    {
        return (blinkPagePayloadSize() - sizeof(HeapPage) - headerPadding<Header>()) & ~allocationMask;
    }

    Address end() { return payload() + payloadSize(); }

    void getStats(HeapStats&);
    void clearMarks();
    void sweep();
    void clearObjectStartBitMap();
    void finalize(Header*);
    virtual bool checkAndMarkPointer(Visitor*, Address);
    ThreadHeap<Header>* heap() { return m_heap; }
#if USE_ASAN
    void poisonUnmarkedObjects();
#endif

protected:
    void populateObjectStartBitMap();
    bool isObjectStartBitMapComputed() { return m_objectStartBitMapComputed; }
    TraceCallback traceCallback(Header*);

    HeapPage<Header>* m_next;
    ThreadHeap<Header>* m_heap;
    bool m_objectStartBitMapComputed;
    uint8_t m_objectStartBitMap[reservedForObjectBitMap];

    friend class ThreadHeap<Header>;
};

// A HeapContainsCache provides a fast way of taking an arbitrary
// pointer-sized word, and determining whether it can be interpreted
// as a pointer to an area that is managed by the garbage collected
// Blink heap. There is a cache of 'pages' that have previously been
// determined to be either wholly inside or wholly outside the
// heap. The size of these pages must be smaller than the allocation
// alignment of the heap pages. We determine on-heap-ness by rounding
// down the pointer to the nearest page and looking up the page in the
// cache. If there is a miss in the cache we ask the heap to determine
// the status of the pointer by iterating over all of the heap. The
// result is then cached in the two-way associative page cache.
//
// A HeapContainsCache is both a positive and negative
// cache. Therefore, it must be flushed both when new memory is added
// and when memory is removed from the Blink heap.
class HeapContainsCache {
public:
    HeapContainsCache();

    void flush();
    bool contains(Address);

    // Perform a lookup in the cache.
    //
    // If lookup returns false the argument address was not found in
    // the cache and it is unknown if the address is in the Blink
    // heap.
    //
    // If lookup returns true the argument address was found in the
    // cache. In that case, the address is in the heap if the base
    // heap page out parameter is different from 0 and is not in the
    // heap if the base heap page out parameter is 0.
    bool lookup(Address, BaseHeapPage**);

    // Add an entry to the cache. Use a 0 base heap page pointer to
    // add a negative entry.
    void addEntry(Address, BaseHeapPage*);

private:
    class Entry {
    public:
        Entry()
            : m_address(0)
            , m_containingPage(0)
        {
        }

        Entry(Address address, BaseHeapPage* containingPage)
            : m_address(address)
            , m_containingPage(containingPage)
        {
        }

        BaseHeapPage* containingPage() { return m_containingPage; }
        Address address() { return m_address; }

    private:
        Address m_address;
        BaseHeapPage* m_containingPage;
    };

    static const int numberOfEntriesLog2 = 12;
    static const int numberOfEntries = 1 << numberOfEntriesLog2;

    static int hash(Address);

    WTF::OwnPtr<HeapContainsCache::Entry[]> m_entries;

    friend class ThreadState;
};

typedef void (*CallbackTrampoline)(VisitorCallback, Visitor*, void*);

// The CallbackStack contains all the visitor callbacks used to trace and mark
// objects. A specific CallbackStack instance contains at most bufferSize elements.
// If more space is needed a new CallbackStack instance is created and chained
// together with the former instance. I.e. a logical CallbackStack can be made of
// multiple chained CallbackStack object instances.
// There are two logical callback stacks. One containing all the marking callbacks and
// one containing the weak pointer callbacks.
class CallbackStack {
public:
    CallbackStack(CallbackStack** first)
        : m_limit(&(m_buffer[bufferSize]))
        , m_current(&(m_buffer[0]))
        , m_next(*first)
    {
#ifndef NDEBUG
        clearUnused();
#endif
        *first = this;
    }

    ~CallbackStack();
    void clearUnused();

    void assertIsEmpty();

    class Item {
    public:
        Item() { }
        Item(void* object, VisitorCallback callback)
            : m_object(object)
            , m_callback(callback)
        {
        }
        void* object() { return m_object; }
        VisitorCallback callback() { return m_callback; }

    private:
        void* m_object;
        VisitorCallback m_callback;
    };

    static void init(CallbackStack** first);
    static void shutdown(CallbackStack** first);
    bool popAndInvokeCallback(CallbackStack** first, Visitor*, CallbackTrampoline);

    Item* allocateEntry(CallbackStack** first)
    {
        if (m_current < m_limit)
            return m_current++;
        return (new CallbackStack(first))->allocateEntry(first);
    }

private:
    static const int bufferSize = 8000;
    Item m_buffer[bufferSize];
    Item* m_limit;
    Item* m_current;
    CallbackStack* m_next;
};

// Non-template super class used to pass a heap around to other classes.
class BaseHeap {
public:
    virtual ~BaseHeap() { }

    // Find the page in this thread heap containing the given
    // address. Returns 0 if the address is not contained in any
    // page in this thread heap.
    virtual BaseHeapPage* heapPageFromAddress(Address) = 0;

    // Find the large object in this thread heap containing the given
    // address. Returns 0 if the address is not contained in any
    // page in this thread heap.
    virtual BaseHeapPage* largeHeapObjectFromAddress(Address) = 0;

    // Check if the given address could point to an object in this
    // heap. If so, find the start of that object and mark it using
    // the given Visitor.
    //
    // Returns true if the object was found and marked, returns false
    // otherwise.
    //
    // This is used during conservative stack scanning to
    // conservatively mark all objects that could be referenced from
    // the stack.
    virtual bool checkAndMarkLargeHeapObject(Visitor*, Address) = 0;

    // Sweep this part of the Blink heap. This finalizes dead objects
    // and builds freelists for all the unused memory.
    virtual void sweep() = 0;

    // Forcefully finalize all objects in this part of the Blink heap
    // (potentially with the exception of one object). This is used
    // during thread termination to make sure that all objects for the
    // dying thread are finalized.
    virtual void finalizeAll(const void* except = 0) = 0;
    virtual bool inFinalizeAll() = 0;

    virtual void clearFreeLists() = 0;
    virtual void clearMarks() = 0;
#ifndef NDEBUG
    virtual void getScannedStats(HeapStats&) = 0;
#endif

    virtual void makeConsistentForGC() = 0;
    virtual bool isConsistentForGC() = 0;

    // Returns a bucket number for inserting a FreeListEntry of a
    // given size. All FreeListEntries in the given bucket, n, have
    // size >= 2^n.
    static int bucketIndexForSize(size_t);
};

// Thread heaps represent a part of the per-thread Blink heap.
//
// Each Blink thread has a number of thread heaps: one general heap
// that contains any type of object and a number of heaps specialized
// for specific object types (such as Node).
//
// Each thread heap contains the functionality to allocate new objects
// (potentially adding new pages to the heap), to find and mark
// objects during conservative stack scanning and to sweep the set of
// pages after a GC.
template<typename Header>
class ThreadHeap : public BaseHeap {
public:
    ThreadHeap(ThreadState*);
    virtual ~ThreadHeap();

    virtual BaseHeapPage* heapPageFromAddress(Address);
    virtual BaseHeapPage* largeHeapObjectFromAddress(Address);
    virtual bool checkAndMarkLargeHeapObject(Visitor*, Address);
    virtual void sweep();
    virtual void finalizeAll(const void* except = 0);
    virtual bool inFinalizeAll() { return m_inFinalizeAll; }
    virtual void clearFreeLists();
    virtual void clearMarks();
#ifndef NDEBUG
    virtual void getScannedStats(HeapStats&);
#endif

    virtual void makeConsistentForGC();
    virtual bool isConsistentForGC();

    ThreadState* threadState() { return m_threadState; }
    HeapStats& stats() { return m_threadState->stats(); }
    HeapContainsCache* heapContainsCache() { return m_threadState->heapContainsCache(); }

    inline Address allocate(size_t, const GCInfo*);
    void addToFreeList(Address, size_t);
    void addPageToPool(HeapPage<Header>*);

private:
    // Once pages have been used for one thread heap they will never
    // be reused for another thread heap. Instead of unmapping, we add
    // the pages to a pool of pages to be reused later by this thread
    // heap. This is done as a security feature to avoid type
    // confusion. The heap is type segregated by having separate
    // thread heaps for various types of objects. Holding on to pages
    // ensures that the same virtual address space cannot be used for
    // objects of another type than the type contained in this thread
    // heap.
    class PagePoolEntry {
    public:
        PagePoolEntry(PageMemory* storage, PagePoolEntry* next)
            : m_storage(storage)
            , m_next(next)
        { }

        PageMemory* storage() { return m_storage; }
        PagePoolEntry* next() { return m_next; }

    private:
        PageMemory* m_storage;
        PagePoolEntry* m_next;
    };

    HEAP_EXPORT Address outOfLineAllocate(size_t, const GCInfo*);
    void addPageToHeap(const GCInfo*);
    HEAP_EXPORT Address allocateLargeObject(size_t, const GCInfo*);
    Address currentAllocationPoint() const { return m_currentAllocationPoint; }
    size_t remainingAllocationSize() const { return m_remainingAllocationSize; }
    bool ownsNonEmptyAllocationArea() const { return currentAllocationPoint() && remainingAllocationSize(); }
    void setAllocationPoint(Address point, size_t size)
    {
        ASSERT(!point || heapPageFromAddress(point));
        ASSERT(size <= HeapPage<Header>::payloadSize());
        m_currentAllocationPoint = point;
        m_remainingAllocationSize = size;
    }
    void ensureCurrentAllocation(size_t, const GCInfo*);
    bool allocateFromFreeList(size_t);

    void setFinalizeAll(bool finalizingAll) { m_inFinalizeAll = finalizingAll; }
    void freeLargeObject(LargeHeapObject<Header>*, LargeHeapObject<Header>**);

    void allocatePage(const GCInfo*);
    PageMemory* takePageFromPool();
    void clearPagePool();
    void deletePages();

    Address m_currentAllocationPoint;
    size_t m_remainingAllocationSize;

    HeapPage<Header>* m_firstPage;
    LargeHeapObject<Header>* m_firstLargeHeapObject;

    int m_biggestFreeListIndex;
    bool m_inFinalizeAll;
    ThreadState* m_threadState;

    // All FreeListEntries in the nth list have size >= 2^n.
    FreeListEntry* m_freeLists[blinkPageSizeLog2];

    // List of pages that have been previously allocated, but are now
    // unused.
    PagePoolEntry* m_pagePool;
};

class HEAP_EXPORT Heap {
public:
    enum GCType {
        Normal,
        ForcedForTesting
    };

    static void init(intptr_t* startOfStack);
    static void shutdown();

    static bool contains(Address);
    static bool contains(void* pointer) { return contains(reinterpret_cast<Address>(pointer)); }
    static bool contains(const void* pointer) { return contains(const_cast<void*>(pointer)); }

    // Push a trace callback on the marking stack.
    static void pushTraceCallback(void* containerObject, TraceCallback);

    // Pop the top of the marking stack and call the callback with the visitor
    // and the object. Returns false when there is nothing more to do.
    static bool popAndInvokeTraceCallback(Visitor*);

    // Pop the top of the weak callback stack and call the callback with the visitor
    // and the object. Returns false when there is nothing more to do.
    static bool popAndInvokeWeakPointerCallback(Visitor*)
    {
        // FIXME: Change when moving weak pointers to trunk.
        return false;
    }

    template<typename T> static Address allocate(size_t);
    template<typename T> static Address reallocate(void* previous, size_t);

    static void collectGarbage(ThreadState::StackState, GCType = Normal);

    static void prepareForGC();

    // Collect heap stats for all threads attached to the Blink
    // garbage collector. Should only be called during garbage
    // collection where threads are known to be at safe points.
    static void getStats(HeapStats*);

    static bool isConsistentForGC();
    static void makeConsistentForGC();

    static CallbackStack* s_markingStack;
};

// The NoAllocationScope class is used in debug mode to catch unwanted
// allocations. E.g. allocations during GC.
template<ThreadAffinity Affinity>
class NoAllocationScope {
public:
    NoAllocationScope() : m_active(true) { enter(); }

    explicit NoAllocationScope(bool active) : m_active(active) { enter(); }

    NoAllocationScope(const NoAllocationScope& other) : m_active(other.m_active) { enter(); }

    NoAllocationScope& operator=(const NoAllocationScope& other)
    {
        release();
        m_active = other.m_active;
        enter();
        return *this;
    }

    ~NoAllocationScope() { release(); }

    void release()
    {
        if (m_active) {
            ThreadStateFor<Affinity>::state()->leaveNoAllocationScope();
            m_active = false;
        }
    }

private:
    void enter() const
    {
        if (m_active)
            ThreadStateFor<Affinity>::state()->enterNoAllocationScope();
    }

    bool m_active;
};

// Base class for objects allocated in the Blink garbage-collected
// heap.
//
// Defines a 'new' operator that allocates the memory in the
// heap. 'delete' should not be called on objects that inherit from
// GarbageCollected.
//
// Instances of GarbageCollected will *NOT* get finalized. Their
// destructor will not be called. Therefore, only classes that have
// trivial destructors with no semantic meaning (including all their
// subclasses) should inherit from GarbageCollected. If there are
// non-trival destructors in a given class or any of its subclasses,
// GarbageCollectedFinalized should be used which guarantees that the
// destructor is called on an instance when the garbage collector
// determines that it is no longer reachable.
template<typename T>
class GarbageCollected {
    WTF_MAKE_NONCOPYABLE(GarbageCollected);

    // For now direct allocation of arrays on the heap is not allowed.
    void* operator new[](size_t size);
    void operator delete[](void* p);
public:
    void* operator new(size_t size)
    {
        return Heap::allocate<T>(size);
    }

    void operator delete(void* p)
    {
        ASSERT_NOT_REACHED();
    }

protected:
    GarbageCollected()
    {
        ASSERT(ThreadStateFor<ThreadingTrait<T>::Affinity>::state()->contains(reinterpret_cast<Address>(this)));
    }
    ~GarbageCollected() { }
};

// Base class for objects allocated in the Blink garbage-collected
// heap.
//
// Defines a 'new' operator that allocates the memory in the
// heap. 'delete' should not be called on objects that inherit from
// GarbageCollected.
//
// Instances of GarbageCollectedFinalized will have their destructor
// called when the garbage collector determines that the object is no
// longer reachable.
template<typename T>
class GarbageCollectedFinalized : public GarbageCollected<T> {
    WTF_MAKE_NONCOPYABLE(GarbageCollectedFinalized);

protected:
    // Finalize is called when the object is freed from the heap. By
    // default finalization means calling the destructor on the
    // object. Finalize can be overridden to support calling the
    // destructor of a subclass. This is useful for objects without
    // vtables that require explicit dispatching.
    void finalize()
    {
        static_cast<T*>(this)->~T();
    }

    GarbageCollectedFinalized() { }
    ~GarbageCollectedFinalized() { }

    template<typename U> friend struct HasFinalizer;
    template<typename U, bool> friend struct FinalizerTraitImpl;
};

NO_SANITIZE_ADDRESS
void HeapObjectHeader::checkHeader() const
{
    ASSERT(m_magic == magic);
}

Address HeapObjectHeader::payload()
{
    return reinterpret_cast<Address>(this) + objectHeaderSize;
}

size_t HeapObjectHeader::payloadSize()
{
    return size() - objectHeaderSize;
}

Address HeapObjectHeader::payloadEnd()
{
    return reinterpret_cast<Address>(this) + size();
}

NO_SANITIZE_ADDRESS
void HeapObjectHeader::mark()
{
    checkHeader();
    m_size |= markBitMask;
}

Address FinalizedHeapObjectHeader::payload()
{
    return reinterpret_cast<Address>(this) + finalizedHeaderSize;
}

size_t FinalizedHeapObjectHeader::payloadSize()
{
    return size() - finalizedHeaderSize;
}

template<typename Header>
size_t allocationSizeFromSize(size_t size)
{
    // Check the size before computing the actual allocation size. The
    // allocation size calculation can overflow for large sizes and
    // the check therefore has to happen before any calculation on the
    // size.
    RELEASE_ASSERT(size < maxHeapObjectSize);

    // Add space for header.
    size_t allocationSize = size + sizeof(Header);
    // Align size with allocation granularity.
    allocationSize = (allocationSize + allocationMask) & ~allocationMask;
    return allocationSize;
}

template<typename Header>
Address ThreadHeap<Header>::allocate(size_t size, const GCInfo* gcInfo)
{
    size_t allocationSize = allocationSizeFromSize<Header>(size);
    bool isLargeObject = allocationSize > blinkPageSize / 2;
    if (isLargeObject)
        return allocateLargeObject(allocationSize, gcInfo);
    if (m_remainingAllocationSize < allocationSize)
        return outOfLineAllocate(size, gcInfo);
    Address headerAddress = m_currentAllocationPoint;
    m_currentAllocationPoint += allocationSize;
    m_remainingAllocationSize -= allocationSize;
    Header* header = new (NotNull, headerAddress) Header(allocationSize, gcInfo);
    size_t payloadSize = allocationSize - sizeof(Header);
    stats().increaseObjectSpace(payloadSize);
    Address result = headerAddress + sizeof(*header);
    ASSERT(!(reinterpret_cast<uintptr_t>(result) & allocationMask));
    // Unpoison the memory used for the object (payload).
    ASAN_UNPOISON_MEMORY_REGION(result, payloadSize);
    memset(result, 0, payloadSize);
    ASSERT(heapPageFromAddress(headerAddress + allocationSize - 1));
    return result;
}

// FIXME: Allocate objects that do not need finalization separately
// and use separate sweeping to not have to check for finalizers.
template<typename T>
Address Heap::allocate(size_t size)
{
    ThreadState* state = ThreadStateFor<ThreadingTrait<T>::Affinity>::state();
    ASSERT(state->isAllocationAllowed());
    BaseHeap* heap = state->heap(HeapTrait<T>::index);
    Address addr =
        static_cast<typename HeapTrait<T>::HeapType*>(heap)->allocate(size, GCInfoTrait<T>::get());
    return addr;
}

// FIXME: Allocate objects that do not need finalization separately
// and use separate sweeping to not have to check for finalizers.
template<typename T>
Address Heap::reallocate(void* previous, size_t size)
{
    if (!size) {
        // If the new size is 0 this is equivalent to either
        // free(previous) or malloc(0). In both cases we do
        // nothing and return 0.
        return 0;
    }
    ThreadState* state = ThreadStateFor<ThreadingTrait<T>::Affinity>::State();
    ASSERT(state->isAllocationAllowed());
    // FIXME: Currently only supports raw allocation on the
    // GeneralHeap. Hence we assume the header is a
    // FinalizedHeapObjectHeader.
    ASSERT(HeapTrait<T>::index == GeneralHeap);
    BaseHeap* heap = state->heap(HeapTrait<T>::index);
    Address address = static_cast<typename HeapTrait<T>::HeapType*>(heap)->allocate(size, GCInfoTrait<T>::get());
    if (!previous) {
        // This is equivalent to malloc(size).
        return address;
    }
    FinalizedHeapObjectHeader* previousHeader = FinalizedHeapObjectHeader::fromPayload(previous);
    ASSERT(!previousHeader->hasFinalizer());
    ASSERT(previousHeader->gcInfo() == GCInfoTrait<T>::get());
    size_t copySize = previousHeader->payloadSize();
    if (copySize > size)
        copySize = size;
    memcpy(address, previous, copySize);
    return address;
}

#if COMPILER(CLANG)
// Clang does not export the symbols that we have explicitly asked it
// to export. This forces it to export all the methods from ThreadHeap.
template<> void ThreadHeap<FinalizedHeapObjectHeader>::addPageToHeap(const GCInfo*);
template<> void ThreadHeap<HeapObjectHeader>::addPageToHeap(const GCInfo*);
extern template class HEAP_EXPORT ThreadHeap<FinalizedHeapObjectHeader>;
extern template class HEAP_EXPORT ThreadHeap<HeapObjectHeader>;
#endif

}

#endif // Heap_h
