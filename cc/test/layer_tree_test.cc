// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/layer_tree_test.h"

#include "base/command_line.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_registrar.h"
#include "cc/animation/layer_animation_controller.h"
#include "cc/animation/timing_function.h"
#include "cc/base/switches.h"
#include "cc/input/input_handler.h"
#include "cc/layers/content_layer.h"
#include "cc/layers/layer.h"
#include "cc/layers/layer_impl.h"
#include "cc/test/animation_test_common.h"
#include "cc/test/fake_layer_tree_host_client.h"
#include "cc/test/fake_output_surface.h"
#include "cc/test/occlusion_tracker_test_common.h"
#include "cc/test/test_context_provider.h"
#include "cc/test/tiled_layer_test_common.h"
#include "cc/trees/layer_tree_host_client.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/layer_tree_host_single_thread_client.h"
#include "cc/trees/layer_tree_impl.h"
#include "cc/trees/single_thread_proxy.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/gfx/frame_time.h"
#include "ui/gfx/size_conversions.h"

namespace cc {

TestHooks::TestHooks() {}

TestHooks::~TestHooks() {}

DrawSwapReadbackResult::DrawResult TestHooks::PrepareToDrawOnThread(
    LayerTreeHostImpl* host_impl,
    LayerTreeHostImpl::FrameData* frame_data,
    DrawSwapReadbackResult::DrawResult draw_result) {
  return draw_result;
}

base::TimeDelta TestHooks::LowFrequencyAnimationInterval() const {
  return base::TimeDelta::FromMilliseconds(16);
}

// Adapts LayerTreeHostImpl for test. Runs real code, then invokes test hooks.
class LayerTreeHostImplForTesting : public LayerTreeHostImpl {
 public:
  static scoped_ptr<LayerTreeHostImplForTesting> Create(
      TestHooks* test_hooks,
      const LayerTreeSettings& settings,
      LayerTreeHostImplClient* host_impl_client,
      Proxy* proxy,
      RenderingStatsInstrumentation* stats_instrumentation) {
    return make_scoped_ptr(new LayerTreeHostImplForTesting(
        test_hooks, settings, host_impl_client, proxy, stats_instrumentation));
  }

 protected:
  LayerTreeHostImplForTesting(
      TestHooks* test_hooks,
      const LayerTreeSettings& settings,
      LayerTreeHostImplClient* host_impl_client,
      Proxy* proxy,
      RenderingStatsInstrumentation* stats_instrumentation)
      : LayerTreeHostImpl(settings,
                          host_impl_client,
                          proxy,
                          stats_instrumentation,
                          NULL,
                          0),
        test_hooks_(test_hooks),
        block_notify_ready_to_activate_for_testing_(false),
        notify_ready_to_activate_was_blocked_(false) {}

  virtual void BeginImplFrame(const BeginFrameArgs& args) OVERRIDE {
    test_hooks_->WillBeginImplFrameOnThread(this, args);
    LayerTreeHostImpl::BeginImplFrame(args);
    test_hooks_->DidBeginImplFrameOnThread(this, args);
  }

  virtual void BeginMainFrameAborted(bool did_handle) OVERRIDE {
    LayerTreeHostImpl::BeginMainFrameAborted(did_handle);
    test_hooks_->BeginMainFrameAbortedOnThread(this, did_handle);
  }

  virtual void BeginCommit() OVERRIDE {
    LayerTreeHostImpl::BeginCommit();
    test_hooks_->BeginCommitOnThread(this);
  }

  virtual void CommitComplete() OVERRIDE {
    LayerTreeHostImpl::CommitComplete();
    test_hooks_->CommitCompleteOnThread(this);

    if (!settings().impl_side_painting) {
      test_hooks_->WillActivateTreeOnThread(this);
      test_hooks_->DidActivateTreeOnThread(this);
    }
  }

