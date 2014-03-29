// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RemoteFrame_h
#define RemoteFrame_h

#include "core/frame/Frame.h"

namespace WebCore {

class RemoteFrameView;

class RemoteFrame: public Frame {
public:
    static PassRefPtr<RemoteFrame> create(FrameHost*, HTMLFrameOwnerElement*);
    virtual bool isRemoteFrame() const OVERRIDE { return true; }

    virtual ~RemoteFrame();

    void setView(PassRefPtr<RemoteFrameView>);
    void createView();

private:
    RemoteFrame(FrameHost*, HTMLFrameOwnerElement*);

    RefPtr<RemoteFrameView> m_view;
};

DEFINE_TYPE_CASTS(RemoteFrame, Frame, remoteFrame, remoteFrame->isRemoteFrame(), remoteFrame.isRemoteFrame());

} // namespace WebCore

#endif // RemoteFrame_h
