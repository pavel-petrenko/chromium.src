// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebVideoLayerImpl_h
#define WebVideoLayerImpl_h

#include <public/WebVideoLayer.h>

namespace WebCore {
class VideoLayerChromium;
}

namespace WebKit {
class WebLayerImpl;

class WebVideoLayerImpl : public WebVideoLayer {
public:
    explicit WebVideoLayerImpl(PassRefPtr<WebCore::VideoLayerChromium>);
    virtual ~WebVideoLayerImpl();

    // WebVideoLayer implementation.
    virtual WebLayer* layer() OVERRIDE;
    virtual bool active() const OVERRIDE;

private:
    OwnPtr<WebLayerImpl> m_layer;
};

}

#endif // WebVideoLayerImpl_h