  virtual DrawSwapReadbackResult::DrawResult PrepareToDraw(
      FrameData* frame,
      const gfx::Rect& damage_rect) OVERRIDE {
    DrawSwapReadbackResult::DrawResult draw_result =
        LayerTreeHostImpl::PrepareToDraw(frame, damage_rect);
    return test_hooks_->PrepareToDrawOnThread(this, frame, draw_result);
  }

  virtual void DrawLayers(FrameData* frame,
                          base::TimeTicks frame_begin_time) OVERRIDE {
    LayerTreeHostImpl::DrawLayers(frame, frame_begin_time);
    test_hooks_->DrawLayersOnThread(this);
  }

  virtual bool SwapBuffers(const LayerTreeHostImpl::FrameData& frame) OVERRIDE {
    bool result = LayerTreeHostImpl::SwapBuffers(frame);
    test_hooks_->SwapBuffersOnThread(this, result);
    return result;
  }

  virtual void OnSwapBuffersComplete() OVERRIDE {
    LayerTreeHostImpl::OnSwapBuffersComplete();
    test_hooks_->SwapBuffersCompleteOnThread(this);
  }

  virtual void ReclaimResources(const CompositorFrameAck* ack) OVERRIDE {
    LayerTreeHostImpl::ReclaimResources(ack);
  }

  virtual void UpdateVisibleTiles() OVERRIDE {
    LayerTreeHostImpl::UpdateVisibleTiles();
    test_hooks_->UpdateVisibleTilesOnThread(this);
  }

  virtual void NotifyReadyToActivate() OVERRIDE {
    if (block_notify_ready_to_activate_for_testing_)
      notify_ready_to_activate_was_blocked_ = true;
    else
      client_->NotifyReadyToActivate();
  }

  virtual void BlockNotifyReadyToActivateForTesting(bool block) OVERRIDE {
    block_notify_ready_to_activate_for_testing_ = block;
    if (!block && notify_ready_to_activate_was_blocked_) {
      NotifyReadyToActivate();
      notify_ready_to_activate_was_blocked_ = false;
    }
  }

  virtual void ActivatePendingTree() OVERRIDE {
    test_hooks_->WillActivateTreeOnThread(this);
    LayerTreeHostImpl::ActivatePendingTree();
    DCHECK(!pending_tree());
    test_hooks_->DidActivateTreeOnThread(this);
  }

  virtual bool InitializeRenderer(scoped_ptr<OutputSurface> output_surface)
      OVERRIDE {
    bool success = LayerTreeHostImpl::InitializeRenderer(output_surface.Pass());
    test_hooks_->InitializedRendererOnThread(this, success);
    return success;
  }

  virtual void SetVisible(bool visible) OVERRIDE {
    LayerTreeHostImpl::SetVisible(visible);
    test_hooks_->DidSetVisibleOnImplTree(this, visible);
  }

  virtual void AnimateLayers(base::TimeTicks monotonic_time,
                             base::Time wall_clock_time) OVERRIDE {
    test_hooks_->WillAnimateLayers(this, monotonic_time);
    LayerTreeHostImpl::AnimateLayers(monotonic_time, wall_clock_time);
    test_hooks_->AnimateLayers(this, monotonic_time);
  }

  virtual void UpdateAnimationState(bool start_ready_animations) OVERRIDE {
    LayerTreeHostImpl::UpdateAnimationState(start_ready_animations);
    bool has_unfinished_animation = false;
    AnimationRegistrar::AnimationControllerMap::const_iterator iter =
        active_animation_controllers().begin();
    for (; iter != active_animation_controllers().end(); ++iter) {
      if (iter->second->HasActiveAnimation()) {
        has_unfinished_animation = true;
        break;
      }
    }
    test_hooks_->UpdateAnimationState(this, has_unfinished_animation);
  }

  virtual base::TimeDelta LowFrequencyAnimationInterval() const OVERRIDE {
    return test_hooks_->LowFrequencyAnimationInterval();
  }

