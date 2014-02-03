// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "cc/output/context_provider.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/skia/include/core/SkXfermode.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/root_window.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/compositor_observer.h"
#include "ui/compositor/debug_utils.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/skia_util.h"
#include "ui/gl/gl_surface.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include "third_party/khronos/GLES2/gl2ext.h"

#if defined(USE_X11)
#include "base/message_loop/message_pump_x11.h"
#endif

using base::TimeTicks;
using ui::Compositor;
using ui::Layer;
using ui::LayerDelegate;

namespace {

class ColoredLayer : public Layer, public LayerDelegate {
 public:
  explicit ColoredLayer(SkColor color)
      : Layer(ui::LAYER_TEXTURED),
        color_(color),
        draw_(true) {
    set_delegate(this);
  }

  virtual ~ColoredLayer() {}

  // Overridden from LayerDelegate:
  virtual void OnPaintLayer(gfx::Canvas* canvas) OVERRIDE {
    if (draw_) {
      canvas->DrawColor(color_);
    }
  }

  virtual void OnDeviceScaleFactorChanged(float device_scale_factor) OVERRIDE {
  }

  virtual base::Closure PrepareForLayerBoundsChange() OVERRIDE {
    return base::Closure();
  }

  void set_color(SkColor color) { color_ = color; }
  void set_draw(bool draw) { draw_ = draw; }

 private:
  SkColor color_;
  bool draw_;

  DISALLOW_COPY_AND_ASSIGN(ColoredLayer);
};

const int kFrames = 100;

// Benchmark base class, hooks up drawing callback and displaying FPS.
class BenchCompositorObserver : public ui::CompositorObserver {
 public:
  explicit BenchCompositorObserver(int max_frames)
      : start_time_(),
        frames_(0),
        max_frames_(max_frames) {
  }

  virtual void OnCompositingDidCommit(ui::Compositor* compositor) OVERRIDE {}

  virtual void OnCompositingStarted(Compositor* compositor,
                                    base::TimeTicks start_time) OVERRIDE {}

  virtual void OnCompositingEnded(Compositor* compositor) OVERRIDE {
    if (start_time_.is_null()) {
      start_time_ = TimeTicks::Now();
    } else {
      ++frames_;
      if (frames_ % kFrames == 0) {
        TimeTicks now = TimeTicks::Now();
        double ms = (now - start_time_).InMillisecondsF() / kFrames;
        LOG(INFO) << "FPS: " << 1000.f / ms << " (" << ms << " ms)";
        start_time_ = now;
      }
    }
    if (max_frames_ && frames_ == max_frames_) {
      base::MessageLoop::current()->Quit();
    } else {
      Draw();
    }
  }

  virtual void OnCompositingAborted(Compositor* compositor) OVERRIDE {}

  virtual void OnCompositingLockStateChanged(
      Compositor* compositor) OVERRIDE {}

  virtual void OnUpdateVSyncParameters(ui::Compositor* compositor,
                                       base::TimeTicks timebase,
                                       base::TimeDelta interval) OVERRIDE {
  }

  virtual void Draw() {}

  int frames() const { return frames_; }

 private:
  TimeTicks start_time_;
  int frames_;
  int max_frames_;

  DISALLOW_COPY_AND_ASSIGN(BenchCompositorObserver);
};

class WebGLTexture : public ui::Texture {
 public:
  WebGLTexture(gpu::gles2::GLES2Interface* gl, const gfx::Size& size)
      : ui::Texture(false, size, 1.0f),
        gl_(gl),
        texture_id_(0u) {
    gl->GenTextures(1, &texture_id_);
    gl->BindTexture(GL_TEXTURE_2D, texture_id_);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(),
                   0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  virtual unsigned int PrepareTexture() OVERRIDE {
    return texture_id_;
  }

 private:
  virtual ~WebGLTexture() {
    gl_->DeleteTextures(1, &texture_id_);
  }

  gpu::gles2::GLES2Interface* gl_;
  GLuint texture_id_;

  DISALLOW_COPY_AND_ASSIGN(WebGLTexture);
};

// A benchmark that adds a texture layer that is updated every frame.
class WebGLBench : public BenchCompositorObserver {
 public:
  WebGLBench(Layer* parent, Compositor* compositor, int max_frames)
      : BenchCompositorObserver(max_frames),
        parent_(parent),
        webgl_(ui::LAYER_TEXTURED),
        compositor_(compositor),
        texture_(),
        fbo_(0),
        do_draw_(true) {
    CommandLine* command_line = CommandLine::ForCurrentProcess();
    do_draw_ = !command_line->HasSwitch("disable-draw");

    std::string webgl_size = command_line->GetSwitchValueASCII("webgl-size");
    int width = 0;
    int height = 0;
    if (!webgl_size.empty()) {
      std::vector<std::string> split_size;
      base::SplitString(webgl_size, 'x', &split_size);
      if (split_size.size() == 2) {
        width = atoi(split_size[0].c_str());
        height = atoi(split_size[1].c_str());
      }
    }
    if (!width || !height) {
      width = 800;
      height = 600;
    }
    gfx::Rect bounds(width, height);
    webgl_.SetBounds(bounds);
    parent_->Add(&webgl_);

    context_provider_ =
        ui::ContextFactory::GetInstance()->SharedMainThreadContextProvider();
    gpu::gles2::GLES2Interface* gl = context_provider_->ContextGL();
    texture_ = new WebGLTexture(gl, bounds.size());
    gl->GenFramebuffers(1, &fbo_);
    compositor->AddObserver(this);
    webgl_.SetExternalTexture(texture_.get());
    gl->BindFramebuffer(GL_FRAMEBUFFER, fbo_);
    gl->FramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, texture_->PrepareTexture(), 0);
    gl->ClearColor(0.f, 1.f, 0.f, 1.f);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    gl->Flush();
  }

