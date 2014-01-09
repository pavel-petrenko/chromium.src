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
#include "core/rendering/FastTextAutosizer.h"

#include "core/dom/Document.h"
#include "core/frame/Frame.h"
#include "core/frame/FrameView.h"
#include "core/frame/Settings.h"
#include "core/page/Page.h"
#include "core/rendering/InlineIterator.h"
#include "core/rendering/RenderBlock.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/TextAutosizer.h"

using namespace std;

namespace WebCore {

FastTextAutosizer::FastTextAutosizer(Document* document)
    : m_document(document)
{
}

void FastTextAutosizer::record(RenderBlock* block)
{
    if (!enabled())
        return;

    if (!shouldBeClusterRoot(block))
        return;

    AtomicString fingerprint = computeFingerprint(block);
    if (fingerprint.isNull())
        return;

    m_fingerprintMapper.add(block, fingerprint);
}

void FastTextAutosizer::destroy(RenderBlock* block)
{
    m_fingerprintMapper.remove(block);
}

void FastTextAutosizer::beginLayout(RenderBlock* block)
{
    if (!enabled())
        return;

    if (block->isRenderView())
        prepareWindowInfo(toRenderView(block));

    if (shouldBeClusterRoot(block))
        m_clusterStack.append(getOrCreateCluster(block));
}

void FastTextAutosizer::inflate(RenderBlock* block)
{
    if (m_clusterStack.isEmpty())
        return;
    Cluster* cluster = m_clusterStack.last();

    applyMultiplier(block, cluster->m_multiplier);

    // FIXME: Add an optimization to not do this walk if it's not needed.
    for (InlineWalker walker(block); !walker.atEnd(); walker.advance()) {
        RenderObject* inlineObj = walker.current();
        if (inlineObj->isText())
            applyMultiplier(inlineObj, cluster->m_multiplier);
    }
}

void FastTextAutosizer::endLayout(RenderBlock* block)
{
    if (!enabled())
        return;

    if (!m_clusterStack.isEmpty() && m_clusterStack.last()->m_root == block)
        m_clusterStack.removeLast();
}

bool FastTextAutosizer::enabled()
{
    return m_document->settings()
        && m_document->settings()->textAutosizingEnabled()
        && !m_document->printing()
        && m_document->page();
}

void FastTextAutosizer::prepareWindowInfo(RenderView* renderView)
{
    bool horizontalWritingMode = isHorizontalWritingMode(renderView->style()->writingMode());

    Frame* mainFrame = m_document->page()->mainFrame();
    IntSize windowSize = m_document->settings()->textAutosizingWindowSizeOverride();
    if (windowSize.isEmpty())
        windowSize = mainFrame->view()->unscaledVisibleContentSize(ScrollableArea::IncludeScrollbars);
    m_windowWidth = horizontalWritingMode ? windowSize.width() : windowSize.height();

    IntSize layoutSize = m_document->page()->mainFrame()->view()->layoutSize();
    m_layoutWidth = horizontalWritingMode ? layoutSize.width() : layoutSize.height();
}

bool FastTextAutosizer::shouldBeClusterRoot(RenderBlock* block)
{
    // FIXME: move the logic out of TextAutosizer.cpp into this class.
    return block->isRenderView()
        || (TextAutosizer::isAutosizingContainer(block)
            && TextAutosizer::isIndependentDescendant(block));
}

bool FastTextAutosizer::clusterWantsAutosizing(RenderBlock* root)
{
    // FIXME: this should be slightly different.
    return TextAutosizer::containerShouldBeAutosized(root);
}

AtomicString FastTextAutosizer::computeFingerprint(RenderBlock* block)
{
    // FIXME(crbug.com/322340): Implement a fingerprinting algorithm.
    return nullAtom;
}

FastTextAutosizer::Cluster* FastTextAutosizer::getOrCreateCluster(RenderBlock* block)
{
    ClusterMap::AddResult addResult = m_clusters.add(block, PassOwnPtr<Cluster>());
    if (!addResult.isNewEntry)
        return addResult.iterator->value.get();

    AtomicString fingerprint = m_fingerprintMapper.get(block);
    if (fingerprint.isNull()) {
        addResult.iterator->value = adoptPtr(createCluster(block));
        return addResult.iterator->value.get();
    }
    return addSupercluster(fingerprint, block);
}

FastTextAutosizer::Cluster* FastTextAutosizer::createCluster(RenderBlock* block)
{
    float multiplier = clusterWantsAutosizing(block) ? computeMultiplier(block) : 1.0f;
    return new Cluster(block, multiplier);
}

FastTextAutosizer::Cluster* FastTextAutosizer::addSupercluster(AtomicString fingerprint, RenderBlock* returnFor)
{
    BlockSet& roots = m_fingerprintMapper.getBlocks(fingerprint);
    RenderBlock* superRoot = deepestCommonAncestor(roots);

    bool shouldAutosize = false;
    for (BlockSet::iterator it = roots.begin(); it != roots.end(); ++it)
        shouldAutosize |= clusterWantsAutosizing(*it);

    float multiplier = shouldAutosize ? computeMultiplier(superRoot) : 1.0f;

    Cluster* result = 0;
    for (BlockSet::iterator it = roots.begin(); it != roots.end(); ++it) {
        Cluster* cluster = new Cluster(*it, multiplier);
        m_clusters.set(*it, adoptPtr(cluster));

        if (*it == returnFor)
            result = cluster;
    }
    return result;
}

RenderBlock* FastTextAutosizer::deepestCommonAncestor(BlockSet& blocks)
{
    // Find the lowest common ancestor of blocks.
    // Note: this could be improved to not be O(b*h) for b blocks and tree height h.
    HashCountedSet<RenderObject*> ancestors;
    for (BlockSet::iterator it = blocks.begin(); it != blocks.end(); ++it) {
        for (RenderBlock* block = (*it); block; block = block->containingBlock()) {
            ancestors.add(block);
            // The first ancestor that has all of the blocks as children wins.
            if (ancestors.count(block) == blocks.size())
                return block;
        }
    }
    ASSERT_NOT_REACHED();
    return 0;
}

float FastTextAutosizer::computeMultiplier(RenderBlock* block)
{
    // Block width, in CSS pixels.
    float blockWidth = block->contentLogicalWidth();

    // FIXME: incorporate font scale factor.
    // FIXME: incorporate device scale adjustment.
    return max(min(blockWidth, (float) m_layoutWidth) / m_windowWidth, 1.0f);
}

void FastTextAutosizer::applyMultiplier(RenderObject* renderer, float multiplier)
{
    RenderStyle* currentStyle = renderer->style();
    if (currentStyle->textAutosizingMultiplier() == multiplier)
        return;

    // We need to clone the render style to avoid breaking style sharing.
    RefPtr<RenderStyle> style = RenderStyle::clone(currentStyle);
    style->setTextAutosizingMultiplier(multiplier);
    style->setUnique();
    renderer->setStyleInternal(style.release());
}

void FastTextAutosizer::FingerprintMapper::add(RenderBlock* block, AtomicString fingerprint)
{
    m_fingerprints.set(block, fingerprint);

    ReverseFingerprintMap::AddResult addResult = m_blocksForFingerprint.add(fingerprint, PassOwnPtr<BlockSet>());
    if (addResult.isNewEntry)
        addResult.iterator->value = adoptPtr(new BlockSet);
    addResult.iterator->value->add(block);
}

void FastTextAutosizer::FingerprintMapper::remove(RenderBlock* block)
{
    AtomicString fingerprint = m_fingerprints.take(block);
    if (fingerprint.isNull())
        return;

    ReverseFingerprintMap::iterator blocksIter = m_blocksForFingerprint.find(fingerprint);
    BlockSet& blocks = *blocksIter->value;
    blocks.remove(block);
    if (blocks.isEmpty())
        m_blocksForFingerprint.remove(blocksIter);
}

AtomicString FastTextAutosizer::FingerprintMapper::get(RenderBlock* block)
{
    return m_fingerprints.get(block);
}

FastTextAutosizer::BlockSet& FastTextAutosizer::FingerprintMapper::getBlocks(AtomicString fingerprint)
{
    return *m_blocksForFingerprint.get(fingerprint);
}

} // namespace WebCore
