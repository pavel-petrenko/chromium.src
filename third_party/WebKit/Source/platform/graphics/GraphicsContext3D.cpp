/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2010 Mozilla Corporation. All rights reserved.
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

#include "platform/graphics/GraphicsContext3D.h"
#include "platform/CheckedInt.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/ImageBuffer.h"
#include "platform/graphics/ImageObserver.h"
#include "platform/graphics/gpu/DrawingBuffer.h"
#include "platform/image-decoders/ImageDecoder.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"
#include "wtf/CPU.h"
#include "wtf/text/CString.h"
#include "wtf/text/StringHash.h"

#include "public/platform/Platform.h"
#include "public/platform/WebGraphicsContext3D.h"
#include "public/platform/WebGraphicsContext3DProvider.h"

namespace WebCore {

namespace {

void getDrawingParameters(DrawingBuffer* drawingBuffer, blink::WebGraphicsContext3D* graphicsContext3D,
                          Platform3DObject* frameBufferId, int* width, int* height)
{
    ASSERT(drawingBuffer);
    *frameBufferId = drawingBuffer->framebuffer();
    *width = drawingBuffer->size().width();
    *height = drawingBuffer->size().height();
}

} // anonymous namespace

GraphicsContext3D::GraphicsContext3D(PassOwnPtr<blink::WebGraphicsContext3D> webContext, bool preserveDrawingBuffer)
    : m_impl(webContext.get())
    , m_ownedWebContext(webContext)
    , m_initializedAvailableExtensions(false)
    , m_layerComposited(false)
    , m_preserveDrawingBuffer(preserveDrawingBuffer)
    , m_packAlignment(4)
    , m_grContext(0)
{
}

GraphicsContext3D::GraphicsContext3D(PassOwnPtr<blink::WebGraphicsContext3DProvider> provider, bool preserveDrawingBuffer)
    : m_provider(provider)
    , m_impl(m_provider->context3d())
    , m_initializedAvailableExtensions(false)
    , m_layerComposited(false)
    , m_preserveDrawingBuffer(preserveDrawingBuffer)
    , m_packAlignment(4)
    , m_grContext(m_provider->grContext())
{
}

GraphicsContext3D::GraphicsContext3D(blink::WebGraphicsContext3D* webContext)
    : m_impl(webContext)
    , m_initializedAvailableExtensions(false)
    , m_layerComposited(false)
    , m_preserveDrawingBuffer(false)
    , m_packAlignment(4)
    , m_grContext(0)
{
}

GraphicsContext3D::~GraphicsContext3D()
{
    setContextLostCallback(nullptr);
    setErrorMessageCallback(nullptr);
}

// Macros to assist in delegating from GraphicsContext3D to
// WebGraphicsContext3D.

#define DELEGATE_TO_WEBCONTEXT(name) \
void GraphicsContext3D::name() \
{ \
    m_impl->name(); \
}

#define DELEGATE_TO_WEBCONTEXT_R(name, rt) \
rt GraphicsContext3D::name() \
{ \
    return m_impl->name(); \
}

#define DELEGATE_TO_WEBCONTEXT_1(name, t1) \
void GraphicsContext3D::name(t1 a1) \
{ \
    m_impl->name(a1); \
}

#define DELEGATE_TO_WEBCONTEXT_1R(name, t1, rt) \
rt GraphicsContext3D::name(t1 a1) \
{ \
    return m_impl->name(a1); \
}

#define DELEGATE_TO_WEBCONTEXT_2(name, t1, t2) \
void GraphicsContext3D::name(t1 a1, t2 a2) \
{ \
    m_impl->name(a1, a2); \
}

#define DELEGATE_TO_WEBCONTEXT_3(name, t1, t2, t3) \
void GraphicsContext3D::name(t1 a1, t2 a2, t3 a3) \
{ \
    m_impl->name(a1, a2, a3); \
}

#define DELEGATE_TO_WEBCONTEXT_4(name, t1, t2, t3, t4) \
void GraphicsContext3D::name(t1 a1, t2 a2, t3 a3, t4 a4) \
{ \
    m_impl->name(a1, a2, a3, a4); \
}

#define DELEGATE_TO_WEBCONTEXT_5(name, t1, t2, t3, t4, t5) \
void GraphicsContext3D::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) \
{ \
    m_impl->name(a1, a2, a3, a4, a5); \
}

