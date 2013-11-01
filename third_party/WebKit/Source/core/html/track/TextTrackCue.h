/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2012, 2013 Apple Inc.  All rights reserved.
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

#ifndef TextTrackCue_h
#define TextTrackCue_h

#include "bindings/v8/ScriptWrappable.h"
#include "core/events/EventTarget.h"
#include "core/html/HTMLDivElement.h"
#include "core/page/UseCounter.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class Document;
class DocumentFragment;
class ExceptionState;
class ExecutionContext;
class TextTrack;
class TextTrackCue;

// ----------------------------

class TextTrackCueBox FINAL : public HTMLDivElement {
public:
    static PassRefPtr<TextTrackCueBox> create(Document& document, TextTrackCue* cue)
    {
        return adoptRef(new TextTrackCueBox(document, cue));
    }

    TextTrackCue* getCue() const;
    void applyCSSProperties(const IntSize& videoSize);

    static const AtomicString& textTrackCueBoxShadowPseudoId();

protected:
    TextTrackCueBox(Document&, TextTrackCue*);

    virtual RenderObject* createRenderer(RenderStyle*) OVERRIDE;

    TextTrackCue* m_cue;
};

// ----------------------------

class TextTrackCue : public RefCounted<TextTrackCue>, public ScriptWrappable, public EventTargetWithInlineData {
    REFCOUNTED_EVENT_TARGET(TextTrackCue);
public:
    static PassRefPtr<TextTrackCue> create(Document& document, double start, double end, const String& content)
    {
        UseCounter::count(document, UseCounter::TextTrackCueConstructor);
        return adoptRef(new TextTrackCue(document, start, end, content));
    }

    static const AtomicString& cueShadowPseudoId()
    {
        DEFINE_STATIC_LOCAL(const AtomicString, cue, ("cue", AtomicString::ConstructFromLiteral));
        return cue;
    }

    virtual ~TextTrackCue();

    TextTrack* track() const;
    void setTrack(TextTrack*);

    const String& id() const { return m_id; }
    void setId(const String&);

    double startTime() const { return m_startTime; }
    void setStartTime(double, ExceptionState&);

    double endTime() const { return m_endTime; }
    void setEndTime(double, ExceptionState&);

    bool pauseOnExit() const { return m_pauseOnExit; }
    void setPauseOnExit(bool);

    const String& vertical() const;
    void setVertical(const String&, ExceptionState&);

    bool snapToLines() const { return m_snapToLines; }
    void setSnapToLines(bool);

    int line() const { return m_linePosition; }
    void setLine(int, ExceptionState&);

    int position() const { return m_textPosition; }
    void setPosition(int, ExceptionState&);

    int size() const { return m_cueSize; }
    void setSize(int, ExceptionState&);

    const String& align() const;
    void setAlign(const String&, ExceptionState&);

    const String& text() const { return m_content; }
    void setText(const String&);

    const String& cueSettings() const { return m_settings; }
    void setCueSettings(const String&);

    int cueIndex();
    void invalidateCueIndex();

    PassRefPtr<DocumentFragment> getCueAsHTML();
    PassRefPtr<DocumentFragment> createCueRenderingTree();

    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

    const String& regionId() const { return m_regionId; }
    void setRegionId(const String&);
    void notifyRegionWhenRemovingDisplayTree(bool);

    bool isActive();
    void setIsActive(bool);

    bool hasDisplayTree() const { return m_displayTree; }
    PassRefPtr<TextTrackCueBox> getDisplayTree(const IntSize& videoSize);
    PassRefPtr<HTMLDivElement> element() const { return m_cueBackgroundBox; }

    void updateDisplayTree(double);
    void removeDisplayTree();
    void markFutureAndPastNodes(ContainerNode*, double, double);

    int calculateComputedLinePosition();

    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ExecutionContext* executionContext() const OVERRIDE;

    std::pair<double, double> getCSSPosition() const;

    CSSValueID getCSSAlignment() const;
    int getCSSSize() const;
    CSSValueID getCSSWritingDirection() const;
    CSSValueID getCSSWritingMode() const;

    enum WritingDirection {
        Horizontal = 0,
        VerticalGrowingLeft,
        VerticalGrowingRight,
        NumberOfWritingDirections
    };
    WritingDirection getWritingDirection() const { return m_writingDirection; }

    enum CueAlignment {
        Start = 0,
        Middle,
        End,
        Left,
        Right,
        NumberOfAlignments
    };
    CueAlignment getAlignment() const { return m_cueAlignment; }

    bool operator==(const TextTrackCue&) const;
    bool operator!=(const TextTrackCue& cue) const
    {
        return !(*this == cue);
    }

    DEFINE_ATTRIBUTE_EVENT_LISTENER(enter);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(exit);

private:
    TextTrackCue(Document&, double start, double end, const String& content);

    Document& document() const;

    PassRefPtr<TextTrackCueBox> displayTreeInternal();

    void createWebVTTNodeTree();
    void copyWebVTTNodeToDOMTree(ContainerNode* WebVTTNode, ContainerNode* root);

    std::pair<double, double> getPositionCoordinates() const;
    void parseSettings(const String&);

    void determineTextDirection();
    void calculateDisplayParameters();

    void cueWillChange();
    void cueDidChange();

    enum CueSetting {
        None,
        Vertical,
        Line,
        Position,
        Size,
        Align,
        RegionId
    };
    CueSetting settingName(const String&);

    String m_id;
    double m_startTime;
    double m_endTime;
    String m_content;
    String m_settings;
    int m_linePosition;
    int m_computedLinePosition;
    int m_textPosition;
    int m_cueSize;
    int m_cueIndex;

    WritingDirection m_writingDirection;

    CueAlignment m_cueAlignment;

    RefPtr<DocumentFragment> m_webVTTNodeTree;
    TextTrack* m_track;

    bool m_isActive;
    bool m_pauseOnExit;
    bool m_snapToLines;

    RefPtr<HTMLDivElement> m_cueBackgroundBox;

    bool m_displayTreeShouldChange;
    RefPtr<TextTrackCueBox> m_displayTree;

    CSSValueID m_displayDirection;

    int m_displaySize;

    std::pair<float, float> m_displayPosition;
    String m_regionId;
    bool m_notifyRegion;
};

} // namespace WebCore

#endif