 private:
  TestHooks* test_hooks_;
  bool block_notify_ready_to_activate_for_testing_;
  bool notify_ready_to_activate_was_blocked_;
};

// Implementation of LayerTreeHost callback interface.
class LayerTreeHostClientForTesting : public LayerTreeHostClient,
                                      public LayerTreeHostSingleThreadClient {
 public:
  static scoped_ptr<LayerTreeHostClientForTesting> Create(
      TestHooks* test_hooks) {
    return make_scoped_ptr(new LayerTreeHostClientForTesting(test_hooks));
  }
  virtual ~LayerTreeHostClientForTesting() {}

  virtual void WillBeginMainFrame(int frame_id) OVERRIDE {
    test_hooks_->WillBeginMainFrame();
  }

  virtual void DidBeginMainFrame() OVERRIDE {
    test_hooks_->DidBeginMainFrame();
  }

  virtual void Animate(double monotonic_time) OVERRIDE {
    test_hooks_->Animate(base::TimeTicks::FromInternalValue(
        monotonic_time * base::Time::kMicrosecondsPerSecond));
  }

  virtual void Layout() OVERRIDE { test_hooks_->Layout(); }

  virtual void ApplyScrollAndScale(gfx::Vector2d scroll_delta,
                                   float scale) OVERRIDE {
    test_hooks_->ApplyScrollAndScale(scroll_delta, scale);
  }

  virtual scoped_ptr<OutputSurface> CreateOutputSurface(bool fallback)
      OVERRIDE {
    return test_hooks_->CreateOutputSurface(fallback);
  }

  virtual void DidInitializeOutputSurface(bool succeeded) OVERRIDE {
    test_hooks_->DidInitializeOutputSurface(succeeded);
  }

  virtual void DidFailToInitializeOutputSurface() OVERRIDE {
    test_hooks_->DidFailToInitializeOutputSurface();
  }

  virtual void WillCommit() OVERRIDE { test_hooks_->WillCommit(); }

  virtual void DidCommit() OVERRIDE { test_hooks_->DidCommit(); }

  virtual void DidCommitAndDrawFrame() OVERRIDE {
    test_hooks_->DidCommitAndDrawFrame();
  }

  virtual void DidCompleteSwapBuffers() OVERRIDE {
    test_hooks_->DidCompleteSwapBuffers();
  }

  virtual void ScheduleComposite() OVERRIDE {
    test_hooks_->ScheduleComposite();
  }

  virtual void ScheduleAnimation() OVERRIDE {
    test_hooks_->ScheduleAnimation();
  }

  virtual void DidPostSwapBuffers() OVERRIDE {}
  virtual void DidAbortSwapBuffers() OVERRIDE {}

  virtual scoped_refptr<ContextProvider> OffscreenContextProvider() OVERRIDE {
    return test_hooks_->OffscreenContextProvider();
  }

 private:
  explicit LayerTreeHostClientForTesting(TestHooks* test_hooks)
      : test_hooks_(test_hooks) {}

  TestHooks* test_hooks_;
};

// Adapts LayerTreeHost for test. Injects LayerTreeHostImplForTesting.
class LayerTreeHostForTesting : public LayerTreeHost {
 public:
  static scoped_ptr<LayerTreeHostForTesting> Create(
      TestHooks* test_hooks,
      LayerTreeHostClientForTesting* client,
      const LayerTreeSettings& settings,
      scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner) {
    scoped_ptr<LayerTreeHostForTesting> layer_tree_host(
        new LayerTreeHostForTesting(test_hooks, client, settings));
    if (impl_task_runner.get())
      layer_tree_host->InitializeThreaded(impl_task_runner);
    else
      layer_tree_host->InitializeSingleThreaded(client);
    return layer_tree_host.Pass();
  }

  virtual scoped_ptr<LayerTreeHostImpl> CreateLayerTreeHostImpl(
      LayerTreeHostImplClient* host_impl_client) OVERRIDE {
    return LayerTreeHostImplForTesting::Create(
        test_hooks_,
        settings(),
        host_impl_client,
        proxy(),
        rendering_stats_instrumentation()).PassAs<LayerTreeHostImpl>();
  }

