// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_SHELL_DEFAULT_CONTAINER_EVENT_FILTER_H_
#define UI_AURA_SHELL_DEFAULT_CONTAINER_EVENT_FILTER_H_
#pragma once

#include "ui/aura/toplevel_window_event_filter.h"

namespace aura_shell {
namespace internal {

class DefaultContainerEventFilter : public aura::ToplevelWindowEventFilter {
 public:
  explicit DefaultContainerEventFilter(aura::Window* owner);
  virtual ~DefaultContainerEventFilter();

  // Overridden from aura::ToplevelWindowEventFilter:
  virtual bool PreHandleMouseEvent(aura::Window* target,
                                   aura::MouseEvent* event) OVERRIDE;

 private:
  enum DragState {
    DRAG_NONE,
    DRAG_MOVE,
    DRAG_RESIZE
  };

  // If the mouse is currently over a portion of the window that should
  // trigger a drag or resize, drag_state_ is set appropriately and true
  // is returned. If the mouse is not over a portion of the window that should
  // trigger a more or resize, drag_state_ is not updated and false is returend.
  bool UpdateDragState();

  DragState drag_state_;

  DISALLOW_COPY_AND_ASSIGN(DefaultContainerEventFilter);
};

}  // namespace internal
}  // namespace aura_shell

#endif  //  UI_AURA_SHELL_DEFAULT_CONTAINER_EVENT_FILTER_H_
