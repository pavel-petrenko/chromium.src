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

#ifndef TimelineTraceEventProcessor_h
#define TimelineTraceEventProcessor_h

#include "core/inspector/InspectorTimelineAgent.h"
#include "platform/JSONValues.h"
#include "platform/TraceEvent.h"
#include "wtf/HashMap.h"
#include "wtf/Threading.h"
#include "wtf/ThreadingPrimitives.h"
#include "wtf/Vector.h"
#include "wtf/WeakPtr.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class InspectorClient;
class InspectorTimelineAgent;
class Page;

class TimelineRecordStack {
private:
    struct Entry {
        Entry(PassRefPtr<JSONObject> record)
            : record(record)
            , children(JSONArray::create())
        {
        }

        RefPtr<JSONObject> record;
        RefPtr<JSONArray> children;
    };

public:
    TimelineRecordStack() { }
    TimelineRecordStack(WeakPtr<InspectorTimelineAgent>);

    void addScopedRecord(PassRefPtr<JSONObject> record);
    void closeScopedRecord(double endTime);
    void addInstantRecord(PassRefPtr<JSONObject> record);

#ifndef NDEBUG
    bool isOpenRecordOfType(const String& type);
#endif

private:
    void send(PassRefPtr<JSONObject>);

    WeakPtr<InspectorTimelineAgent> m_timelineAgent;
    Vector<Entry> m_stack;
};

class TimelineTraceEventProcessor : public ThreadSafeRefCounted<TimelineTraceEventProcessor> {
public:
    TimelineTraceEventProcessor(WeakPtr<InspectorTimelineAgent>, InspectorClient*);
    ~TimelineTraceEventProcessor();

    void shutdown();
    void processEventOnAnyThread(double timestamp, char, const char* name, unsigned long long id,
        int numArgs, const char* const* argNames, const unsigned char* argTypes, const unsigned long long* argValues,
        unsigned char flags);

private:
    struct TimelineThreadState {
        TimelineThreadState() { }

        TimelineThreadState(WeakPtr<InspectorTimelineAgent> timelineAgent)
            : recordStack(timelineAgent)
            , inKnownLayerTask(false)
            , decodedPixelRefId(0)
        {
        }

        TimelineRecordStack recordStack;
        bool inKnownLayerTask;
        unsigned long long decodedPixelRefId;
    };

    class TraceEvent {
    public:
        TraceEvent()
            : m_name(0)
            , m_argumentCount(0)
        {
        }

        TraceEvent(double timestamp, char phase, const char* name, unsigned long long id, ThreadIdentifier threadIdentifier,
            int argumentCount, const char* const* argumentNames, const unsigned char* argumentTypes, const unsigned long long* argumentValues)
            : m_timestamp(timestamp)
            , m_phase(phase)
            , m_name(name)
            , m_id(id)
            , m_threadIdentifier(threadIdentifier)
            , m_argumentCount(argumentCount)
        {
            if (m_argumentCount > MaxArguments) {
                ASSERT_NOT_REACHED();
                m_argumentCount = MaxArguments;
            }
            for (int i = 0; i < m_argumentCount; ++i) {
                m_argumentNames[i] = argumentNames[i];
                m_argumentTypes[i] = argumentTypes[i];
                m_argumentValues[i] = argumentValues[i];
            }
        }

        double timestamp() const { return m_timestamp; }
        char phase() const { return m_phase; }
        const char* name() const { return m_name; }
        unsigned long long id() const { return m_id; }
        ThreadIdentifier threadIdentifier() const { return m_threadIdentifier; }
        int argumentCount() const { return m_argumentCount; }
        bool isNull() const { return !m_name; }

        bool asBool(const char* name) const
        {
            return parameter(name, TRACE_VALUE_TYPE_BOOL).m_bool;
        }
        long long asInt(const char* name) const
        {
            size_t index = findParameter(name);
            if (index == kNotFound || (m_argumentTypes[index] != TRACE_VALUE_TYPE_INT && m_argumentTypes[index] != TRACE_VALUE_TYPE_UINT)) {
                ASSERT_NOT_REACHED();
                return 0;
            }
            return reinterpret_cast<const WebCore::TraceEvent::TraceValueUnion*>(m_argumentValues + index)->m_int;
        }
        unsigned long long asUInt(const char* name) const
        {
            return asInt(name);
        }
        double asDouble(const char* name) const
        {
            return parameter(name, TRACE_VALUE_TYPE_DOUBLE).m_double;
        }
        const char* asString(const char* name) const
        {
            return parameter(name, TRACE_VALUE_TYPE_STRING).m_string;
        }