  virtual void SetNeedsCommit() OVERRIDE {
    if (!test_started_)
      return;
    LayerTreeHost::SetNeedsCommit();
  }

  void set_test_started(bool started) { test_started_ = started; }

  virtual void DidDeferCommit() OVERRIDE { test_hooks_->DidDeferCommit(); }

 private:
  LayerTreeHostForTesting(TestHooks* test_hooks,
                          LayerTreeHostClient* client,
                          const LayerTreeSettings& settings)
      : LayerTreeHost(client, NULL, settings),
        test_hooks_(test_hooks),
        test_started_(false) {}

  TestHooks* test_hooks_;
  bool test_started_;
};

LayerTreeTest::LayerTreeTest()
    : beginning_(false),
      end_when_begin_returns_(false),
      timed_out_(false),
      scheduled_(false),
      schedule_when_set_visible_true_(false),
      started_(false),
      ended_(false),
      delegating_renderer_(false),
      timeout_seconds_(0),
      weak_factory_(this) {
  main_thread_weak_ptr_ = weak_factory_.GetWeakPtr();

  // Tests should timeout quickly unless --cc-layer-tree-test-no-timeout was
  // specified (for running in a debugger).
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kCCLayerTreeTestNoTimeout))
    timeout_seconds_ = 5;
}

LayerTreeTest::~LayerTreeTest() {}

void LayerTreeTest::EndTest() {
  if (ended_)
    return;
  ended_ = true;

  // For the case where we EndTest during BeginTest(), set a flag to indicate
  // that the test should end the second BeginTest regains control.
  if (beginning_) {
    end_when_begin_returns_ = true;
  } else {
    main_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&LayerTreeTest::RealEndTest, main_thread_weak_ptr_));
  }
}

void LayerTreeTest::EndTestAfterDelay(int delay_milliseconds) {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&LayerTreeTest::EndTest, main_thread_weak_ptr_));
}

void LayerTreeTest::PostAddAnimationToMainThread(
    Layer* layer_to_receive_animation) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchAddAnimation,
                 main_thread_weak_ptr_,
                 base::Unretained(layer_to_receive_animation),
                 0.000001));
}

void LayerTreeTest::PostAddInstantAnimationToMainThread(
    Layer* layer_to_receive_animation) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchAddAnimation,
                 main_thread_weak_ptr_,
                 base::Unretained(layer_to_receive_animation),
                 0.0));
}

void LayerTreeTest::PostAddLongAnimationToMainThread(
    Layer* layer_to_receive_animation) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchAddAnimation,
                 main_thread_weak_ptr_,
                 base::Unretained(layer_to_receive_animation),
                 1.0));
}

void LayerTreeTest::PostSetNeedsCommitToMainThread() {
  main_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&LayerTreeTest::DispatchSetNeedsCommit,
                                         main_thread_weak_ptr_));
}

void LayerTreeTest::PostSetNeedsUpdateLayersToMainThread() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchSetNeedsUpdateLayers,
                 main_thread_weak_ptr_));
}

void LayerTreeTest::PostReadbackToMainThread() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchReadback, main_thread_weak_ptr_));
}

void LayerTreeTest::PostAcquireLayerTextures() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchAcquireLayerTextures,
                 main_thread_weak_ptr_));
}

void LayerTreeTest::PostSetNeedsRedrawToMainThread() {
  main_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&LayerTreeTest::DispatchSetNeedsRedraw,
                                         main_thread_weak_ptr_));
}

void LayerTreeTest::PostSetNeedsRedrawRectToMainThread(
    const gfx::Rect& damage_rect) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchSetNeedsRedrawRect,
                 main_thread_weak_ptr_,
                 damage_rect));
}

void LayerTreeTest::PostSetVisibleToMainThread(bool visible) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(
          &LayerTreeTest::DispatchSetVisible, main_thread_weak_ptr_, visible));
}

void LayerTreeTest::PostSetNextCommitForcesRedrawToMainThread() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchSetNextCommitForcesRedraw,
                 main_thread_weak_ptr_));
}

