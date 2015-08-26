// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/view_manager/public/cpp/view_manager_init.h"

#include "components/view_manager/public/cpp/lib/view_tree_client_impl.h"
#include "components/view_manager/public/cpp/view_tree_delegate.h"
#include "mojo/application/public/cpp/application_impl.h"

namespace mojo {

class ViewManagerInit::ClientFactory
    : public InterfaceFactory<ViewTreeClient> {
 public:
  ClientFactory(ViewManagerInit* init) : init_(init) {}
  ~ClientFactory() override {}

  // InterfaceFactory<ViewTreeClient> implementation.
  void Create(ApplicationConnection* connection,
              InterfaceRequest<ViewTreeClient> request) override {
    init_->OnCreate(request.Pass());
  }

 private:
  ViewManagerInit* init_;

  DISALLOW_COPY_AND_ASSIGN(ClientFactory);
};

ViewManagerInit::ViewManagerInit(ApplicationImpl* app,
                                 ViewTreeDelegate* delegate,
                                 ViewManagerRootClient* root_client)
    : app_(app),
      connection_(nullptr),
      delegate_(delegate),
      client_factory_(new ClientFactory(this)) {
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = mojo::String::From("mojo:view_manager");
  connection_ = app_->ConnectToApplication(request.Pass());

  // The view_manager will request a ViewTreeClient service for each
  // ViewManagerRoot created.
  connection_->AddService<ViewTreeClient>(client_factory_.get());
  connection_->ConnectToService(&view_manager_root_);

  if (root_client) {
    root_client_binding_.reset(new Binding<ViewManagerRootClient>(root_client));
    ViewManagerRootClientPtr root_client_ptr;
    root_client_binding_->Bind(GetProxy(&root_client_ptr));
    view_manager_root_->SetViewManagerRootClient(root_client_ptr.Pass());
  }
}

ViewManagerInit::~ViewManagerInit() {}

void ViewManagerInit::OnCreate(InterfaceRequest<ViewTreeClient> request) {
  // TODO(sky): straighten out lifetime.
  ViewTreeConnection::Create(delegate_, request.Pass());
}

}  // namespace mojo
