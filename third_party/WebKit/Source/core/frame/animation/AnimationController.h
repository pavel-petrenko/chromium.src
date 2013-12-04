/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AnimationController_h
#define AnimationController_h

#include "CSSPropertyNames.h"
#include "wtf/Forward.h"
#include "wtf/OwnPtr.h"

namespace WebCore {

class AnimationBase;
class AnimationControllerPrivate;
class Document;
class Element;
class Frame;
class Node;
class RenderObject;
class RenderStyle;

class AnimationController {
public:
    AnimationController(Frame*);
    ~AnimationController();

    void cancelAnimations(RenderObject*);
    PassRefPtr<RenderStyle> updateAnimations(RenderObject&, RenderStyle& newStyle);
    PassRefPtr<RenderStyle> getAnimatedStyleForRenderer(RenderObject*);

    // This is called when an accelerated animation or transition has actually started to animate.
    void notifyAnimationStarted(RenderObject*, double startTime);

    void pauseAnimationsForTesting(double t);
    unsigned numberOfActiveAnimations(Document*) const; // To be used only for testing

    bool isRunningAnimationOnRenderer(RenderObject*, CSSPropertyID, bool isRunningNow = true) const;
    bool isRunningAcceleratableAnimationOnRenderer(RenderObject*) const;
    bool isRunningAcceleratedAnimationOnRenderer(RenderObject*, CSSPropertyID, bool isRunningNow = true) const;

    void serviceAnimations();

    void beginAnimationUpdate();
    void endAnimationUpdate();

    static bool supportsAcceleratedAnimationOfProperty(CSSPropertyID);

private:
    OwnPtr<AnimationControllerPrivate> m_data;
    int m_beginAnimationUpdateCount;
};

class AnimationUpdateBlock {
public:
    explicit AnimationUpdateBlock(AnimationController* animationController)
        : m_animationController(animationController)
    {
        if (m_animationController)
            m_animationController->beginAnimationUpdate();
    }

    explicit AnimationUpdateBlock(AnimationController& animationController)
        : m_animationController(&animationController)
    {
        m_animationController->beginAnimationUpdate();
    }

    ~AnimationUpdateBlock()
    {
        if (m_animationController)
            m_animationController->endAnimationUpdate();
    }

    AnimationController* m_animationController;
};

} // namespace WebCore

#endif // AnimationController_h