void LayerTreeTest::WillBeginTest() {
  layer_tree_host_->SetLayerTreeHostClientReady();
}

void LayerTreeTest::DoBeginTest() {
  client_ = LayerTreeHostClientForTesting::Create(this);

  DCHECK(!impl_thread_ || impl_thread_->message_loop_proxy().get());
  layer_tree_host_ = LayerTreeHostForTesting::Create(
      this,
      client_.get(),
      settings_,
      impl_thread_ ? impl_thread_->message_loop_proxy() : NULL);
  ASSERT_TRUE(layer_tree_host_);

  started_ = true;
  beginning_ = true;
  SetupTree();
  WillBeginTest();
  BeginTest();
  beginning_ = false;
  if (end_when_begin_returns_)
    RealEndTest();

  // Allow commits to happen once BeginTest() has had a chance to post tasks
  // so that those tasks will happen before the first commit.
  if (layer_tree_host_) {
    static_cast<LayerTreeHostForTesting*>(layer_tree_host_.get())
        ->set_test_started(true);
  }
}

void LayerTreeTest::SetupTree() {
  if (!layer_tree_host_->root_layer()) {
    scoped_refptr<Layer> root_layer = Layer::Create();
    root_layer->SetAnchorPoint(gfx::PointF());
    root_layer->SetBounds(gfx::Size(1, 1));
    root_layer->SetIsDrawable(true);
    layer_tree_host_->SetRootLayer(root_layer);
  }

  gfx::Size root_bounds = layer_tree_host_->root_layer()->bounds();
  gfx::Size device_root_bounds = gfx::ToCeiledSize(
      gfx::ScaleSize(root_bounds, layer_tree_host_->device_scale_factor()));
  layer_tree_host_->SetViewportSize(device_root_bounds);
}

void LayerTreeTest::Timeout() {
  timed_out_ = true;
  EndTest();
}

void LayerTreeTest::ScheduleComposite() {
  if (!started_ || scheduled_)
    return;
  scheduled_ = true;
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DispatchComposite, main_thread_weak_ptr_));
}

void LayerTreeTest::RealEndTest() {
  if (layer_tree_host_ && proxy()->CommitPendingForTesting()) {
    main_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&LayerTreeTest::RealEndTest, main_thread_weak_ptr_));
    return;
  }

  base::MessageLoop::current()->Quit();
}

void LayerTreeTest::DispatchAddAnimation(Layer* layer_to_receive_animation,
                                         double animation_duration) {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_to_receive_animation) {
    AddOpacityTransitionToLayer(
        layer_to_receive_animation, animation_duration, 0, 0.5, true);
  }
}

void LayerTreeTest::DispatchSetNeedsCommit() {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_tree_host_)
    layer_tree_host_->SetNeedsCommit();
}

void LayerTreeTest::DispatchSetNeedsUpdateLayers() {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_tree_host_)
    layer_tree_host_->SetNeedsUpdateLayers();
}

void LayerTreeTest::DispatchReadback() {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_tree_host_) {
    char pixels[4];
    layer_tree_host()->CompositeAndReadback(&pixels, gfx::Rect(0, 0, 1, 1));
  }
}

void LayerTreeTest::DispatchAcquireLayerTextures() {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_tree_host_)
    layer_tree_host_->AcquireLayerTextures();
}

void LayerTreeTest::DispatchSetNeedsRedraw() {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_tree_host_)
    layer_tree_host_->SetNeedsRedraw();
}

void LayerTreeTest::DispatchSetNeedsRedrawRect(const gfx::Rect& damage_rect) {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_tree_host_)
    layer_tree_host_->SetNeedsRedrawRect(damage_rect);
}

void LayerTreeTest::DispatchSetVisible(bool visible) {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (!layer_tree_host_)
    return;

  layer_tree_host_->SetVisible(visible);

  // If the LTH is being made visible and a previous ScheduleComposite() was
  // deferred because the LTH was not visible, re-schedule the composite now.
  if (layer_tree_host_->visible() && schedule_when_set_visible_true_)
    ScheduleComposite();
}

