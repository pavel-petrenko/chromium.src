/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "Heap.h"

#include "CodeBlock.h"
#include "ConservativeRoots.h"
#include "GCActivityCallback.h"
#include "HeapRootVisitor.h"
#include "Interpreter.h"
#include "JSGlobalData.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "JSONObject.h"
#include "Tracing.h"
#include <algorithm>

#define COLLECT_ON_EVERY_ALLOCATION 0

using namespace std;

namespace JSC {

const size_t minBytesPerCycle = 512 * 1024;

static inline bool isValidSharedInstanceThreadState()
{
    if (!JSLock::lockCount())
        return false;

    if (!JSLock::currentThreadIsHoldingLock())
        return false;

    return true;
}

static inline bool isValidThreadState(JSGlobalData* globalData)
{
    if (globalData->identifierTable != wtfThreadData().currentIdentifierTable())
        return false;

    if (globalData->isSharedInstance() && !isValidSharedInstanceThreadState())
        return false;

    return true;
}

Heap::Heap(JSGlobalData* globalData)
    : m_operationInProgress(NoOperation)
    , m_newSpace(this)
    , m_extraCost(0)
    , m_markListSet(0)
    , m_activityCallback(DefaultGCActivityCallback::create(this))
    , m_machineThreads(this)
    , m_markStack(globalData->jsArrayVPtr)
    , m_handleHeap(globalData)
    , m_globalData(globalData)
{
    m_newSpace.setHighWaterMark(minBytesPerCycle);
    (*m_activityCallback)();
}

Heap::~Heap()
{
    // The destroy function must already have been called, so assert this.
    ASSERT(!m_globalData);
}

void Heap::destroy()
{
    JSLock lock(SilenceAssertionsOnly);

    if (!m_globalData)
        return;

    ASSERT(!m_globalData->dynamicGlobalObject);
    ASSERT(m_operationInProgress == NoOperation);
    
    // The global object is not GC protected at this point, so sweeping may delete it
    // (and thus the global data) before other objects that may use the global data.
    RefPtr<JSGlobalData> protect(m_globalData);

#if ENABLE(JIT)
    m_globalData->jitStubs->clearHostFunctionStubs();
#endif

    delete m_markListSet;
    m_markListSet = 0;

    clearMarks();
    m_handleHeap.finalizeWeakHandles();
    m_globalData->smallStrings.finalizeSmallStrings();

#if !ENABLE(JSC_ZOMBIES)
    shrink();
    ASSERT(!size());
#endif

    m_globalData = 0;
}

void Heap::reportExtraMemoryCostSlowCase(size_t cost)
{
    // Our frequency of garbage collection tries to balance memory use against speed
    // by collecting based on the number of newly created values. However, for values
    // that hold on to a great deal of memory that's not in the form of other JS values,
    // that is not good enough - in some cases a lot of those objects can pile up and
    // use crazy amounts of memory without a GC happening. So we track these extra
    // memory costs. Only unusually large objects are noted, and we only keep track
    // of this extra cost until the next GC. In garbage collected languages, most values
    // are either very short lived temporaries, or have extremely long lifetimes. So
    // if a large value survives one garbage collection, there is not much point to
    // collecting more frequently as long as it stays alive.

    if (m_extraCost > maxExtraCost && m_extraCost > m_newSpace.highWaterMark() / 2)
        collectAllGarbage();
    m_extraCost += cost;
}

void* Heap::allocate(NewSpace::SizeClass& sizeClass)
{
#if COLLECT_ON_EVERY_ALLOCATION
    collectAllGarbage();
    ASSERT(m_operationInProgress == NoOperation);
#endif

    m_operationInProgress = Allocation;
    void* result = m_newSpace.allocate(sizeClass);
    m_operationInProgress = NoOperation;

    if (result)
        return result;

    if (m_newSpace.waterMark() < m_newSpace.highWaterMark()) {
        m_newSpace.addBlock(sizeClass, allocateBlock(sizeClass.cellSize));
        return allocate(sizeClass);
    }

    collect(DoNotSweep);
    return allocate(sizeClass);
}

void Heap::protect(JSValue k)
{
    ASSERT(k);
    ASSERT(JSLock::currentThreadIsHoldingLock() || !m_globalData->isSharedInstance());

    if (!k.isCell())
        return;

    m_protectedValues.add(k.asCell());
}

bool Heap::unprotect(JSValue k)
{
    ASSERT(k);
    ASSERT(JSLock::currentThreadIsHoldingLock() || !m_globalData->isSharedInstance());

    if (!k.isCell())
        return false;

    return m_protectedValues.remove(k.asCell());
}

void Heap::markProtectedObjects(HeapRootVisitor& heapRootVisitor)
{
    ProtectCountSet::iterator end = m_protectedValues.end();
    for (ProtectCountSet::iterator it = m_protectedValues.begin(); it != end; ++it)
        heapRootVisitor.visit(&it->first);
}

void Heap::pushTempSortVector(Vector<ValueStringPair>* tempVector)
{
    m_tempSortingVectors.append(tempVector);
}

void Heap::popTempSortVector(Vector<ValueStringPair>* tempVector)
{
    ASSERT_UNUSED(tempVector, tempVector == m_tempSortingVectors.last());
    m_tempSortingVectors.removeLast();
}
    
void Heap::markTempSortVectors(HeapRootVisitor& heapRootVisitor)
{
    typedef Vector<Vector<ValueStringPair>* > VectorOfValueStringVectors;

    VectorOfValueStringVectors::iterator end = m_tempSortingVectors.end();
    for (VectorOfValueStringVectors::iterator it = m_tempSortingVectors.begin(); it != end; ++it) {
        Vector<ValueStringPair>* tempSortingVector = *it;

        Vector<ValueStringPair>::iterator vectorEnd = tempSortingVector->end();
        for (Vector<ValueStringPair>::iterator vectorIt = tempSortingVector->begin(); vectorIt != vectorEnd; ++vectorIt) {
            if (vectorIt->first)
                heapRootVisitor.visit(&vectorIt->first);
        }
    }
}

inline RegisterFile& Heap::registerFile()
{
    return m_globalData->interpreter->registerFile();
}

void Heap::markRoots()
{
    ASSERT(isValidThreadState(m_globalData));
    if (m_operationInProgress != NoOperation)
        CRASH();
    m_operationInProgress = Collection;

    void* dummy;

    MarkStack& visitor = m_markStack;
    HeapRootVisitor heapRootVisitor(visitor);

    // We gather conservative roots before clearing mark bits because conservative
    // gathering uses the mark bits to determine whether a reference is valid.
    ConservativeRoots machineThreadRoots(this);
    m_machineThreads.gatherConservativeRoots(machineThreadRoots, &dummy);

    ConservativeRoots registerFileRoots(this);
    registerFile().gatherConservativeRoots(registerFileRoots);

    clearMarks();

    visitor.append(machineThreadRoots);
    visitor.drain();

    visitor.append(registerFileRoots);
    visitor.drain();

    markProtectedObjects(heapRootVisitor);
    visitor.drain();
    
    markTempSortVectors(heapRootVisitor);
    visitor.drain();

    if (m_markListSet && m_markListSet->size())
        MarkedArgumentBuffer::markLists(heapRootVisitor, *m_markListSet);
    if (m_globalData->exception)
        heapRootVisitor.visit(&m_globalData->exception);
    visitor.drain();

    m_handleHeap.visitStrongHandles(heapRootVisitor);
    visitor.drain();

    m_handleStack.visit(heapRootVisitor);
    visitor.drain();

    // Weak handles must be marked last, because their owners use the set of
    // opaque roots to determine reachability.
    int lastOpaqueRootCount;
    do {
        lastOpaqueRootCount = visitor.opaqueRootCount();
        m_handleHeap.visitWeakHandles(heapRootVisitor);
        visitor.drain();
    // If the set of opaque roots has grown, more weak handles may have become reachable.
    } while (lastOpaqueRootCount != visitor.opaqueRootCount());

    visitor.reset();

    m_operationInProgress = NoOperation;
}

void Heap::clearMarks()
{
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        (*it)->clearMarks();
}

void Heap::sweep()
{
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        (*it)->sweep();
}

size_t Heap::objectCount() const
{
    size_t result = 0;
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        result += (*it)->markCount();
    return result;
}

size_t Heap::size() const
{
    size_t result = 0;
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        result += (*it)->size();
    return result;
}

size_t Heap::capacity() const
{
    size_t result = 0;
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        result += (*it)->capacity();
    return result;
}

size_t Heap::globalObjectCount()
{
    return m_globalData->globalObjectCount;
}

size_t Heap::protectedGlobalObjectCount()
{
    size_t count = m_handleHeap.protectedGlobalObjectCount();

    ProtectCountSet::iterator end = m_protectedValues.end();
    for (ProtectCountSet::iterator it = m_protectedValues.begin(); it != end; ++it) {
        if (it->first->isObject() && asObject(it->first)->isGlobalObject())
            count++;
    }

    return count;
}

size_t Heap::protectedObjectCount()
{
    return m_protectedValues.size();
}

class TypeCounter {
public:
    TypeCounter();
    void operator()(JSCell*);
    PassOwnPtr<TypeCountSet> take();
    
private:
    const char* typeName(JSCell*);
    OwnPtr<TypeCountSet> m_typeCountSet;
    HashSet<JSCell*> m_cells;
};

inline TypeCounter::TypeCounter()
    : m_typeCountSet(adoptPtr(new TypeCountSet))
{
}

inline const char* TypeCounter::typeName(JSCell* cell)
{
    if (cell->isString())
        return "string";
    if (cell->isGetterSetter())
        return "Getter-Setter";
    if (cell->isAPIValueWrapper())
        return "API wrapper";
    if (cell->isPropertyNameIterator())
        return "For-in iterator";
    if (const ClassInfo* info = cell->classInfo())
        return info->className;
    if (!cell->isObject())
        return "[empty cell]";
    return "Object";
}

inline void TypeCounter::operator()(JSCell* cell)
{
    if (!m_cells.add(cell).second)
        return;
    m_typeCountSet->add(typeName(cell));
}

inline PassOwnPtr<TypeCountSet> TypeCounter::take()
{
    return m_typeCountSet.release();
}

PassOwnPtr<TypeCountSet> Heap::protectedObjectTypeCounts()
{
    TypeCounter typeCounter;

    ProtectCountSet::iterator end = m_protectedValues.end();
    for (ProtectCountSet::iterator it = m_protectedValues.begin(); it != end; ++it)
        typeCounter(it->first);
    m_handleHeap.protectedObjectTypeCounts(typeCounter);

    return typeCounter.take();
}

void HandleHeap::protectedObjectTypeCounts(TypeCounter& typeCounter)
{
    Node* end = m_strongList.end();
    for (Node* node = m_strongList.begin(); node != end; node = node->next()) {
        JSValue value = *node->slot();
        if (value && value.isCell())
            typeCounter(value.asCell());
    }
}

PassOwnPtr<TypeCountSet> Heap::objectTypeCounts()
{
    TypeCounter typeCounter;
    forEach(typeCounter);
    return typeCounter.take();
}

void Heap::collectAllGarbage()
{
    if (!m_globalData->dynamicGlobalObject)
        m_globalData->recompileAllJSFunctions();

    collect(DoSweep);
}

void Heap::collect(SweepToggle sweepToggle)
{
    ASSERT(globalData()->identifierTable == wtfThreadData().currentIdentifierTable());
    JAVASCRIPTCORE_GC_BEGIN();

    markRoots();
    m_handleHeap.finalizeWeakHandles();
    m_globalData->smallStrings.finalizeSmallStrings();

    JAVASCRIPTCORE_GC_MARKED();
    
    resetAllocator();

#if ENABLE(JSC_ZOMBIES)
    sweepToggle = DoSweep;
#endif

    if (sweepToggle == DoSweep) {
        sweep();
        shrink();
    }

    // To avoid pathological GC churn in large heaps, we set the allocation high
    // water mark to be proportional to the current size of the heap. The exact
    // proportion is a bit arbitrary. A 2X multiplier gives a 1:1 (heap size :
    // new bytes allocated) proportion, and seems to work well in benchmarks.
    size_t proportionalBytes = 2 * size();
    m_newSpace.setHighWaterMark(max(proportionalBytes, minBytesPerCycle));

    JAVASCRIPTCORE_GC_END();

    (*m_activityCallback)();
}

void Heap::resetAllocator()
{
    m_newSpace.resetAllocator();

    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        (*it)->resetAllocator();

    m_extraCost = 0;
}

void Heap::setActivityCallback(PassOwnPtr<GCActivityCallback> activityCallback)
{
    m_activityCallback = activityCallback;
}

GCActivityCallback* Heap::activityCallback()
{
    return m_activityCallback.get();
}

bool Heap::isValidAllocation(size_t bytes)
{
    if (!isValidThreadState(m_globalData))
        return false;

    if (bytes > NewSpace::maxCellSize)
        return false;

    if (m_operationInProgress != NoOperation)
        return false;
    
    return true;
}

MarkedBlock* Heap::allocateBlock(size_t cellSize)
{
    MarkedBlock* block = MarkedBlock::create(this, cellSize);
    m_blocks.add(block);

    return block;
}

void Heap::freeBlocks(DoublyLinkedList<MarkedBlock>& blocks)
{
    MarkedBlock* next;
    for (MarkedBlock* block = blocks.head(); block; block = next) {
        next = block->next();

        m_blocks.remove(block);
        MarkedBlock::destroy(block);
    }
}

void Heap::shrink()
{
    // We record a temporary list of empties to avoid modifying m_blocks while iterating it.
    DoublyLinkedList<MarkedBlock> empties;

    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it) {
        MarkedBlock* block = *it;
        if (!block->isEmpty())
            continue;

        m_newSpace.removeBlock(block);
        empties.append(block);
    }
    
    freeBlocks(empties);
}

} // namespace JSC
