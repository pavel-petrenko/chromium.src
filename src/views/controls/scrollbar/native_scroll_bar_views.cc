// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/controls/scrollbar/native_scroll_bar_views.h"

#include "base/logging.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/canvas_skia.h"
#include "ui/gfx/path.h"
#include "views/controls/button/custom_button.h"
#include "views/controls/focusable_border.h"
#include "views/controls/scrollbar/native_scroll_bar.h"
#include "views/controls/scrollbar/scroll_bar.h"
#include "views/controls/scrollbar/base_scroll_bar_button.h"
#include "views/controls/scrollbar/base_scroll_bar_thumb.h"

namespace views {

namespace {

// Wrapper for the scroll buttons.
class ScrollBarButton : public BaseScrollBarButton {
 public:
  enum Type {
    UP,
    DOWN,
    LEFT,
    RIGHT,
  };

  ScrollBarButton(ButtonListener* listener, Type type);
  virtual ~ScrollBarButton();

  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual std::string GetClassName() const OVERRIDE {
    return "views/ScrollBarButton";
  }

 protected:
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;

 private:
  gfx::NativeTheme::ExtraParams GetNativeThemeParams() const;
  gfx::NativeTheme::Part GetNativeThemePart() const;
  gfx::NativeTheme::State GetNativeThemeState() const;

  Type type_;
};

// Wrapper for the scroll thumb
class ScrollBarThumb : public BaseScrollBarThumb {
 public:
  explicit ScrollBarThumb(BaseScrollBar* scroll_bar);
  virtual ~ScrollBarThumb();

  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual std::string GetClassName() const OVERRIDE {
    return "views/ScrollBarThumb";
  }

 protected:
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;

 private:
  gfx::NativeTheme::ExtraParams GetNativeThemeParams() const;
  gfx::NativeTheme::Part GetNativeThemePart() const;
  gfx::NativeTheme::State GetNativeThemeState() const;

