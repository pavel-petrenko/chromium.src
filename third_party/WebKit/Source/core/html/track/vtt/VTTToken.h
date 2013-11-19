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

#ifndef VTTToken_h
#define VTTToken_h

#include "wtf/text/StringBuilder.h"

namespace WebCore {

class VTTTokenTypes {
public:
    enum Type {
        Uninitialized,
        Character,
        StartTag,
        EndTag,
        TimestampTag,
    };
};

class VTTToken {
    WTF_MAKE_NONCOPYABLE(VTTToken);
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef VTTTokenTypes Type;

    VTTToken() { clear(); }

    void appendToName(UChar character)
    {
        ASSERT(m_type == VTTTokenTypes::StartTag || m_type == VTTTokenTypes::EndTag);
        ASSERT(character);
        m_data.append(character);
    }

    Type::Type type() const { return m_type; }

    StringBuilder& name()
    {
        return m_data;
    }

    StringBuilder& characters()
    {
        ASSERT(m_type == Type::Character || m_type == Type::TimestampTag);
        return m_data;
    }

    // Starting a character token works slightly differently than starting
    // other types of tokens because we want to save a per-character branch.
    void ensureIsCharacterToken()
    {
        ASSERT(m_type == Type::Uninitialized || m_type == Type::Character);
        m_type = Type::Character;
    }

    void appendToCharacter(char character)
    {
        ASSERT(m_type == Type::Character);
        m_data.append(character);
    }

    void appendToCharacter(UChar character)
    {
        ASSERT(m_type == Type::Character);
        m_data.append(character);
    }

    void appendToCharacter(const StringBuilder& characters)
    {
        ASSERT(m_type == Type::Character);
        m_data.append(characters);
    }

    void beginEmptyStartTag()
    {
        ASSERT(m_type == Type::Uninitialized);
        m_type = Type::StartTag;
        m_data.clear();
    }

    void beginStartTag(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Type::Uninitialized);
        m_type = Type::StartTag;
        m_data.append(character);
    }

    void beginEndTag(LChar character)
    {
        ASSERT(m_type == Type::Uninitialized);
        m_type = Type::EndTag;
        m_data.append(character);
    }

    void beginTimestampTag(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Type::Uninitialized);
        m_type = Type::TimestampTag;
        m_data.append(character);
    }

    void appendToTimestamp(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Type::TimestampTag);
        m_data.append(character);
    }

    void appendToClass(UChar character)
    {
        appendToStartType(character);
    }

    void addNewClass()
    {
        ASSERT(m_type == Type::StartTag);
        if (!m_classes.isEmpty())
            m_classes.append(' ');
        m_classes.append(m_currentBuffer);
        m_currentBuffer.clear();
    }

    StringBuilder& classes()
    {
        return m_classes;
    }

    void appendToAnnotation(UChar character)
    {
        appendToStartType(character);
    }

    void addNewAnnotation()
    {
        ASSERT(m_type == Type::StartTag);
        m_annotation.clear();
        m_annotation.append(m_currentBuffer);
        m_currentBuffer.clear();
    }

    StringBuilder& annotation()
    {
        return m_annotation;
    }

    void clear()
    {
        m_type = Type::Uninitialized;
        m_data.clear();
        m_annotation.clear();
        m_classes.clear();
        m_currentBuffer.clear();
    }

private:
    void appendToStartType(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Type::StartTag);
        m_currentBuffer.append(character);
    }

    Type::Type m_type;
    StringBuilder m_data;
    StringBuilder m_annotation;
    StringBuilder m_classes;
    StringBuilder m_currentBuffer;
};

}

#endif