#define DELEGATE_TO_WEBCONTEXT_6(name, t1, t2, t3, t4, t5, t6) \
void GraphicsContext3D::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) \
{ \
    m_impl->name(a1, a2, a3, a4, a5, a6); \
}

#define DELEGATE_TO_WEBCONTEXT_7(name, t1, t2, t3, t4, t5, t6, t7) \
void GraphicsContext3D::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7) \
{ \
    m_impl->name(a1, a2, a3, a4, a5, a6, a7); \
}

#define DELEGATE_TO_WEBCONTEXT_9(name, t1, t2, t3, t4, t5, t6, t7, t8, t9) \
void GraphicsContext3D::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8, t9 a9) \
{ \
    m_impl->name(a1, a2, a3, a4, a5, a6, a7, a8, a9); \
}

class GraphicsContext3DContextLostCallbackAdapter : public blink::WebGraphicsContext3D::WebGraphicsContextLostCallback {
public:
    GraphicsContext3DContextLostCallbackAdapter(PassOwnPtr<GraphicsContext3D::ContextLostCallback> callback)
        : m_contextLostCallback(callback) { }
    virtual ~GraphicsContext3DContextLostCallbackAdapter() { }

    virtual void onContextLost()
    {
        if (m_contextLostCallback)
            m_contextLostCallback->onContextLost();
    }
private:
    OwnPtr<GraphicsContext3D::ContextLostCallback> m_contextLostCallback;
};

class GraphicsContext3DErrorMessageCallbackAdapter : public blink::WebGraphicsContext3D::WebGraphicsErrorMessageCallback {
public:
    GraphicsContext3DErrorMessageCallbackAdapter(PassOwnPtr<GraphicsContext3D::ErrorMessageCallback> callback)
        : m_errorMessageCallback(callback) { }
    virtual ~GraphicsContext3DErrorMessageCallbackAdapter() { }

    virtual void onErrorMessage(const blink::WebString& message, blink::WGC3Dint id)
    {
        if (m_errorMessageCallback)
            m_errorMessageCallback->onErrorMessage(message, id);
    }
private:
    OwnPtr<GraphicsContext3D::ErrorMessageCallback> m_errorMessageCallback;
};

void GraphicsContext3D::setContextLostCallback(PassOwnPtr<GraphicsContext3D::ContextLostCallback> callback)
{
    if (m_ownedWebContext) {
        m_contextLostCallbackAdapter = adoptPtr(new GraphicsContext3DContextLostCallbackAdapter(callback));
        m_ownedWebContext->setContextLostCallback(m_contextLostCallbackAdapter.get());
    }
}

void GraphicsContext3D::setErrorMessageCallback(PassOwnPtr<GraphicsContext3D::ErrorMessageCallback> callback)
{
    if (m_ownedWebContext) {
        m_errorMessageCallbackAdapter = adoptPtr(new GraphicsContext3DErrorMessageCallbackAdapter(callback));
        m_ownedWebContext->setErrorMessageCallback(m_errorMessageCallbackAdapter.get());
    }
}

PassRefPtr<GraphicsContext3D> GraphicsContext3D::createContextSupport(blink::WebGraphicsContext3D* webContext)
{
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(webContext));
    return context.release();
}