  ScrollBar* scroll_bar_;
};

/////////////////////////////////////////////////////////////////////////////
// ScrollBarButton

ScrollBarButton::ScrollBarButton(
    ButtonListener* listener,
    Type type)
    : BaseScrollBarButton(listener),
      type_(type) {
  set_focusable(false);
  set_accessibility_focusable(false);
}

ScrollBarButton::~ScrollBarButton() {
}

gfx::Size ScrollBarButton::GetPreferredSize() {
  const gfx::NativeTheme* native_theme = gfx::NativeTheme::instance();
  return native_theme->GetPartSize(GetNativeThemePart(),
                                   GetNativeThemeState(),
                                   GetNativeThemeParams());
}

void ScrollBarButton::OnPaint(gfx::Canvas* canvas) {
  const gfx::NativeTheme* native_theme = gfx::NativeTheme::instance();
  gfx::Rect bounds;
  bounds.set_size(GetPreferredSize());

  native_theme->Paint(canvas->GetSkCanvas(),
                      GetNativeThemePart(),
                      GetNativeThemeState(),
                      bounds,
                      GetNativeThemeParams());
}

gfx::NativeTheme::ExtraParams
    ScrollBarButton::GetNativeThemeParams() const {
  gfx::NativeTheme::ExtraParams params;

  switch (state_) {
    case CustomButton::BS_HOT:
      params.scrollbar_arrow.is_hovering = true;
      break;
    default:
      params.scrollbar_arrow.is_hovering = false;
      break;
  }

  return params;
}

gfx::NativeTheme::Part
    ScrollBarButton::GetNativeThemePart() const {
  switch (type_) {
    case UP:
      return gfx::NativeTheme::kScrollbarUpArrow;
    case DOWN:
      return gfx::NativeTheme::kScrollbarDownArrow;
    case LEFT:
      return gfx::NativeTheme::kScrollbarLeftArrow;
    case RIGHT:
      return gfx::NativeTheme::kScrollbarRightArrow;
    default:
      return gfx::NativeTheme::kScrollbarUpArrow;
  }
}

gfx::NativeTheme::State
    ScrollBarButton::GetNativeThemeState() const {
  gfx::NativeTheme::State state;

  switch (state_) {
    case CustomButton::BS_HOT:
      state = gfx::NativeTheme::kHovered;
      break;
    case CustomButton::BS_PUSHED:
      state = gfx::NativeTheme::kPressed;
      break;
    case CustomButton::BS_DISABLED:
      state = gfx::NativeTheme::kDisabled;
      break;
    case CustomButton::BS_NORMAL:
    default:
      state = gfx::NativeTheme::kNormal;
      break;
  }

  return state;
}

/////////////////////////////////////////////////////////////////////////////
// ScrollBarThumb

ScrollBarThumb::ScrollBarThumb(BaseScrollBar* scroll_bar)
    : BaseScrollBarThumb(scroll_bar),
      scroll_bar_(scroll_bar) {
  set_focusable(false);
  set_accessibility_focusable(false);
}

ScrollBarThumb::~ScrollBarThumb() {
}

gfx::Size ScrollBarThumb::GetPreferredSize() {
  const gfx::NativeTheme* native_theme = gfx::NativeTheme::instance();
  return native_theme->GetPartSize(GetNativeThemePart(),
                                   GetNativeThemeState(),
                                   GetNativeThemeParams());
}

void ScrollBarThumb::OnPaint(gfx::Canvas* canvas) {
  const gfx::NativeTheme* native_theme = gfx::NativeTheme::instance();

  native_theme->Paint(canvas->GetSkCanvas(),
                      GetNativeThemePart(),
                      GetNativeThemeState(),
                      GetLocalBounds(),
                      GetNativeThemeParams());
}

gfx::NativeTheme::ExtraParams
    ScrollBarThumb::GetNativeThemeParams() const {
  gfx::NativeTheme::ExtraParams params;

  switch (GetState()) {
    case CustomButton::BS_HOT:
      params.scrollbar_thumb.is_hovering = true;
      break;
    default:
      params.scrollbar_thumb.is_hovering = false;
      break;
  }

  return params;
}

gfx::NativeTheme::Part ScrollBarThumb::GetNativeThemePart() const {
  if (scroll_bar_->IsHorizontal())
    return gfx::NativeTheme::kScrollbarHorizontalThumb;
  return gfx::NativeTheme::kScrollbarVerticalThumb;
}

gfx::NativeTheme::State ScrollBarThumb::GetNativeThemeState() const {
  gfx::NativeTheme::State state;

  switch (GetState()) {
    case CustomButton::BS_HOT:
      state = gfx::NativeTheme::kHovered;
      break;
    case CustomButton::BS_PUSHED:
      state = gfx::NativeTheme::kPressed;
      break;
    case CustomButton::BS_DISABLED:
      state = gfx::NativeTheme::kDisabled;
      break;
    case CustomButton::BS_NORMAL:
    default:
      state = gfx::NativeTheme::kNormal;
      break;
  }

  return state;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// NativeScrollBarViews, public:

const char NativeScrollBarViews::kViewClassName[] =
    "views/NativeScrollBarViews";

NativeScrollBarViews::NativeScrollBarViews(NativeScrollBar* scroll_bar)
    : BaseScrollBar(scroll_bar->IsHorizontal(),
                    new ScrollBarThumb(this)),
      native_scroll_bar_(scroll_bar) {
  set_controller(native_scroll_bar_->controller());

  if (native_scroll_bar_->IsHorizontal()) {
    prev_button_ = new ScrollBarButton(this, ScrollBarButton::LEFT);
    next_button_ = new ScrollBarButton(this, ScrollBarButton::RIGHT);

    part_ = gfx::NativeTheme::kScrollbarHorizontalTrack;
  } else {
    prev_button_ = new ScrollBarButton(this, ScrollBarButton::UP);
    next_button_ = new ScrollBarButton(this, ScrollBarButton::DOWN);

    part_ = gfx::NativeTheme::kScrollbarVerticalTrack;
  }

  state_ = gfx::NativeTheme::kNormal;

  AddChildView(prev_button_);
  AddChildView(next_button_);

  prev_button_->set_context_menu_controller(this);
  next_button_->set_context_menu_controller(this);
}

NativeScrollBarViews::~NativeScrollBarViews() {
}

////////////////////////////////////////////////////////////////////////////////
// NativeScrollBarViews, View overrides:

void NativeScrollBarViews::Layout() {
  SetBoundsRect(native_scroll_bar_->GetLocalBounds());

  gfx::Size size = prev_button_->GetPreferredSize();
  prev_button_->SetBounds(0, 0, size.width(), size.height());

  if (native_scroll_bar_->IsHorizontal()) {
    next_button_->SetBounds(width() - size.width(), 0,
                            size.width(), size.height());
  } else {
    next_button_->SetBounds(0, height() - size.height(),
                            size.width(), size.height());
  }

  GetThumb()->SetBoundsRect(GetTrackBounds());
}

void NativeScrollBarViews::OnPaint(gfx::Canvas* canvas) {
  const gfx::NativeTheme* native_theme = gfx::NativeTheme::instance();
  gfx::Rect bounds = GetTrackBounds();

  if (bounds.IsEmpty())
    return;

  params_.scrollbar_track.track_x = bounds.x();
  params_.scrollbar_track.track_y = bounds.y();
  params_.scrollbar_track.track_width = bounds.width();
  params_.scrollbar_track.track_height = bounds.height();


  native_theme->Paint(canvas->GetSkCanvas(),
                      part_,
                      state_,
                      bounds,
                      params_);
}

gfx::Size NativeScrollBarViews::GetPreferredSize() {
  if (native_scroll_bar_->IsHorizontal())
    return gfx::Size(0, GetHorizontalScrollBarHeight());
  return gfx::Size(GetVerticalScrollBarWidth(), 0);
}

std::string NativeScrollBarViews::GetClassName() const {
  return kViewClassName;
}

int NativeScrollBarViews::GetLayoutSize() const {
  gfx::Size size = prev_button_->GetPreferredSize();
  return IsHorizontal() ? size.height() : size.width();
}

void NativeScrollBarViews::ScrollToPosition(int position) {
  controller()->ScrollToPosition(native_scroll_bar_, position);
}

int NativeScrollBarViews::GetScrollIncrement(bool is_page, bool is_positive) {
  return controller()->GetScrollIncrement(native_scroll_bar_,
                                          is_page,
                                          is_positive);
}

//////////////////////////////////////////////////////////////////////////////
// BaseButton::ButtonListener overrides:

void NativeScrollBarViews::ButtonPressed(Button* sender,
                                         const views::Event& event) {
  if (sender == prev_button_) {
    ScrollByAmount(SCROLL_PREV_LINE);
  } else if (sender == next_button_) {
    ScrollByAmount(SCROLL_NEXT_LINE);
  }
}

////////////////////////////////////////////////////////////////////////////////
// NativeScrollBarViews, NativeScrollBarWrapper overrides:

int NativeScrollBarViews::GetPosition() const {
  return BaseScrollBar::GetPosition();
}

View* NativeScrollBarViews::GetView() {
  return this;
}

void NativeScrollBarViews::Update(int viewport_size,
                                  int content_size,
                                  int current_pos) {
  BaseScrollBar::Update(viewport_size, content_size, current_pos);
}

////////////////////////////////////////////////////////////////////////////////
// NativeScrollBarViews, private:

gfx::Rect NativeScrollBarViews::GetTrackBounds() const {
  gfx::Rect bounds = GetLocalBounds();
  gfx::Size size = prev_button_->GetPreferredSize();
  BaseScrollBarThumb* thumb = GetThumb();

  if (native_scroll_bar_->IsHorizontal()) {
    bounds.set_x(bounds.x() + size.width());
    bounds.set_width(std::max(0, bounds.width() - 2 * size.width()));
    bounds.set_height(thumb->GetPreferredSize().height());
  } else {
    bounds.set_y(bounds.y() + size.height());
    bounds.set_height(std::max(0, bounds.height() - 2 * size.height()));
    bounds.set_width(thumb->GetPreferredSize().width());
  }

  return bounds;
}

#if defined(USE_WAYLAND) || defined(USE_AURA)
////////////////////////////////////////////////////////////////////////////////
// NativewScrollBarWrapper, public:

// static
NativeScrollBarWrapper* NativeScrollBarWrapper::CreateWrapper(
    NativeScrollBar* scroll_bar) {
  return new NativeScrollBarViews(scroll_bar);
}

// static
int NativeScrollBarWrapper::GetHorizontalScrollBarHeight() {
  return 20;
}

// static
int NativeScrollBarWrapper::GetVerticalScrollBarWidth() {
  return 20;
}
#endif

}  // namespace views
