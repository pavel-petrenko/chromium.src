/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "core/editing/DeleteFromTextNodeCommand.h"

#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ExceptionStatePlaceholder.h"
#include "core/accessibility/AXObjectCache.h"
#include "core/dom/Document.h"
#include "core/dom/Text.h"

namespace WebCore {

DeleteFromTextNodeCommand::DeleteFromTextNodeCommand(PassRefPtr<Text> node, unsigned offset, unsigned count)
    : SimpleEditCommand(node->document())
    , m_node(node)
    , m_offset(offset)
    , m_count(count)
{
    ASSERT(m_node);
    ASSERT(m_offset <= m_node->length());
    ASSERT(m_offset + m_count <= m_node->length());
}

void DeleteFromTextNodeCommand::doApply()
{
    ASSERT(m_node);

    if (!m_node->isContentEditable(Node::UserSelectAllIsAlwaysNonEditable))
        return;

    TrackExceptionState es;
    m_text = m_node->substringData(m_offset, m_count, es);
    if (es.hadException())
        return;

    // Need to notify this before actually deleting the text
    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->nodeTextChangeNotification(m_node.get(), AXObjectCache::AXTextDeleted, m_offset, m_text);

    m_node->deleteData(m_offset, m_count, es);
}

void DeleteFromTextNodeCommand::doUnapply()
{
    ASSERT(m_node);

    if (!m_node->rendererIsEditable())
        return;

    m_node->insertData(m_offset, m_text, IGNORE_EXCEPTION);

    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->nodeTextChangeNotification(m_node.get(), AXObjectCache::AXTextInserted, m_offset, m_text);
}

#ifndef NDEBUG
void DeleteFromTextNodeCommand::getNodesInCommand(HashSet<Node*>& nodes)
{
    addNodeAndDescendants(m_node.get(), nodes);
}
#endif

} // namespace WebCore
