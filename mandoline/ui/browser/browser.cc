// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mandoline/ui/browser/browser.h"

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "components/view_manager/public/cpp/view.h"
#include "components/view_manager/public/cpp/view_manager_init.h"
#include "mandoline/tab/frame.h"
#include "mandoline/tab/frame_tree.h"
#include "mandoline/ui/browser/browser_ui.h"
#include "mandoline/ui/browser/merged_service_provider.h"
#include "mojo/application/public/cpp/application_runner.h"
#include "mojo/common/common_type_converters.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "third_party/mojo/src/mojo/public/c/system/main.h"
#include "ui/gfx/geometry/size.h"

namespace mandoline {
namespace {

gfx::Size GetInitialViewportSize() {
#if defined(OS_ANDROID)
  // Resize to match the Nexus 5 aspect ratio:
  return gfx::Size(320, 640);
#else
  return gfx::Size(1280, 800);
#endif
}

}  // namespace

Browser::Browser()
    : root_(nullptr),
      content_(nullptr),
      omnibox_(nullptr),
      navigator_host_(this),
      app_(nullptr) {
  exposed_services_impl_.AddService<mojo::NavigatorHost>(this);
}

Browser::~Browser() {
  // Destruct ui_ manually while |this| is alive and reset the pointer first.
  // This is to avoid a double delete when OnViewManagerDisconnected gets
  // called.
  delete ui_.release();
}

void Browser::ReplaceContentWithRequest(mojo::URLRequestPtr request) {
  Embed(request.Pass(), nullptr, nullptr);
}

void Browser::Initialize(mojo::ApplicationImpl* app) {
  app_ = app;
  view_manager_init_.reset(new mojo::ViewManagerInit(app, this, this));

  ui_.reset(BrowserUI::Create(this, app));

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::CommandLine::StringVector args = command_line->GetArgs();
  if (args.empty()) {
    default_url_ = "http://www.google.com/";
  } else {
#if defined(OS_WIN)
    default_url_ = base::WideToUTF8(args[0]);
#else
    default_url_ = args[0];
#endif
  }
}

bool Browser::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  // TODO: register embed interface here.
  return true;
}

bool Browser::ConfigureOutgoingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<ViewEmbedder>(this);
  return true;
}

void Browser::OnEmbed(
    mojo::View* root,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  // Browser does not support being embedded more than once.
  CHECK(!root_);

  // Make it so we get OnWillEmbed() for any Embed()s done by other apps we
  // Embed().
  root->view_manager()->SetEmbedRoot();

  // TODO(beng): still unhappy with the fact that both this class & the UI class
  //             know so much about these views. Figure out how to shift more to
  //             the UI class.
  root_ = root;
  content_ = root->view_manager()->CreateView();
  ui_->Init(root_);

  view_manager_init_->view_manager_root()->SetViewportSize(
      mojo::Size::From(GetInitialViewportSize()));

  root_->AddChild(content_);
  content_->SetVisible(true);

  view_manager_init_->view_manager_root()->AddAccelerator(
      mojo::KEYBOARD_CODE_BROWSER_BACK, mojo::EVENT_FLAGS_NONE);

  // Now that we're ready, either load a pending url or the default url.
  if (pending_request_) {
    Embed(pending_request_.Pass(), services.Pass(), exposed_services.Pass());
  } else if (!default_url_.empty()) {
    mojo::URLRequestPtr request(mojo::URLRequest::New());
    request->url = mojo::String::From(default_url_);
    Embed(request.Pass(), services.Pass(), exposed_services.Pass());
  }
}

bool Browser::OnWillEmbed(
    mojo::View* view,
    mojo::InterfaceRequest<mojo::ServiceProvider>* services,
    mojo::ServiceProviderPtr* exposed_services) {
  // TODO(sky): move this to Frame/FrameTree.
  Frame* frame = Frame::FindFirstFrameAncestor(view);
  if (!frame) {
    // TODO(sky): add requestor url so that we can return false if it's not
    // an app we expect.
    return true;
  }

  Frame* parent = frame;
  if (frame->view() == view) {
    parent = frame->parent();
    // This is a reembed.
    delete frame;
    frame = nullptr;
  }

  Frame* child_frame = new Frame(frame->tree(), view, ViewOwnership::OWNS_VIEW,
                                 services, exposed_services);
  parent->Add(child_frame);
  return true;
}

void Browser::OnViewManagerDisconnected(
    mojo::ViewManager* view_manager) {
  ui_.reset();
  root_ = nullptr;
  app_->Terminate();
}

void Browser::OnAccelerator(mojo::EventPtr event) {
  DCHECK_EQ(mojo::KEYBOARD_CODE_BROWSER_BACK,
            event->key_data->windows_key_code);
  navigator_host_.RequestNavigateHistory(-1);
}

void Browser::OpenURL(const mojo::String& url) {
  omnibox_->SetVisible(false);
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = mojo::String::From(url);
  ReplaceContentWithRequest(request.Pass());
}

void Browser::Embed(mojo::URLRequestPtr request,
                    mojo::InterfaceRequest<mojo::ServiceProvider> services,
                    mojo::ServiceProviderPtr exposed_services) {
  std::string string_url = request->url.To<std::string>();
  if (string_url == "mojo:omnibox") {
    ShowOmnibox(request.Pass(), services.Pass(), exposed_services.Pass());
    return;
  }

  // We can get Embed calls before we've actually been
  // embedded into the root view and content_ is created.
  // Just save the last url, we'll embed it when we're ready.
  if (!content_) {
    pending_request_ = request.Pass();
    return;
  }

  GURL gurl(string_url);
  bool changed = current_url_ != gurl;
  current_url_ = gurl;
  if (changed)
    ui_->OnURLChanged();

  // TODO(sky): move serviceproviders to frame tree.
  frame_tree_.reset(new FrameTree(content_));
  merged_service_provider_.reset(
      new MergedServiceProvider(exposed_services.Pass(), this));
  content_->Embed(request.Pass(), services.Pass(),
                  merged_service_provider_->GetServiceProviderPtr().Pass());

  navigator_host_.RecordNavigation(gurl.spec());
}

void Browser::Create(mojo::ApplicationConnection* connection,
                     mojo::InterfaceRequest<mojo::NavigatorHost> request) {
  navigator_host_.Bind(request.Pass());
}

void Browser::Create(mojo::ApplicationConnection* connection,
                     mojo::InterfaceRequest<ViewEmbedder> request) {
  view_embedder_bindings_.AddBinding(this, request.Pass());
}

void Browser::ShowOmnibox(
    mojo::URLRequestPtr request,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  if (!omnibox_) {
    omnibox_ = root_->view_manager()->CreateView();
    root_->AddChild(omnibox_);
    omnibox_->SetVisible(true);
    omnibox_->SetBounds(root_->bounds());
  }
  omnibox_->Embed(request.Pass(), services.Pass(), exposed_services.Pass());
}

}  // namespace mandoline
