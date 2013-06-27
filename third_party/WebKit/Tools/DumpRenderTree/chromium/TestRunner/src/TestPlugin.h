/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef TestPlugin_h
#define TestPlugin_h

#include "public/platform/WebExternalTextureLayer.h"
#include "public/platform/WebExternalTextureLayerClient.h"
#include "public/web/WebPlugin.h"
#include "public/web/WebPluginContainer.h"
#include <memory>
#include <string>

namespace WebTestRunner {

class WebTestDelegate;

// A fake implemention of WebKit::WebPlugin for testing purposes.
//
// It uses WebGraphicsContext3D to paint a scene consisiting of a primitive
// over a background. The primitive and background can be customized using
// the following plugin parameters:
// primitive: none (default), triangle.
// background-color: black (default), red, green, blue.
// primitive-color: black (default), red, green, blue.
// opacity: [0.0 - 1.0]. Default is 1.0.
//
// Whether the plugin accepts touch events or not can be customized using the
// 'accepts-touch' plugin parameter (defaults to false).
class TestPlugin : public WebKit::WebPlugin, public WebKit::WebExternalTextureLayerClient {
public:
    static TestPlugin* create(WebKit::WebFrame*, const WebKit::WebPluginParams&, WebTestDelegate*);
    virtual ~TestPlugin();

    static const WebKit::WebString& mimeType();

    // WebPlugin methods:
    virtual bool initialize(WebKit::WebPluginContainer*);
    virtual void destroy();
    virtual NPObject* scriptableObject() { return 0; }
    virtual struct _NPP* pluginNPP() { return 0; }
    virtual bool canProcessDrag() const { return m_canProcessDrag; }
    virtual void paint(WebKit::WebCanvas*, const WebKit::WebRect&) { }
    virtual void updateGeometry(const WebKit::WebRect& frameRect, const WebKit::WebRect& clipRect, const WebKit::WebVector<WebKit::WebRect>& cutOutsRects, bool isVisible);
    virtual void updateFocus(bool) { }
    virtual void updateVisibility(bool) { }
    virtual bool acceptsInputEvents() { return true; }
    virtual bool handleInputEvent(const WebKit::WebInputEvent&, WebKit::WebCursorInfo&);
    virtual bool handleDragStatusUpdate(WebKit::WebDragStatus, const WebKit::WebDragData&, WebKit::WebDragOperationsMask, const WebKit::WebPoint& position, const WebKit::WebPoint& screenPosition);
    virtual void didReceiveResponse(const WebKit::WebURLResponse&) { }
    virtual void didReceiveData(const char* data, int dataLength) { }
    virtual void didFinishLoading() { }
    virtual void didFailLoading(const WebKit::WebURLError&) { }
    virtual void didFinishLoadingFrameRequest(const WebKit::WebURL&, void* notifyData) { }
    virtual void didFailLoadingFrameRequest(const WebKit::WebURL&, void* notifyData, const WebKit::WebURLError&) { }
    virtual bool isPlaceholder() { return false; }

    // WebExternalTextureLayerClient methods:
    virtual unsigned prepareTexture(WebKit::WebTextureUpdater&) { return m_colorTexture; }
    virtual WebKit::WebGraphicsContext3D* context() { return m_context; }
    virtual bool prepareMailbox(WebKit::WebExternalTextureMailbox*) { return false; };
    virtual void mailboxReleased(const WebKit::WebExternalTextureMailbox&) { }

private:
    TestPlugin(WebKit::WebFrame*, const WebKit::WebPluginParams&, WebTestDelegate*);

    enum Primitive {
        PrimitiveNone,
        PrimitiveTriangle
    };

    struct Scene {
        Primitive primitive;
        unsigned backgroundColor[3];
        unsigned primitiveColor[3];
        float opacity;

        unsigned vbo;
        unsigned program;
        int colorLocation;
        int positionLocation;

        Scene()
            : primitive(PrimitiveNone)
            , opacity(1.0f) // Fully opaque.
            , vbo(0)
            , program(0)
            , colorLocation(-1)
            , positionLocation(-1)
        {
            backgroundColor[0] = backgroundColor[1] = backgroundColor[2] = 0;
            primitiveColor[0] = primitiveColor[1] = primitiveColor[2] = 0;
        }
    };

    // Functions for parsing plugin parameters.
    Primitive parsePrimitive(const WebKit::WebString&);
    void parseColor(const WebKit::WebString&, unsigned color[3]);
    float parseOpacity(const WebKit::WebString&);
    bool parseBoolean(const WebKit::WebString&);

    // Functions for loading and drawing scene.
    bool initScene();
    void drawScene();
    void destroyScene();
    bool initProgram();
    bool initPrimitive();
    void drawPrimitive();
    unsigned loadShader(unsigned type, const std::string& source);
    unsigned loadProgram(const std::string& vertexSource, const std::string& fragmentSource);

    WebKit::WebFrame* m_frame;
    WebTestDelegate* m_delegate;
    WebKit::WebPluginContainer* m_container;

    WebKit::WebRect m_rect;
    WebKit::WebGraphicsContext3D* m_context;
    unsigned m_colorTexture;
    unsigned m_framebuffer;
    Scene m_scene;
    std::auto_ptr<WebKit::WebExternalTextureLayer> m_layer;

    WebKit::WebPluginContainer::TouchEventRequestType m_touchEventRequest;
    // Requests touch events from the WebPluginContainerImpl multiple times to tickle webkit.org/b/108381
    bool m_reRequestTouchEvents;
    bool m_printEventDetails;
    bool m_printUserGestureStatus;
    bool m_canProcessDrag;
};

}

#endif // TestPlugin_h
