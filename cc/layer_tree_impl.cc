// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layer_tree_impl.h"

#include "base/debug/trace_event.h"
#include "cc/heads_up_display_layer_impl.h"
#include "cc/layer_tree_host_common.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/scrollbar_layer_impl.h"
#include "ui/gfx/size_conversions.h"
#include "ui/gfx/vector2d_conversions.h"

namespace cc {

LayerTreeImpl::LayerTreeImpl(LayerTreeHostImpl* layer_tree_host_impl)
    : layer_tree_host_impl_(layer_tree_host_impl),
      source_frame_number_(-1),
      hud_layer_(0),
      root_scroll_layer_(0),
      currently_scrolling_layer_(0),
      background_color_(0),
      has_transparent_background_(false),
      page_scale_factor_(1),
      page_scale_delta_(1),
      sent_page_scale_delta_(1),
      min_page_scale_factor_(0),
      max_page_scale_factor_(0),
      scrolling_layer_id_from_previous_tree_(0),
      contents_textures_purged_(false),
      viewport_size_invalid_(false),
      needs_update_draw_properties_(true),
      needs_full_tree_sync_(true) {
}

LayerTreeImpl::~LayerTreeImpl() {
  // Need to explicitly clear the tree prior to destroying this so that
  // the LayerTreeImpl pointer is still valid in the LayerImpl dtor.
  root_layer_.reset();
}

static LayerImpl* findRootScrollLayer(LayerImpl* layer)
{
    if (!layer)
        return 0;

    if (layer->scrollable())
        return layer;

    for (size_t i = 0; i < layer->children().size(); ++i) {
        LayerImpl* found = findRootScrollLayer(layer->children()[i]);
        if (found)
            return found;
    }

    return 0;
}

void LayerTreeImpl::SetRootLayer(scoped_ptr<LayerImpl> layer) {
  root_layer_ = layer.Pass();
  root_scroll_layer_ = NULL;
  currently_scrolling_layer_ = NULL;

  layer_tree_host_impl_->OnCanDrawStateChangedForTree();
}

void LayerTreeImpl::FindRootScrollLayer() {
  root_scroll_layer_ = findRootScrollLayer(root_layer_.get());

  if (root_layer_ && scrolling_layer_id_from_previous_tree_) {
    currently_scrolling_layer_ = LayerTreeHostCommon::findLayerInSubtree(
        root_layer_.get(),
        scrolling_layer_id_from_previous_tree_);
  }

  scrolling_layer_id_from_previous_tree_ = 0;
}

scoped_ptr<LayerImpl> LayerTreeImpl::DetachLayerTree() {
  // Clear all data structures that have direct references to the layer tree.
  scrolling_layer_id_from_previous_tree_ =
    currently_scrolling_layer_ ? currently_scrolling_layer_->id() : 0;
  root_scroll_layer_ = NULL;
  currently_scrolling_layer_ = NULL;

  render_surface_layer_list_.clear();
  set_needs_update_draw_properties();
  return root_layer_.Pass();
}

void LayerTreeImpl::PushPropertiesTo(LayerTreeImpl* target_tree) {
  target_tree->SetPageScaleFactorAndLimits(
      page_scale_factor(), min_page_scale_factor(), max_page_scale_factor());
  target_tree->SetPageScaleDelta(
      target_tree->page_scale_delta() / target_tree->sent_page_scale_delta());
  target_tree->set_sent_page_scale_delta(1);

  // This should match the property synchronization in
  // LayerTreeHost::finishCommitOnImplThread().
  target_tree->set_source_frame_number(source_frame_number());
  target_tree->set_background_color(background_color());
  target_tree->set_has_transparent_background(has_transparent_background());

  if (ContentsTexturesPurged())
    target_tree->SetContentsTexturesPurged();
  else
    target_tree->ResetContentsTexturesPurged();

  if (ViewportSizeInvalid())
    target_tree->SetViewportSizeInvalid();
  else
    target_tree->ResetViewportSizeInvalid();

  if (hud_layer())
    target_tree->set_hud_layer(static_cast<HeadsUpDisplayLayerImpl*>(
        LayerTreeHostCommon::findLayerInSubtree(
            target_tree->root_layer(), hud_layer()->id())));
  else
    target_tree->set_hud_layer(NULL);
}

LayerImpl* LayerTreeImpl::RootScrollLayer() const {
  DCHECK(IsActiveTree());
  return root_scroll_layer_;
}

LayerImpl* LayerTreeImpl::RootClipLayer() const {
  return root_scroll_layer_ ? root_scroll_layer_->parent() : NULL;
}

LayerImpl* LayerTreeImpl::CurrentlyScrollingLayer() const {
  DCHECK(IsActiveTree());
  return currently_scrolling_layer_;
}

void LayerTreeImpl::ClearCurrentlyScrollingLayer() {
  currently_scrolling_layer_ = NULL;
  scrolling_layer_id_from_previous_tree_ = 0;
}

void LayerTreeImpl::SetPageScaleFactorAndLimits(float page_scale_factor,
    float min_page_scale_factor, float max_page_scale_factor)
{
  if (!page_scale_factor)
    return;

  min_page_scale_factor_ = min_page_scale_factor;
  max_page_scale_factor_ = max_page_scale_factor;
  page_scale_factor_ = page_scale_factor;
}

void LayerTreeImpl::SetPageScaleDelta(float delta)
{
  // Clamp to the current min/max limits.
  float total = page_scale_factor_ * delta;
  if (min_page_scale_factor_ && total < min_page_scale_factor_)
    delta = min_page_scale_factor_ / page_scale_factor_;
  else if (max_page_scale_factor_ && total > max_page_scale_factor_)
    delta = max_page_scale_factor_ / page_scale_factor_;

  if (delta == page_scale_delta_)
    return;

  page_scale_delta_ = delta;

  if (IsActiveTree()) {
    LayerTreeImpl* pending_tree = layer_tree_host_impl_->pending_tree();
    if (pending_tree) {
      DCHECK_EQ(1, pending_tree->sent_page_scale_delta());
      pending_tree->SetPageScaleDelta(page_scale_delta_ / sent_page_scale_delta_);
    }
  }

  UpdateMaxScrollOffset();
  set_needs_update_draw_properties();
}

gfx::SizeF LayerTreeImpl::ScrollableViewportSize() const {
  return gfx::ScaleSize(layer_tree_host_impl_->VisibleViewportSize(),
                        1.0f / total_page_scale_factor());
}

void LayerTreeImpl::UpdateMaxScrollOffset() {
  if (!root_scroll_layer_ || !root_scroll_layer_->children().size())
    return;

  gfx::Vector2dF max_scroll = gfx::Rect(ScrollableSize()).bottom_right() -
      gfx::RectF(ScrollableViewportSize()).bottom_right();

  // The viewport may be larger than the contents in some cases, such as
  // having a vertical scrollbar but no horizontal overflow.
  max_scroll.ClampToMin(gfx::Vector2dF());

  root_scroll_layer_->SetMaxScrollOffset(gfx::ToFlooredVector2d(max_scroll));
}

gfx::Transform LayerTreeImpl::ImplTransform() const {
  gfx::Transform transform;
  transform.Scale(total_page_scale_factor(), total_page_scale_factor());
  return transform;
}

void LayerTreeImpl::UpdateSolidColorScrollbars() {
  DCHECK(settings().solidColorScrollbars);

  LayerImpl* root_scroll = RootScrollLayer();
  if (!root_scroll)
    return;

  if (!IsActiveTree())
    return;

  gfx::RectF scrollable_viewport(
      gfx::PointAtOffsetFromOrigin(root_scroll->TotalScrollOffset()),
      ScrollableViewportSize());
  float vertical_adjust = 0.0f;
  if (RootClipLayer())
    vertical_adjust = layer_tree_host_impl_->VisibleViewportSize().height() -
                      RootClipLayer()->bounds().height();
  if (ScrollbarLayerImpl* horiz = root_scroll->horizontal_scrollbar_layer()) {
    horiz->set_vertical_adjust(vertical_adjust);
    horiz->SetViewportWithinScrollableArea(scrollable_viewport,
                                           ScrollableSize());
  }
  if (ScrollbarLayerImpl* vertical = root_scroll->vertical_scrollbar_layer()) {
    vertical->set_vertical_adjust(vertical_adjust);
    vertical->SetViewportWithinScrollableArea(scrollable_viewport,
                                              ScrollableSize());
  }
}

struct UpdateTilePrioritiesForLayer {
  void operator()(LayerImpl *layer) {
    layer->UpdateTilePriorities();
  }
};

void LayerTreeImpl::UpdateDrawProperties(UpdateDrawPropertiesReason reason) {
  if (settings().solidColorScrollbars && IsActiveTree()) {
    UpdateSolidColorScrollbars();

    // The top controls manager is incompatible with the WebKit-created cliprect
    // because it can bring into view a larger amount of content when it
    // hides. It's safe to deactivate the clip rect if no non-overlay scrollbars
    // are present.
    if (layer_tree_host_impl_->top_controls_manager())
      RootScrollLayer()->parent()->SetMasksToBounds(false);
  }

  if (!needs_update_draw_properties_) {
    if (reason == UPDATE_ACTIVE_TREE_FOR_DRAW && root_layer())
      LayerTreeHostCommon::callFunctionForSubtree<UpdateTilePrioritiesForLayer>(
          root_layer());
    return;
  }

  needs_update_draw_properties_ = false;
  render_surface_layer_list_.clear();

  // For maxTextureSize.
  if (!layer_tree_host_impl_->renderer())
      return;

  if (!root_layer())
    return;

  if (root_scroll_layer_) {
    root_scroll_layer_->SetImplTransform(ImplTransform());
    // Setting the impl transform re-sets this.
    needs_update_draw_properties_ = false;
  }

  {
    TRACE_EVENT1("cc",
                 "LayerTreeImpl::UpdateDrawProperties",
                 "IsActive",
                 IsActiveTree());
    bool update_tile_priorities =
        reason == UPDATE_PENDING_TREE ||
        reason == UPDATE_ACTIVE_TREE_FOR_DRAW;
    LayerTreeHostCommon::calculateDrawProperties(
        root_layer(),
        device_viewport_size(),
        device_scale_factor(),
        total_page_scale_factor(),
        MaxTextureSize(),
        settings().canUseLCDText,
        render_surface_layer_list_,
        update_tile_priorities);
  }

  DCHECK(!needs_update_draw_properties_) <<
      "calcDrawProperties should not set_needs_update_draw_properties()";
}

static void ClearRenderSurfacesOnLayerImplRecursive(LayerImpl* current)
{
    DCHECK(current);
    for (size_t i = 0; i < current->children().size(); ++i)
        ClearRenderSurfacesOnLayerImplRecursive(current->children()[i]);
    current->ClearRenderSurface();
}

void LayerTreeImpl::ClearRenderSurfaces() {
  ClearRenderSurfacesOnLayerImplRecursive(root_layer());
  render_surface_layer_list_.clear();
  set_needs_update_draw_properties();
}

bool LayerTreeImpl::AreVisibleResourcesReady() const {
  TRACE_EVENT0("cc", "LayerTreeImpl::AreVisibleResourcesReady");

  typedef LayerIterator<LayerImpl,
                        std::vector<LayerImpl*>,
                        RenderSurfaceImpl,
                        LayerIteratorActions::BackToFront> LayerIteratorType;
  LayerIteratorType end = LayerIteratorType::end(&render_surface_layer_list_);
  for (LayerIteratorType it = LayerIteratorType::begin(
           &render_surface_layer_list_); it != end; ++it) {
    if (it.representsItself() && !(*it)->AreVisibleResourcesReady())
      return false;
  }

  return true;
}

const LayerTreeImpl::LayerList& LayerTreeImpl::RenderSurfaceLayerList() const {
  // If this assert triggers, then the list is dirty.
  DCHECK(!needs_update_draw_properties_);
  return render_surface_layer_list_;
}

gfx::Size LayerTreeImpl::ScrollableSize() const {
  if (!root_scroll_layer_ || root_scroll_layer_->children().empty())
    return gfx::Size();
  return root_scroll_layer_->children()[0]->bounds();
}

LayerImpl* LayerTreeImpl::LayerById(int id) {
  LayerIdMap::iterator iter = layer_id_map_.find(id);
  return iter != layer_id_map_.end() ? iter->second : NULL;
}

void LayerTreeImpl::RegisterLayer(LayerImpl* layer) {
  DCHECK(!LayerById(layer->id()));
  layer_id_map_[layer->id()] = layer;
}

void LayerTreeImpl::UnregisterLayer(LayerImpl* layer) {
  DCHECK(LayerById(layer->id()));
  layer_id_map_.erase(layer->id());
}

void LayerTreeImpl::PushPersistedState(LayerTreeImpl* pendingTree) {
  int id = currently_scrolling_layer_ ? currently_scrolling_layer_->id() : 0;
  pendingTree->set_currently_scrolling_layer(
      LayerTreeHostCommon::findLayerInSubtree(pendingTree->root_layer(), id));
}

static void DidBecomeActiveRecursive(LayerImpl* layer) {
  layer->DidBecomeActive();
  for (size_t i = 0; i < layer->children().size(); ++i)
    DidBecomeActiveRecursive(layer->children()[i]);
}

void LayerTreeImpl::DidBecomeActive() {
  if (root_layer())
    DidBecomeActiveRecursive(root_layer());
  FindRootScrollLayer();
  UpdateMaxScrollOffset();
}

bool LayerTreeImpl::ContentsTexturesPurged() const {
  return contents_textures_purged_;
}

void LayerTreeImpl::SetContentsTexturesPurged() {
  contents_textures_purged_ = true;
  layer_tree_host_impl_->OnCanDrawStateChangedForTree();
}

void LayerTreeImpl::ResetContentsTexturesPurged() {
  contents_textures_purged_ = false;
  layer_tree_host_impl_->OnCanDrawStateChangedForTree();
}

bool LayerTreeImpl::ViewportSizeInvalid() const {
  return viewport_size_invalid_;
}

void LayerTreeImpl::SetViewportSizeInvalid() {
  viewport_size_invalid_ = true;
  layer_tree_host_impl_->OnCanDrawStateChangedForTree();
}

void LayerTreeImpl::ResetViewportSizeInvalid() {
  viewport_size_invalid_ = false;
  layer_tree_host_impl_->OnCanDrawStateChangedForTree();
}

Proxy* LayerTreeImpl::proxy() const {
  return layer_tree_host_impl_->proxy();
}

const LayerTreeSettings& LayerTreeImpl::settings() const {
  return layer_tree_host_impl_->settings();
}

const RendererCapabilities& LayerTreeImpl::rendererCapabilities() const {
  return layer_tree_host_impl_->GetRendererCapabilities();
}

OutputSurface* LayerTreeImpl::output_surface() const {
  return layer_tree_host_impl_->output_surface();
}

ResourceProvider* LayerTreeImpl::resource_provider() const {
  return layer_tree_host_impl_->resource_provider();
}

TileManager* LayerTreeImpl::tile_manager() const {
  return layer_tree_host_impl_->tile_manager();
}

FrameRateCounter* LayerTreeImpl::frame_rate_counter() const {
  return layer_tree_host_impl_->fps_counter();
}

PaintTimeCounter* LayerTreeImpl::paint_time_counter() const {
  return layer_tree_host_impl_->paint_time_counter();
}

MemoryHistory* LayerTreeImpl::memory_history() const {
  return layer_tree_host_impl_->memory_history();
}

bool LayerTreeImpl::IsActiveTree() const {
  return layer_tree_host_impl_->active_tree() == this;
}

bool LayerTreeImpl::IsPendingTree() const {
  return layer_tree_host_impl_->pending_tree() == this;
}

bool LayerTreeImpl::IsRecycleTree() const {
  return layer_tree_host_impl_->recycle_tree() == this;
}

LayerImpl* LayerTreeImpl::FindActiveTreeLayerById(int id) {
  LayerTreeImpl* tree = layer_tree_host_impl_->active_tree();
  if (!tree)
    return NULL;
  return tree->LayerById(id);
}

LayerImpl* LayerTreeImpl::FindPendingTreeLayerById(int id) {
  LayerTreeImpl* tree = layer_tree_host_impl_->pending_tree();
  if (!tree)
    return NULL;
  return tree->LayerById(id);
}

int LayerTreeImpl::MaxTextureSize() const {
  return layer_tree_host_impl_->GetRendererCapabilities().max_texture_size;
}

bool LayerTreeImpl::PinchGestureActive() const {
  return layer_tree_host_impl_->pinch_gesture_active();
}

base::TimeTicks LayerTreeImpl::CurrentFrameTime() const {
  return layer_tree_host_impl_->CurrentFrameTime();
}

void LayerTreeImpl::SetNeedsRedraw() {
  layer_tree_host_impl_->setNeedsRedraw();
}

const LayerTreeDebugState& LayerTreeImpl::debug_state() const {
  return layer_tree_host_impl_->debug_state();
}

float LayerTreeImpl::device_scale_factor() const {
  return layer_tree_host_impl_->device_scale_factor();
}

gfx::Size LayerTreeImpl::device_viewport_size() const {
  return layer_tree_host_impl_->device_viewport_size();
}

gfx::Size LayerTreeImpl::layout_viewport_size() const {
  return layer_tree_host_impl_->layout_viewport_size();
}

std::string LayerTreeImpl::layer_tree_as_text() const {
  return layer_tree_host_impl_->LayerTreeAsText();
}

DebugRectHistory* LayerTreeImpl::debug_rect_history() const {
  return layer_tree_host_impl_->debug_rect_history();
}

AnimationRegistrar* LayerTreeImpl::animationRegistrar() const {
  return layer_tree_host_impl_->animation_registrar();
}

scoped_ptr<base::Value> LayerTreeImpl::AsValue() const {
  scoped_ptr<base::ListValue> state(new base::ListValue());
  typedef LayerIterator<LayerImpl,
                        std::vector<LayerImpl*>,
                        RenderSurfaceImpl,
                        LayerIteratorActions::BackToFront> LayerIteratorType;
  LayerIteratorType end = LayerIteratorType::end(&render_surface_layer_list_);
  for (LayerIteratorType it = LayerIteratorType::begin(
           &render_surface_layer_list_); it != end; ++it) {
    if (!it.representsItself())
      continue;
    state->Append((*it)->AsValue().release());
  }
  return state.PassAs<base::Value>();
}

} // namespace cc