// The following three creation methods are obsolete and should not be used by new code. They will be removed soon.
PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs)
{
    blink::WebGraphicsContext3D::Attributes webAttributes;
    webAttributes.alpha = attrs.alpha;
    webAttributes.depth = attrs.depth;
    webAttributes.stencil = attrs.stencil;
    webAttributes.antialias = attrs.antialias;
    webAttributes.premultipliedAlpha = attrs.premultipliedAlpha;
    webAttributes.noExtensions = attrs.noExtensions;
    webAttributes.shareResources = attrs.shareResources;
    webAttributes.preferDiscreteGPU = attrs.preferDiscreteGPU;
    webAttributes.failIfMajorPerformanceCaveat = attrs.failIfMajorPerformanceCaveat;
    webAttributes.topDocumentURL = attrs.topDocumentURL.string();

    OwnPtr<blink::WebGraphicsContext3D> webContext = adoptPtr(blink::Platform::current()->createOffscreenGraphicsContext3D(webAttributes));
    if (!webContext)
        return 0;

    return GraphicsContext3D::createGraphicsContextFromWebContext(webContext.release(), attrs.preserveDrawingBuffer);
}

PassRefPtr<GraphicsContext3D> GraphicsContext3D::createGraphicsContextFromProvider(PassOwnPtr<blink::WebGraphicsContext3DProvider> provider, bool preserveDrawingBuffer)
{
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(provider, preserveDrawingBuffer));
    return context.release();
}

PassRefPtr<GraphicsContext3D> GraphicsContext3D::createGraphicsContextFromWebContext(PassOwnPtr<blink::WebGraphicsContext3D> webContext, bool preserveDrawingBuffer)
{
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(webContext, preserveDrawingBuffer));
    return context.release();
}

GrContext* GraphicsContext3D::grContext()
{
    return m_grContext;
}

DELEGATE_TO_WEBCONTEXT_R(makeContextCurrent, bool)
DELEGATE_TO_WEBCONTEXT_R(lastFlushID, uint32_t)

DELEGATE_TO_WEBCONTEXT_1(activeTexture, GLenum)
DELEGATE_TO_WEBCONTEXT_2(attachShader, Platform3DObject, Platform3DObject)