  virtual ~WebGLBench() {
    context_provider_->ContextGL()->DeleteFramebuffers(1, &fbo_);
    webgl_.SetShowPaintedContent();
    texture_ = NULL;
    compositor_->RemoveObserver(this);
  }

  virtual void Draw() OVERRIDE {
    if (do_draw_) {
      gpu::gles2::GLES2Interface* gl = context_provider_->ContextGL();
      gl->ClearColor((frames() % kFrames)*1.0/kFrames, 1.f, 0.f, 1.f);
      gl->Clear(GL_COLOR_BUFFER_BIT);
      gl->Flush();
    }
    webgl_.SetExternalTexture(texture_.get());
    webgl_.SchedulePaint(gfx::Rect(webgl_.bounds().size()));
    compositor_->ScheduleDraw();
  }

 private:
  Layer* parent_;
  Layer webgl_;
  Compositor* compositor_;
  scoped_refptr<cc::ContextProvider> context_provider_;
  scoped_refptr<WebGLTexture> texture_;

  // The FBO that is used to render to the texture.
  unsigned int fbo_;

  // Whether or not to draw to the texture every frame.
  bool do_draw_;

  DISALLOW_COPY_AND_ASSIGN(WebGLBench);
};

// A benchmark that paints (in software) all tiles every frame.
class SoftwareScrollBench : public BenchCompositorObserver {
 public:
  SoftwareScrollBench(ColoredLayer* layer,
                      Compositor* compositor,
                      int max_frames)
      : BenchCompositorObserver(max_frames),
        layer_(layer),
        compositor_(compositor) {
    compositor->AddObserver(this);
    layer_->set_draw(
        !CommandLine::ForCurrentProcess()->HasSwitch("disable-draw"));
  }

  virtual ~SoftwareScrollBench() {
    compositor_->RemoveObserver(this);
  }

  virtual void Draw() OVERRIDE {
    layer_->set_color(
        SkColorSetARGBInline(255*(frames() % kFrames)/kFrames, 255, 0, 255));
    layer_->SchedulePaint(gfx::Rect(layer_->bounds().size()));
  }

 private:
  ColoredLayer* layer_;
  Compositor* compositor_;

  DISALLOW_COPY_AND_ASSIGN(SoftwareScrollBench);
};

}  // namespace

int main(int argc, char** argv) {
  CommandLine::Init(argc, argv);

  base::AtExitManager exit_manager;

  gfx::GLSurface::InitializeOneOff();

  // The ContextFactory must exist before any Compositors are created.
  bool allow_test_contexts = false;
  ui::InitializeContextFactoryForTests(allow_test_contexts);

  base::i18n::InitializeICU();

  base::MessageLoopForUI message_loop;
  aura::Env::CreateInstance();
  scoped_ptr<aura::TestScreen> test_screen(
      aura::TestScreen::CreateFullscreen());
  gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE, test_screen.get());
  scoped_ptr<aura::RootWindow> root_window(
      test_screen->CreateRootWindowForPrimaryDisplay());
  aura::client::SetCaptureClient(
      root_window->window(),
      new aura::client::DefaultCaptureClient(root_window->window()));

  scoped_ptr<aura::client::FocusClient> focus_client(
      new aura::test::TestFocusClient);
  aura::client::SetFocusClient(root_window->window(), focus_client.get());

  // add layers
  ColoredLayer background(SK_ColorRED);
  background.SetBounds(root_window->window()->bounds());
  root_window->window()->layer()->Add(&background);

  ColoredLayer window(SK_ColorBLUE);
  window.SetBounds(gfx::Rect(background.bounds().size()));
  background.Add(&window);

  Layer content_layer(ui::LAYER_NOT_DRAWN);

  CommandLine* command_line = CommandLine::ForCurrentProcess();
  bool force = command_line->HasSwitch("force-render-surface");
  content_layer.SetForceRenderSurface(force);
  gfx::Rect bounds(window.bounds().size());
  bounds.Inset(0, 30, 0, 0);
  content_layer.SetBounds(bounds);
  window.Add(&content_layer);

  ColoredLayer page_background(SK_ColorWHITE);
  page_background.SetBounds(gfx::Rect(content_layer.bounds().size()));
  content_layer.Add(&page_background);

  int frames = atoi(command_line->GetSwitchValueASCII("frames").c_str());
  scoped_ptr<BenchCompositorObserver> bench;

  if (command_line->HasSwitch("bench-software-scroll")) {
    bench.reset(new SoftwareScrollBench(&page_background,
                                        root_window->host()->compositor(),
                                        frames));
  } else {
    bench.reset(new WebGLBench(&page_background,
                               root_window->host()->compositor(),
                               frames));
  }

#ifndef NDEBUG
  ui::PrintLayerHierarchy(root_window->window()->layer(), gfx::Point(100, 100));
#endif

  root_window->host()->Show();
  base::MessageLoopForUI::current()->Run();
  focus_client.reset();
  root_window.reset();

  return 0;
}