void LayerTreeTest::DispatchSetNextCommitForcesRedraw() {
  DCHECK(!proxy() || proxy()->IsMainThread());

  if (layer_tree_host_)
    layer_tree_host_->SetNextCommitForcesRedraw();
}

void LayerTreeTest::DispatchComposite() {
  scheduled_ = false;

  if (!layer_tree_host_)
    return;

  // If the LTH is not visible, defer the composite until the LTH is made
  // visible.
  if (!layer_tree_host_->visible()) {
    schedule_when_set_visible_true_ = true;
    return;
  }

  schedule_when_set_visible_true_ = false;
  base::TimeTicks now = gfx::FrameTime::Now();
  layer_tree_host_->Composite(now);
}

void LayerTreeTest::RunTest(bool threaded,
                            bool delegating_renderer,
                            bool impl_side_painting) {
  if (threaded) {
    impl_thread_.reset(new base::Thread("Compositor"));
    ASSERT_TRUE(impl_thread_->Start());
  }

  main_task_runner_ = base::MessageLoopProxy::current();

  delegating_renderer_ = delegating_renderer;

  // Spend less time waiting for BeginImplFrame because the output is
  // mocked out.
  settings_.refresh_rate = 200.0;
  if (impl_side_painting) {
    DCHECK(threaded)
        << "Don't run single thread + impl side painting, it doesn't exist.";
    settings_.impl_side_painting = true;
  }
  InitializeSettings(&settings_);

  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LayerTreeTest::DoBeginTest, base::Unretained(this)));

  if (timeout_seconds_) {
    timeout_.Reset(base::Bind(&LayerTreeTest::Timeout, base::Unretained(this)));
    main_task_runner_->PostDelayedTask(
        FROM_HERE,
        timeout_.callback(),
        base::TimeDelta::FromSeconds(timeout_seconds_));
  }

  base::MessageLoop::current()->Run();
  if (layer_tree_host_ && layer_tree_host_->root_layer())
    layer_tree_host_->root_layer()->SetLayerTreeHost(NULL);
  layer_tree_host_.reset();

  timeout_.Cancel();

  ASSERT_FALSE(layer_tree_host_.get());
  client_.reset();
  if (timed_out_) {
    FAIL() << "Test timed out";
    return;
  }
  AfterTest();
}

void LayerTreeTest::RunTestWithImplSidePainting() {
  RunTest(true, false, true);
}

scoped_ptr<OutputSurface> LayerTreeTest::CreateOutputSurface(bool fallback) {
  scoped_ptr<FakeOutputSurface> output_surface =
      CreateFakeOutputSurfaceForTest(fallback);

  output_surface_ = output_surface.get();
  return output_surface.PassAs<OutputSurface>();
}

scoped_ptr<FakeOutputSurface> LayerTreeTest::CreateFakeOutputSurfaceForTest(
    bool fallback) {
  if (delegating_renderer_)
    return FakeOutputSurface::CreateDelegating3d();
  else
    return FakeOutputSurface::Create3d();
}

scoped_refptr<ContextProvider> LayerTreeTest::OffscreenContextProvider() {
  if (!compositor_contexts_.get() ||
      compositor_contexts_->DestroyedOnMainThread())
    compositor_contexts_ = TestContextProvider::Create();
  return compositor_contexts_;
}

TestWebGraphicsContext3D* LayerTreeTest::TestContext() {
  return static_cast<TestContextProvider*>(
      output_surface_->context_provider().get())->TestContext3d();
}

int LayerTreeTest::LastCommittedSourceFrameNumber(LayerTreeHostImpl* impl)
    const {
  if (impl->pending_tree())
    return impl->pending_tree()->source_frame_number();
  if (impl->active_tree())
    return impl->active_tree()->source_frame_number();
  // Source frames start at 0, so this is invalid.
  return -1;
}

}  // namespace cc