DELEGATE_TO_WEBCONTEXT_2(bindBuffer, GLenum, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_2(bindFramebuffer, GLenum, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_2(bindRenderbuffer, GLenum, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_2(bindTexture, GLenum, Platform3DObject)

DELEGATE_TO_WEBCONTEXT_4(bufferData, GLenum, GLsizeiptr, const void*, GLenum)

DELEGATE_TO_WEBCONTEXT_1R(checkFramebufferStatus, GLenum, GLenum)
DELEGATE_TO_WEBCONTEXT_1(clear, GLbitfield)
DELEGATE_TO_WEBCONTEXT_4(clearColor, GLclampf, GLclampf, GLclampf, GLclampf)
DELEGATE_TO_WEBCONTEXT_1(clearDepth, GLclampf)
DELEGATE_TO_WEBCONTEXT_1(clearStencil, GLint)
DELEGATE_TO_WEBCONTEXT_4(colorMask, GLboolean, GLboolean, GLboolean, GLboolean)
DELEGATE_TO_WEBCONTEXT_1(compileShader, Platform3DObject)

DELEGATE_TO_WEBCONTEXT_1(depthMask, GLboolean)
DELEGATE_TO_WEBCONTEXT_1(disable, GLenum)
DELEGATE_TO_WEBCONTEXT_1(disableVertexAttribArray, GLuint)
DELEGATE_TO_WEBCONTEXT_4(drawElements, GLenum, GLsizei, GLenum, GLintptr)

DELEGATE_TO_WEBCONTEXT_1(enable, GLenum)
DELEGATE_TO_WEBCONTEXT_1(enableVertexAttribArray, GLuint)
DELEGATE_TO_WEBCONTEXT(finish)
DELEGATE_TO_WEBCONTEXT(flush)
DELEGATE_TO_WEBCONTEXT_4(framebufferRenderbuffer, GLenum, GLenum, GLenum, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_5(framebufferTexture2D, GLenum, GLenum, GLenum, Platform3DObject, GLint)

bool GraphicsContext3D::getActiveAttrib(Platform3DObject program, GLuint index, ActiveInfo& info)
{
    blink::WebGraphicsContext3D::ActiveInfo webInfo;
    if (!m_impl->getActiveAttrib(program, index, webInfo))
        return false;
    info.name = webInfo.name;
    info.type = webInfo.type;
    info.size = webInfo.size;
    return true;
}

GLint GraphicsContext3D::getAttribLocation(Platform3DObject program, const String& name)
{
    return m_impl->getAttribLocation(program, name.utf8().data());
}

GraphicsContext3D::Attributes GraphicsContext3D::getContextAttributes()
{
    blink::WebGraphicsContext3D::Attributes webAttributes = m_impl->getContextAttributes();
    GraphicsContext3D::Attributes attributes;
    attributes.alpha = webAttributes.alpha;
    attributes.depth = webAttributes.depth;
    attributes.stencil = webAttributes.stencil;
    attributes.antialias = webAttributes.antialias;
    attributes.premultipliedAlpha = webAttributes.premultipliedAlpha;
    attributes.preserveDrawingBuffer = m_preserveDrawingBuffer;
    attributes.preferDiscreteGPU = webAttributes.preferDiscreteGPU;
    return attributes;
}

DELEGATE_TO_WEBCONTEXT_R(getError, GLenum)
DELEGATE_TO_WEBCONTEXT_2(getIntegerv, GLenum, GLint*)
DELEGATE_TO_WEBCONTEXT_3(getProgramiv, Platform3DObject, GLenum, GLint*)
DELEGATE_TO_WEBCONTEXT_3(getShaderiv, Platform3DObject, GLenum, GLint*)
DELEGATE_TO_WEBCONTEXT_1R(getString, GLenum, String)

GLint GraphicsContext3D::getUniformLocation(Platform3DObject program, const String& name)
{
    return m_impl->getUniformLocation(program, name.utf8().data());
}

DELEGATE_TO_WEBCONTEXT_1(linkProgram, Platform3DObject)

void GraphicsContext3D::pixelStorei(GLenum pname, GLint param)
{
    if (pname == GL_PACK_ALIGNMENT)
        m_packAlignment = param;
    m_impl->pixelStorei(pname, param);
}

DELEGATE_TO_WEBCONTEXT_7(readPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*)

DELEGATE_TO_WEBCONTEXT_4(renderbufferStorage, GLenum, GLenum, GLsizei, GLsizei)

void GraphicsContext3D::shaderSource(Platform3DObject shader, const String& string)
{
    m_impl->shaderSource(shader, string.utf8().data());
}

DELEGATE_TO_WEBCONTEXT_2(stencilMaskSeparate, GLenum, GLuint)

DELEGATE_TO_WEBCONTEXT_9(texImage2D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)
DELEGATE_TO_WEBCONTEXT_3(texParameteri, GLenum, GLenum, GLint)

DELEGATE_TO_WEBCONTEXT_2(uniform1f, GLint, GLfloat)
DELEGATE_TO_WEBCONTEXT_3(uniform1fv, GLint, GLsizei, GLfloat*)
DELEGATE_TO_WEBCONTEXT_2(uniform1i, GLint, GLint)
DELEGATE_TO_WEBCONTEXT_3(uniform2f, GLint, GLfloat, GLfloat)
DELEGATE_TO_WEBCONTEXT_4(uniform3f, GLint, GLfloat, GLfloat, GLfloat)
DELEGATE_TO_WEBCONTEXT_5(uniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat)
DELEGATE_TO_WEBCONTEXT_4(uniformMatrix4fv, GLint, GLsizei, GLboolean, GLfloat*)

DELEGATE_TO_WEBCONTEXT_1(useProgram, Platform3DObject)

DELEGATE_TO_WEBCONTEXT_6(vertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, GLintptr)

DELEGATE_TO_WEBCONTEXT_4(viewport, GLint, GLint, GLsizei, GLsizei)

void GraphicsContext3D::markContextChanged()
{
    m_layerComposited = false;
}

bool GraphicsContext3D::layerComposited() const
{
    return m_layerComposited;
}

void GraphicsContext3D::markLayerComposited()
{
    m_layerComposited = true;
}

void GraphicsContext3D::paintRenderingResultsToCanvas(ImageBuffer* imageBuffer, DrawingBuffer* drawingBuffer)
{
    Platform3DObject framebufferId;
    int width, height;
    getDrawingParameters(drawingBuffer, m_impl, &framebufferId, &width, &height);
    paintFramebufferToCanvas(framebufferId, width, height, !getContextAttributes().premultipliedAlpha, imageBuffer);
}

PassRefPtr<Uint8ClampedArray> GraphicsContext3D::paintRenderingResultsToImageData(DrawingBuffer* drawingBuffer, int& width, int& height)
{
    if (getContextAttributes().premultipliedAlpha)
        return 0;

    Platform3DObject framebufferId;
    getDrawingParameters(drawingBuffer, m_impl, &framebufferId, &width, &height);

    Checked<int, RecordOverflow> dataSize = 4;
    dataSize *= width;
    dataSize *= height;
    if (dataSize.hasOverflowed())
        return 0;

    RefPtr<Uint8ClampedArray> pixels = Uint8ClampedArray::createUninitialized(width * height * 4);

    m_impl->bindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    readBackFramebuffer(pixels->data(), width, height, ReadbackRGBA, AlphaDoNothing);
    flipVertically(pixels->data(), width, height);

    return pixels.release();
}

void GraphicsContext3D::readBackFramebuffer(unsigned char* pixels, int width, int height, ReadbackOrder readbackOrder, AlphaOp op)
{
    if (m_packAlignment > 4)
        m_impl->pixelStorei(GL_PACK_ALIGNMENT, 1);
    m_impl->readPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (m_packAlignment > 4)
        m_impl->pixelStorei(GL_PACK_ALIGNMENT, m_packAlignment);

    size_t bufferSize = 4 * width * height;

    if (readbackOrder == ReadbackSkia) {
#if (SK_R32_SHIFT == 16) && !SK_B32_SHIFT
        // Swizzle red and blue channels to match SkBitmap's byte ordering.
        // TODO(kbr): expose GL_BGRA as extension.
        for (size_t i = 0; i < bufferSize; i += 4) {
            std::swap(pixels[i], pixels[i + 2]);
        }
#endif
    }

    if (op == AlphaDoPremultiply) {
        for (size_t i = 0; i < bufferSize; i += 4) {
            pixels[i + 0] = std::min(255, pixels[i + 0] * pixels[i + 3] / 255);
            pixels[i + 1] = std::min(255, pixels[i + 1] * pixels[i + 3] / 255);
            pixels[i + 2] = std::min(255, pixels[i + 2] * pixels[i + 3] / 255);
        }
    } else if (op != AlphaDoNothing) {
        ASSERT_NOT_REACHED();
    }
}

void GraphicsContext3D::setPackAlignment(GLint param)
{
    m_packAlignment = param;
}

DELEGATE_TO_WEBCONTEXT_R(createBuffer, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_R(createFramebuffer, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_R(createProgram, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_R(createRenderbuffer, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_1R(createShader, GLenum, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_R(createTexture, Platform3DObject)

DELEGATE_TO_WEBCONTEXT_1(deleteBuffer, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_1(deleteFramebuffer, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_1(deleteProgram, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_1(deleteRenderbuffer, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_1(deleteShader, Platform3DObject)
DELEGATE_TO_WEBCONTEXT_1(deleteTexture, Platform3DObject)

DELEGATE_TO_WEBCONTEXT_1(synthesizeGLError, GLenum)

Extensions3D* GraphicsContext3D::extensions()
{
    if (!m_extensions)
        m_extensions = adoptPtr(new Extensions3D(this));
    return m_extensions.get();
}

bool GraphicsContext3D::texImage2DResourceSafe(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLint unpackAlignment)
{
    ASSERT(unpackAlignment == 1 || unpackAlignment == 2 || unpackAlignment == 4 || unpackAlignment == 8);
    texImage2D(target, level, internalformat, width, height, border, format, type, 0);
    return true;
}

bool GraphicsContext3D::computeFormatAndTypeParameters(GLenum format, GLenum type, unsigned* componentsPerPixel, unsigned* bytesPerComponent)
{
    switch (format) {
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_STENCIL_OES:
        *componentsPerPixel = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        *componentsPerPixel = 2;
        break;
    case GL_RGB:
        *componentsPerPixel = 3;
        break;
    case GL_RGBA:
    case Extensions3D::BGRA_EXT: // GL_EXT_texture_format_BGRA8888
        *componentsPerPixel = 4;
        break;
    default:
        return false;
    }
    switch (type) {
    case GL_UNSIGNED_BYTE:
        *bytesPerComponent = sizeof(GLubyte);
        break;
    case GL_UNSIGNED_SHORT:
        *bytesPerComponent = sizeof(GLushort);
        break;
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        *componentsPerPixel = 1;
        *bytesPerComponent = sizeof(GLushort);
        break;
    case GL_UNSIGNED_INT_24_8_OES:
    case GL_UNSIGNED_INT:
        *bytesPerComponent = sizeof(GLuint);
        break;
    case GL_FLOAT: // OES_texture_float
        *bytesPerComponent = sizeof(GLfloat);
        break;
    case GL_HALF_FLOAT_OES: // OES_texture_half_float
        *bytesPerComponent = sizeof(GLushort);
        break;
    default:
        return false;
    }
    return true;
}

GLenum GraphicsContext3D::computeImageSizeInBytes(GLenum format, GLenum type, GLsizei width, GLsizei height, GLint alignment, unsigned* imageSizeInBytes, unsigned* paddingInBytes)
{
    ASSERT(imageSizeInBytes);
    ASSERT(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
    if (width < 0 || height < 0)
        return GL_INVALID_VALUE;
    unsigned bytesPerComponent, componentsPerPixel;
    if (!computeFormatAndTypeParameters(format, type, &bytesPerComponent, &componentsPerPixel))
        return GL_INVALID_ENUM;
    if (!width || !height) {
        *imageSizeInBytes = 0;
        if (paddingInBytes)
            *paddingInBytes = 0;
        return GL_NO_ERROR;
    }
    CheckedInt<uint32_t> checkedValue(bytesPerComponent * componentsPerPixel);
    checkedValue *=  width;
    if (!checkedValue.isValid())
        return GL_INVALID_VALUE;
    unsigned validRowSize = checkedValue.value();
    unsigned padding = 0;
    unsigned residual = validRowSize % alignment;
    if (residual) {
        padding = alignment - residual;
        checkedValue += padding;
    }
    // Last row needs no padding.
    checkedValue *= (height - 1);
    checkedValue += validRowSize;
    if (!checkedValue.isValid())
        return GL_INVALID_VALUE;
    *imageSizeInBytes = checkedValue.value();
    if (paddingInBytes)
        *paddingInBytes = padding;
    return GL_NO_ERROR;
}

GraphicsContext3D::ImageExtractor::ImageExtractor(Image* image, ImageHtmlDomSource imageHtmlDomSource, bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    m_image = image;
    m_imageHtmlDomSource = imageHtmlDomSource;
    m_extractSucceeded = extractImage(premultiplyAlpha, ignoreGammaAndColorProfile);
}

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
    if (m_skiaImage)
        m_skiaImage->bitmap().unlockPixels();
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    if (!m_image)
        return false;
    m_skiaImage = m_image->nativeImageForCurrentFrame();
    m_alphaOp = AlphaDoNothing;
    bool hasAlpha = m_skiaImage ? !m_skiaImage->bitmap().isOpaque() : true;
    if ((!m_skiaImage || ignoreGammaAndColorProfile || (hasAlpha && !premultiplyAlpha)) && m_image->data()) {
        // Attempt to get raw unpremultiplied image data.
        OwnPtr<ImageDecoder> decoder(ImageDecoder::create(
            *(m_image->data()), ImageSource::AlphaNotPremultiplied,
            ignoreGammaAndColorProfile ? ImageSource::GammaAndColorProfileIgnored : ImageSource::GammaAndColorProfileApplied));
        if (!decoder)
            return false;
        decoder->setData(m_image->data(), true);
        if (!decoder->frameCount())
            return false;
        ImageFrame* frame = decoder->frameBufferAtIndex(0);
        if (!frame || frame->status() != ImageFrame::FrameComplete)
            return false;
        hasAlpha = frame->hasAlpha();
        m_nativeImage = frame->asNewNativeImage();
        if (!m_nativeImage.get() || !m_nativeImage->isDataComplete() || !m_nativeImage->bitmap().width() || !m_nativeImage->bitmap().height())
            return false;
        SkBitmap::Config skiaConfig = m_nativeImage->bitmap().config();
        if (skiaConfig != SkBitmap::kARGB_8888_Config)
            return false;
        m_skiaImage = m_nativeImage.get();
        if (hasAlpha && premultiplyAlpha)
            m_alphaOp = AlphaDoPremultiply;
    } else if (!premultiplyAlpha && hasAlpha) {
        // 1. For texImage2D with HTMLVideoElment input, assume no PremultiplyAlpha had been applied and the alpha value for each pixel is 0xFF
        // which is true at present and may be changed in the future and needs adjustment accordingly.
        // 2. For texImage2D with HTMLCanvasElement input in which Alpha is already Premultiplied in this port,
        // do AlphaDoUnmultiply if UNPACK_PREMULTIPLY_ALPHA_WEBGL is set to false.
        if (m_imageHtmlDomSource != HtmlDomVideo)
            m_alphaOp = AlphaDoUnmultiply;
    }
    if (!m_skiaImage)
        return false;

    m_imageSourceFormat = SK_B32_SHIFT ? DataFormatRGBA8 : DataFormatBGRA8;
    m_imageWidth = m_skiaImage->bitmap().width();
    m_imageHeight = m_skiaImage->bitmap().height();
    if (!m_imageWidth || !m_imageHeight)
        return false;
    m_imageSourceUnpackAlignment = 0;
    m_skiaImage->bitmap().lockPixels();
    m_imagePixelData = m_skiaImage->bitmap().getPixels();
    return true;
}

unsigned GraphicsContext3D::getClearBitsByFormat(GLenum format)
{
    switch (format) {
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
    case GL_RGB:
    case GL_RGB565:
    case GL_RGBA:
    case GL_RGBA4:
    case GL_RGB5_A1:
        return GL_COLOR_BUFFER_BIT;
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT:
        return GL_DEPTH_BUFFER_BIT;
    case GL_STENCIL_INDEX8:
        return GL_STENCIL_BUFFER_BIT;
    case GL_DEPTH_STENCIL_OES:
        return GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    default:
        return 0;
    }
}

unsigned GraphicsContext3D::getChannelBitsByFormat(GLenum format)
{
    switch (format) {
    case GL_ALPHA:
        return ChannelAlpha;
    case GL_LUMINANCE:
        return ChannelRGB;
    case GL_LUMINANCE_ALPHA:
        return ChannelRGBA;
    case GL_RGB:
    case GL_RGB565:
        return ChannelRGB;
    case GL_RGBA:
    case GL_RGBA4:
    case GL_RGB5_A1:
        return ChannelRGBA;
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT:
        return ChannelDepth;
    case GL_STENCIL_INDEX8:
        return ChannelStencil;
    case GL_DEPTH_STENCIL_OES:
        return ChannelDepth | ChannelStencil;
    default:
        return 0;
    }
}

void GraphicsContext3D::paintFramebufferToCanvas(int framebuffer, int width, int height, bool premultiplyAlpha, ImageBuffer* imageBuffer)
{
    unsigned char* pixels = 0;

    const SkBitmap* canvasBitmap = imageBuffer->context()->bitmap();
    const SkBitmap* readbackBitmap = 0;
    ASSERT(canvasBitmap->config() == SkBitmap::kARGB_8888_Config);
    if (canvasBitmap->width() == width && canvasBitmap->height() == height) {
        // This is the fastest and most common case. We read back
        // directly into the canvas's backing store.
        readbackBitmap = canvasBitmap;
        m_resizingBitmap.reset();
    } else {
        // We need to allocate a temporary bitmap for reading back the
        // pixel data. We will then use Skia to rescale this bitmap to
        // the size of the canvas's backing store.
        if (m_resizingBitmap.width() != width || m_resizingBitmap.height() != height) {
            m_resizingBitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
            if (!m_resizingBitmap.allocPixels())
                return;
        }
        readbackBitmap = &m_resizingBitmap;
    }

    // Read back the frame buffer.
    SkAutoLockPixels bitmapLock(*readbackBitmap);
    pixels = static_cast<unsigned char*>(readbackBitmap->getPixels());

    m_impl->bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    readBackFramebuffer(pixels, width, height, ReadbackSkia, premultiplyAlpha ? AlphaDoPremultiply : AlphaDoNothing);
    flipVertically(pixels, width, height);

    readbackBitmap->notifyPixelsChanged();
    if (m_resizingBitmap.readyToDraw()) {
        // We need to draw the resizing bitmap into the canvas's backing store.
        SkCanvas canvas(*canvasBitmap);
        SkRect dst;
        dst.set(SkIntToScalar(0), SkIntToScalar(0), SkIntToScalar(canvasBitmap->width()), SkIntToScalar(canvasBitmap->height()));
        canvas.drawBitmapRect(m_resizingBitmap, 0, dst);
    }
}

namespace {

void splitStringHelper(const String& str, HashSet<String>& set)
{
    Vector<String> substrings;
    str.split(" ", substrings);
    for (size_t i = 0; i < substrings.size(); ++i)
        set.add(substrings[i]);
}

String mapExtensionName(const String& name)
{
    if (name == "GL_ANGLE_framebuffer_blit"
        || name == "GL_ANGLE_framebuffer_multisample")
        return "GL_CHROMIUM_framebuffer_multisample";
    return name;
}

} // anonymous namespace

void GraphicsContext3D::initializeExtensions()
{
    if (m_initializedAvailableExtensions)
        return;

    m_initializedAvailableExtensions = true;
    bool success = m_impl->makeContextCurrent();
    ASSERT(success);
    if (!success)
        return;

    String extensionsString = m_impl->getString(GL_EXTENSIONS);
    splitStringHelper(extensionsString, m_enabledExtensions);

    String requestableExtensionsString = m_impl->getRequestableExtensionsCHROMIUM();
    splitStringHelper(requestableExtensionsString, m_requestableExtensions);
}


bool GraphicsContext3D::supportsExtension(const String& name)
{
    initializeExtensions();
    String mappedName = mapExtensionName(name);
    return m_enabledExtensions.contains(mappedName) || m_requestableExtensions.contains(mappedName);
}

bool GraphicsContext3D::ensureExtensionEnabled(const String& name)
{
    initializeExtensions();

    String mappedName = mapExtensionName(name);
    if (m_enabledExtensions.contains(mappedName))
        return true;

    if (m_requestableExtensions.contains(mappedName)) {
        m_impl->requestExtensionCHROMIUM(mappedName.ascii().data());
        m_enabledExtensions.clear();
        m_requestableExtensions.clear();
        m_initializedAvailableExtensions = false;
    }

    initializeExtensions();
    fprintf(stderr, "m_enabledExtensions.contains(%s) == %d\n", mappedName.ascii().data(), m_enabledExtensions.contains(mappedName));
    return m_enabledExtensions.contains(mappedName);
}

bool GraphicsContext3D::isExtensionEnabled(const String& name)
{
    initializeExtensions();
    String mappedName = mapExtensionName(name);
    return m_enabledExtensions.contains(mappedName);
}

void GraphicsContext3D::flipVertically(uint8_t* framebuffer, int width, int height)
{
    m_scanline.resize(width * 4);
    uint8* scanline = &m_scanline[0];
    unsigned rowBytes = width * 4;
    unsigned count = height / 2;
    for (unsigned i = 0; i < count; i++) {
        uint8* rowA = framebuffer + i * rowBytes;
        uint8* rowB = framebuffer + (height - i - 1) * rowBytes;
        memcpy(scanline, rowB, rowBytes);
        memcpy(rowB, rowA, rowBytes);
        memcpy(rowA, scanline, rowBytes);
    }
}

} // namespace WebCore