    private:
        enum { MaxArguments = 2 };

        size_t findParameter(const char*) const;
        const WebCore::TraceEvent::TraceValueUnion& parameter(const char* name, unsigned char expectedType) const;

        double m_timestamp;
        char m_phase;
        const char* m_name;
        unsigned long long m_id;
        ThreadIdentifier m_threadIdentifier;
        int m_argumentCount;
        const char* m_argumentNames[MaxArguments];
        unsigned char m_argumentTypes[MaxArguments];
        unsigned long long m_argumentValues[MaxArguments];
    };

    typedef void (TimelineTraceEventProcessor::*TraceEventHandler)(const TraceEvent&);

    TimelineThreadState& threadState(ThreadIdentifier thread)
    {
        ThreadStateMap::iterator it = m_threadStates.find(thread);
        if (it != m_threadStates.end())
            return it->value;
        return m_threadStates.add(thread, TimelineThreadState(m_timelineAgent)).iterator->value;
    }
    bool maybeEnterLayerTask(const TraceEvent&, TimelineThreadState&);
    void leaveLayerTask(TimelineThreadState&);

    void processBackgroundEvents();
    void processBackgroundEventsTask();
    PassRefPtr<JSONObject> createRecord(const TraceEvent&, const String& recordType, PassRefPtr<JSONObject> data = 0);

    void registerHandler(const char* name, char, TraceEventHandler);

    void onBeginFrame(const TraceEvent&);
    void onUpdateLayerBegin(const TraceEvent&);
    void onUpdateLayerEnd(const TraceEvent&);
    void onPaintLayerBegin(const TraceEvent&);
    void onPaintLayerEnd(const TraceEvent&);
    void onPaintSetupBegin(const TraceEvent&);
    void onPaintSetupEnd(const TraceEvent&);
    void onRasterTaskBegin(const TraceEvent&);
    void onRasterTaskEnd(const TraceEvent&);
    void onPaint(const TraceEvent&);
    void onImageDecodeTaskBegin(const TraceEvent&);
    void onImageDecodeTaskEnd(const TraceEvent&);
    void onImageDecodeBegin(const TraceEvent&);
    void onImageDecodeEnd(const TraceEvent&);
    void onLayerDeleted(const TraceEvent&);
    void onDrawLazyPixelRef(const TraceEvent&);
    void onLazyPixelRefDeleted(const TraceEvent&);

    WeakPtr<InspectorTimelineAgent> m_timelineAgent;
    TimelineTimeConverter m_timeConverter;
    InspectorClient* m_inspectorClient;
    unsigned long long m_pageId;
    int m_layerTreeId;

    typedef HashMap<std::pair<String, int>, TraceEventHandler> HandlersMap;
    HandlersMap m_handlersByType;
    Mutex m_backgroundEventsMutex;
    Vector<TraceEvent> m_backgroundEvents;
    double m_lastEventProcessingTime;
    bool m_processEventsTaskInFlight;

    typedef HashMap<ThreadIdentifier, TimelineThreadState> ThreadStateMap;
    ThreadStateMap m_threadStates;

    HashMap<unsigned long long, long long> m_layerToNodeMap;
    unsigned long long m_layerId;
    double m_paintSetupStart;
    double m_paintSetupEnd;
    RefPtr<JSONObject> m_gpuTask;

    struct ImageInfo {
        int backendNodeId;
        String url;

        ImageInfo() { }
        ImageInfo(int backendNodeId, String url) : backendNodeId(backendNodeId), url(url) { }
    };
    typedef HashMap<unsigned long long, ImageInfo> PixelRefToImageInfoMap;
    PixelRefToImageInfoMap m_pixelRefToImageInfo;
};

} // namespace WebCore

#endif // !defined(TimelineTraceEventProcessor_h)
