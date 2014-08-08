// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RENDERER_CONTEXT_MENU_RENDER_TOOLKIT_DELEGATE_VIEWS_H_
#define COMPONENTS_RENDERER_CONTEXT_MENU_RENDER_TOOLKIT_DELEGATE_VIEWS_H_

#include "components/renderer_context_menu/render_view_context_menu_base.h"
#include "ui/base/ui_base_types.h"

namespace gfx {
class Point;
}

namespace views {
class MenuItemView;
class MenuModelAdapter;
class MenuRunner;
class Widget;
}

namespace ui {
class SimpleMenuModel;
}

class ToolkitDelegateViews : public RenderViewContextMenuBase::ToolkitDelegate {
 public:
  ToolkitDelegateViews();
  virtual ~ToolkitDelegateViews();

  void RunMenuAt(views::Widget* parent,
                 const gfx::Point& point,
                 ui::MenuSourceType type);

 private:
  // ToolkitDelegate:
  virtual void Init(ui::SimpleMenuModel* menu_model) OVERRIDE;
  virtual void Cancel() OVERRIDE;
  virtual void UpdateMenuItem(int command_id,
                              bool enabled,
                              bool hidden,
                              const base::string16& title) OVERRIDE;

  scoped_ptr<views::MenuModelAdapter> menu_adapter_;
  scoped_ptr<views::MenuRunner> menu_runner_;

  // Weak. Owned by menu_runner_;
  views::MenuItemView* menu_view_;

  DISALLOW_COPY_AND_ASSIGN(ToolkitDelegateViews);
};

#endif  // COMPONENTS_RENDERER_CONTEXT_MENU_RENDER_TOOLKIT_DELEGATE_VIEWS_H_
