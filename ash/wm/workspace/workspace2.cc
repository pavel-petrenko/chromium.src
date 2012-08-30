// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace/workspace2.h"

#include "ash/shell_window_ids.h"
#include "ash/wm/property_util.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_util.h"
#include "ash/wm/workspace/workspace_event_filter.h"
#include "ash/wm/workspace/workspace_manager2.h"
#include "ui/aura/window.h"

namespace ash {
namespace internal {

Workspace2::Workspace2(WorkspaceManager2* manager,
                       aura::Window* parent,
                       bool is_maximized)
    : is_maximized_(is_maximized),
      workspace_manager_(manager),
      window_(new aura::Window(NULL)),
      event_filter_(new WorkspaceEventFilter(window_)) {
  window_->SetProperty(internal::kChildWindowVisibilityChangesAnimatedKey,
                       true);
  window_->set_id(kShellWindowId_WorkspaceContainer);
  window_->SetName("WorkspaceContainer");
  window_->Init(ui::LAYER_NOT_DRAWN);
  window_->Hide();
  window_->SetParent(parent);
  window_->SetEventFilter(event_filter_);
  window_->SetProperty(internal::kUsesScreenCoordinatesKey, true);
}

Workspace2::~Workspace2() {
  // ReleaseWindow() should have been invoked before we're deleted.
  DCHECK(!window_);
}

aura::Window* Workspace2::ReleaseWindow() {
  // Remove the LayoutManager and EventFilter as they refer back to us and/or
  // WorkspaceManager.
  window_->SetLayoutManager(NULL);
  window_->SetEventFilter(NULL);
  aura::Window* window = window_;
  window_ = NULL;
  event_filter_ = NULL;
  return window;
}

void Workspace2::SetGridSize(int grid_size) {
  event_filter_->set_grid_size(grid_size);
}

bool Workspace2::ShouldMoveToPending() const {
  if (!is_maximized_)
    return false;

  bool has_visible_non_maximized_window = false;
  for (size_t i = 0; i < window_->children().size(); ++i) {
    aura::Window* child(window_->children()[i]);
    if (!child->TargetVisibility() || wm::IsWindowMinimized(child))
      continue;
    if (WorkspaceManager2::IsMaximized(child))
      return false;

    if (GetTrackedByWorkspace(child) && !GetPersistsAcrossAllWorkspaces(child))
      has_visible_non_maximized_window = true;
  }
  return !has_visible_non_maximized_window;
}

int Workspace2::GetNumMaximizedWindows() const {
  int count = 0;
  for (size_t i = 0; i < window_->children().size(); ++i) {
    aura::Window* child = window_->children()[i];
    if (WorkspaceManager2::IsMaximized(child) ||
        WorkspaceManager2::WillRestoreMaximized(child)) {
      if (++count == 2)
        return count;
    }
  }
  return count;
}

}  // namespace internal
}  // namespace ash
