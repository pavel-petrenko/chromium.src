/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EditorClient_h
#define EditorClient_h

#include "wtf/PassRefPtr.h"

namespace WebCore {

class Element;
class Frame;
class KeyboardEvent;
class UndoStep;

class EditorClient {
public:
    virtual ~EditorClient() { }

    virtual void respondToChangedContents() = 0;
    virtual void respondToChangedSelection(Frame*) = 0;

    virtual void registerUndoStep(PassRefPtr<UndoStep>) = 0;
    virtual void registerRedoStep(PassRefPtr<UndoStep>) = 0;
    virtual void clearUndoRedoOperations() = 0;

    virtual bool canCopyCut(Frame*, bool defaultValue) const = 0;
    virtual bool canPaste(Frame*, bool defaultValue) const = 0;
    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;

    virtual void undo() = 0;
    virtual void redo() = 0;

    virtual void handleKeyboardEvent(KeyboardEvent*) = 0;

    virtual void textFieldDidEndEditing(Element*) = 0;
    virtual void textDidChangeInTextField(Element*) = 0;
    virtual bool doTextFieldCommandFromEvent(Element*, KeyboardEvent*) = 0;
};

}

#endif // EditorClient_h
