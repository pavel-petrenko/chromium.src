// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MANDOLINE_TAB_FRAME_CONNECTION_H_
#define MANDOLINE_TAB_FRAME_CONNECTION_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "components/view_manager/public/interfaces/view_tree.mojom.h"
#include "mandoline/tab/frame_tree_delegate.h"
#include "mandoline/tab/frame_user_data.h"
#include "mandoline/tab/public/interfaces/frame_tree.mojom.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"

namespace mojo {
class ApplicationConnection;
class ApplicationImpl;
}

namespace mandoline {

class Frame;

// FrameConnection is a FrameUserData that manages the connection to a
// particular frame. It is also responsible for obtaining the necessary
// services from the remote side.
class FrameConnection : public FrameUserData {
 public:
  FrameConnection();
  ~FrameConnection() override;

  // Creates a FrameConnection to service a call from
  // FrameTreeDelegate::CanNavigateFrame(). |callback| is run once the
  // content handler id for the app is determined.
  static void CreateConnectionForCanNavigateFrame(
      mojo::ApplicationImpl* app,
      Frame* frame,
      mojo::URLRequestPtr request,
      const FrameTreeDelegate::CanNavigateFrameCallback& callback);

  // Initializes the FrameConnection with the specified parameters.
  // |on_got_id_callback| is run once the content handler is obtained from
  // the connection.
  void Init(mojo::ApplicationImpl* app,
            mojo::URLRequestPtr request,
            const base::Closure& on_got_id_callback);

  FrameTreeClient* frame_tree_client() { return frame_tree_client_.get(); }

  mojo::ApplicationConnection* application_connection() {
    return application_connection_.get();
  }

  // Asks the remote application for a ViewTreeClient.
  mojo::ViewTreeClientPtr GetViewTreeClient();

  // Returns the id of the content handler app. Only available once the callback
  // has run.
  uint32_t GetContentHandlerID() const;

 private:
  FrameTreeClientPtr frame_tree_client_;

  // TODO(sky): needs to be destroyed when connection lost.
  scoped_ptr<mojo::ApplicationConnection> application_connection_;

  DISALLOW_COPY_AND_ASSIGN(FrameConnection);
};

}  // namespace mandoline

#endif  // MANDOLINE_TAB_FRAME_CONNECTION_H_
