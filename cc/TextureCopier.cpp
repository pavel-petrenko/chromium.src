// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "TextureCopier.h"

#include "CCRendererGL.h" // For the GLC() macro.
#include "GraphicsContext3D.h"
#include "TraceEvent.h"
#include <public/WebGraphicsContext3D.h>

namespace WebCore {

#if USE(ACCELERATED_COMPOSITING)
AcceleratedTextureCopier::AcceleratedTextureCopier(WebKit::WebGraphicsContext3D* context, bool usingBindUniforms)
    : m_context(context)
    , m_usingBindUniforms(usingBindUniforms)
{
    ASSERT(m_context);
    GLC(m_context, m_fbo = m_context->createFramebuffer());
    GLC(m_context, m_positionBuffer = m_context->createBuffer());

    static const float kPositions[4][4] = {
        {-1, -1, 0, 1},
        { 1, -1, 0, 1},
        { 1, 1, 0, 1},
        {-1, 1, 0, 1}
    };

    GLC(m_context, m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_positionBuffer));
    GLC(m_context, m_context->bufferData(GraphicsContext3D::ARRAY_BUFFER, sizeof(kPositions), kPositions, GraphicsContext3D::STATIC_DRAW));
    GLC(m_context, m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, 0));

    m_blitProgram = adoptPtr(new BlitProgram(m_context));
}

AcceleratedTextureCopier::~AcceleratedTextureCopier()
{
    if (m_blitProgram)
        m_blitProgram->cleanup(m_context);
    if (m_positionBuffer)
        GLC(m_context, m_context->deleteBuffer(m_positionBuffer));
    if (m_fbo)
        GLC(m_context, m_context->deleteFramebuffer(m_fbo));
}

void AcceleratedTextureCopier::copyTexture(Parameters parameters)
{
    TRACE_EVENT0("cc", "TextureCopier::copyTexture");

    // Note: this code does not restore the viewport, bound program, 2D texture, framebuffer, buffer or blend enable.
    GLC(m_context, m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo));
    GLC(m_context, m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, parameters.destTexture, 0));

#if OS(ANDROID)
    // Clear destination to improve performance on tiling GPUs.
    // TODO: Use EXT_discard_framebuffer or skip clearing if it isn't available.
    GLC(m_context, m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT));
#endif

    GLC(m_context, m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, parameters.sourceTexture));
    GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::NEAREST));
    GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::NEAREST));

    if (!m_blitProgram->initialized())
        m_blitProgram->initialize(m_context, m_usingBindUniforms);

    // TODO: Use EXT_framebuffer_blit if available.
    GLC(m_context, m_context->useProgram(m_blitProgram->program()));

    const int kPositionAttribute = 0;
    GLC(m_context, m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_positionBuffer));
    GLC(m_context, m_context->vertexAttribPointer(kPositionAttribute, 4, GraphicsContext3D::FLOAT, false, 0, 0));
    GLC(m_context, m_context->enableVertexAttribArray(kPositionAttribute));
    GLC(m_context, m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, 0));

    GLC(m_context, m_context->viewport(0, 0, parameters.size.width(), parameters.size.height()));
    GLC(m_context, m_context->disable(GraphicsContext3D::BLEND));
    GLC(m_context, m_context->drawArrays(GraphicsContext3D::TRIANGLE_FAN, 0, 4));

    GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR));
    GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR));
    GLC(m_context, m_context->disableVertexAttribArray(kPositionAttribute));

    GLC(m_context, m_context->useProgram(0));

    GLC(m_context, m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, 0, 0));
    GLC(m_context, m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0));
    GLC(m_context, m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0));
}

void AcceleratedTextureCopier::flush()
{
    GLC(m_context, m_context->flush());
}

}

#endif // USE(ACCELERATED_COMPOSITING)
