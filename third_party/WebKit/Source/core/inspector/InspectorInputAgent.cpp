/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "core/inspector/InspectorInputAgent.h"

#include "core/inspector/InspectorClient.h"
#include "core/page/Chrome.h"
#include "core/page/Frame.h"
#include "core/page/FrameView.h"
#include "core/page/Page.h"
#include "core/platform/PlatformEvent.h"
#include "core/platform/PlatformKeyboardEvent.h"
#include "core/platform/PlatformMouseEvent.h"
#include "core/platform/graphics/IntPoint.h"
#include "core/platform/graphics/IntRect.h"
#include "core/platform/graphics/IntSize.h"
#include "wtf/CurrentTime.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

InspectorInputAgent::InspectorInputAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState, Page* page, InspectorClient* client)
    : InspectorBaseAgent<InspectorInputAgent>("Input", instrumentingAgents, inspectorState)
    , m_page(page), m_client(client)
{
}

InspectorInputAgent::~InspectorInputAgent()
{
}

void InspectorInputAgent::dispatchKeyEvent(ErrorString* error, const String& type, const int* modifiers, const double* timestamp, const String* text, const String* unmodifiedText, const String* keyIdentifier, const int* windowsVirtualKeyCode, const int* nativeVirtualKeyCode, const int* macCharCode, const bool* autoRepeat, const bool* isKeypad, const bool* isSystemKey)
{
    PlatformEvent::Type convertedType;
    if (type == "keyDown")
        convertedType = PlatformEvent::KeyDown;
    else if (type == "keyUp")
        convertedType = PlatformEvent::KeyUp;
    else if (type == "char")
        convertedType = PlatformEvent::Char;
    else if (type == "rawKeyDown")
        convertedType = PlatformEvent::RawKeyDown;
    else {
        *error = "Unrecognized type: " + type;
        return;
    }

    PlatformKeyboardEvent event(
        convertedType,
        text ? *text : "",
        unmodifiedText ? *unmodifiedText : "",
        keyIdentifier ? *keyIdentifier : "",
        windowsVirtualKeyCode ? *windowsVirtualKeyCode : 0,
        nativeVirtualKeyCode ? *nativeVirtualKeyCode : 0,
        macCharCode ? *macCharCode : 0,
        autoRepeat ? *autoRepeat : false,
        isKeypad ? *isKeypad : false,
        isSystemKey ? *isSystemKey : false,
        static_cast<PlatformEvent::Modifiers>(modifiers ? *modifiers : 0),
        timestamp ? *timestamp : currentTime());
    m_client->dispatchKeyEvent(event);
}

void InspectorInputAgent::dispatchMouseEvent(ErrorString* error, const String& type, int x, int y, const int* modifiers, const double* timestamp, const String* button, const int* clickCount)
{
    PlatformEvent::Type convertedType;
    if (type == "mousePressed")
        convertedType = PlatformEvent::MousePressed;
    else if (type == "mouseReleased")
        convertedType = PlatformEvent::MouseReleased;
    else if (type == "mouseMoved")
        convertedType = PlatformEvent::MouseMoved;
    else {
        *error = "Unrecognized type: " + type;
        return;
    }

    int convertedModifiers = modifiers ? *modifiers : 0;

    MouseButton convertedButton = NoButton;
    if (button) {
        if (*button == "left")
            convertedButton = LeftButton;
        else if (*button == "middle")
            convertedButton = MiddleButton;
        else if (*button == "right")
            convertedButton = RightButton;
        else if (*button != "none") {
            *error = "Unrecognized button: " + *button;
            return;
        }
    }

    // Some platforms may have flipped coordinate systems, but the given coordinates
    // assume the origin is in the top-left of the window. Convert.
    IntPoint convertedPoint = m_page->mainFrame()->view()->convertToContainingWindow(IntPoint(x, y));
    IntPoint globalPoint = m_page->chrome().rootViewToScreen(IntRect(IntPoint(x, y), IntSize(0, 0))).location();

    PlatformMouseEvent event(
        convertedPoint,
        globalPoint,
        convertedButton,
        convertedType,
        clickCount ? *clickCount : 0,
        convertedModifiers & PlatformEvent::ShiftKey,
        convertedModifiers & PlatformEvent::CtrlKey,
        convertedModifiers & PlatformEvent::AltKey,
        convertedModifiers & PlatformEvent::MetaKey,
        timestamp ? *timestamp : currentTime());

    m_client->dispatchMouseEvent(event);
}

} // namespace WebCore

