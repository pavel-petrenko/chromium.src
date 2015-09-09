// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"

#include "base/command_line.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/test/ui_controls.h"
#include "ui/base/ui_base_switches.h"
#include "ui/events/event_processor.h"
#include "ui/events/event_utils.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/controls/textfield/textfield_test_api.h"

namespace {

void SetClipboardText(ui::ClipboardType type, const std::string& text) {
  ui::ScopedClipboardWriter(type).WriteText(base::ASCIIToUTF16(text));
}

}  // namespace

class OmniboxViewViewsTest : public InProcessBrowserTest {
 protected:
  OmniboxViewViewsTest() {}

  static void GetOmniboxViewForBrowser(const Browser* browser,
                                       OmniboxView** omnibox_view) {
    BrowserWindow* window = browser->window();
    ASSERT_TRUE(window);
    LocationBar* location_bar = window->GetLocationBar();
    ASSERT_TRUE(location_bar);
    *omnibox_view = location_bar->GetOmniboxView();
    ASSERT_TRUE(*omnibox_view);
  }

  // Move the mouse to the center of the browser window and left-click.
  void ClickBrowserWindowCenter() {
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(
        BrowserView::GetBrowserViewForBrowser(
            browser())->GetBoundsInScreen().CenterPoint()));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(ui_controls::LEFT,
                                                   ui_controls::DOWN));
    ASSERT_TRUE(
        ui_test_utils::SendMouseEventsSync(ui_controls::LEFT, ui_controls::UP));
  }

  // Press and release the mouse at the specified locations.  If
  // |release_offset| differs from |press_offset|, the mouse will be moved
  // between the press and release.
  void Click(ui_controls::MouseButton button,
             const gfx::Point& press_location,
             const gfx::Point& release_location) {
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(press_location));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(button, ui_controls::DOWN));

    if (press_location != release_location)
      ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(release_location));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(button, ui_controls::UP));
  }

  // Tap the center of the browser window.
  void TapBrowserWindowCenter() {
    gfx::Point center = BrowserView::GetBrowserViewForBrowser(
        browser())->GetBoundsInScreen().CenterPoint();
    ui::test::EventGenerator generator(browser()->window()->GetNativeWindow());
    generator.GestureTapAt(center);
  }

  // Touch down and release at the specified locations.
  void Tap(const gfx::Point& press_location,
           const gfx::Point& release_location) {
    ui::test::EventGenerator generator(browser()->window()->GetNativeWindow());
    if (press_location == release_location) {
      generator.GestureTapAt(press_location);
    } else {
      generator.GestureScrollSequence(press_location,
                                      release_location,
                                      base::TimeDelta::FromMilliseconds(10),
                                      1);
    }
  }

 private:
  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
    chrome::FocusLocationBar(browser());
    ASSERT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  }

  DISALLOW_COPY_AND_ASSIGN(OmniboxViewViewsTest);
};

IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, PasteAndGoDoesNotLeavePopupOpen) {
  OmniboxView* view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &view));
  OmniboxViewViews* omnibox_view_views = static_cast<OmniboxViewViews*>(view);

  // Put an URL on the clipboard.
  SetClipboardText(ui::CLIPBOARD_TYPE_COPY_PASTE, "http://www.example.com/");

  // Paste and go.
  omnibox_view_views->ExecuteCommand(IDS_PASTE_AND_GO, ui::EF_NONE);

  // The popup should not be open.
  EXPECT_FALSE(view->model()->popup_model()->IsOpen());
}

#if defined(OS_WIN)
// flaky on Windows - http://crbug.com/523255
#define MAYBE_SelectAllOnClick DISABLED_SelectAllOnClick
#else
#define MAYBE_SelectAllOnClick SelectAllOnClick
#endif

IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, MAYBE_SelectAllOnClick) {
  OmniboxView* omnibox_view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &omnibox_view));
  omnibox_view->SetUserText(base::ASCIIToUTF16("http://www.google.com/"));

  // Take the focus away from the omnibox.
  ASSERT_NO_FATAL_FAILURE(ClickBrowserWindowCenter());
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  // Clicking in the omnibox should take focus and select all text.
  const gfx::Rect omnibox_bounds = BrowserView::GetBrowserViewForBrowser(
        browser())->GetViewByID(VIEW_ID_OMNIBOX)->GetBoundsInScreen();
  const gfx::Point click_location = omnibox_bounds.CenterPoint();
  ASSERT_NO_FATAL_FAILURE(Click(ui_controls::LEFT,
                                click_location, click_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_TRUE(omnibox_view->IsSelectAll());

  // Clicking in another view should clear focus and the selection.
  ASSERT_NO_FATAL_FAILURE(ClickBrowserWindowCenter());
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  // Clicking in the omnibox again should take focus and select all text again.
  ASSERT_NO_FATAL_FAILURE(Click(ui_controls::LEFT,
                                click_location, click_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_TRUE(omnibox_view->IsSelectAll());

  // Clicking another omnibox spot should keep focus but clear the selection.
  omnibox_view->SelectAll(false);
  const gfx::Point click2_location = omnibox_bounds.origin() +
      gfx::Vector2d(omnibox_bounds.width() / 4, omnibox_bounds.height() / 4);
  ASSERT_NO_FATAL_FAILURE(Click(ui_controls::LEFT,
                                click2_location, click2_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  // Take the focus away and click in the omnibox again, but drag a bit before
  // releasing.  We should focus the omnibox but not select all of its text.
  ASSERT_NO_FATAL_FAILURE(ClickBrowserWindowCenter());
  ASSERT_NO_FATAL_FAILURE(Click(ui_controls::LEFT,
                                click_location, click2_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  // Middle-click is only handled on Linux, by pasting the selection clipboard
  // and moving the cursor after the pasted text instead of selecting-all.
  ASSERT_NO_FATAL_FAILURE(ClickBrowserWindowCenter());
  ASSERT_NO_FATAL_FAILURE(Click(ui_controls::MIDDLE,
                                click_location, click_location));
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());
#else
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());
#endif  // OS_LINUX && !OS_CHROMEOS
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, SelectionClipboard) {
  OmniboxView* omnibox_view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &omnibox_view));
  omnibox_view->SetUserText(base::ASCIIToUTF16("http://www.google.com/"));
  OmniboxViewViews* omnibox_view_views =
      static_cast<OmniboxViewViews*>(omnibox_view);
  ASSERT_TRUE(omnibox_view_views);
  gfx::RenderText* render_text = omnibox_view_views->GetRenderText();

  // Take the focus away from the omnibox.
  ASSERT_NO_FATAL_FAILURE(TapBrowserWindowCenter());
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  size_t cursor_position = 14;
  int cursor_x = render_text->GetCursorBounds(
      gfx::SelectionModel(cursor_position, gfx::CURSOR_FORWARD), false).x();
  gfx::Point click_location = omnibox_view_views->GetBoundsInScreen().origin();
  click_location.Offset(cursor_x + render_text->display_rect().x(),
                        omnibox_view_views->height() / 2);

  // Middle click focuses the omnibox, pastes, and sets a trailing cursor.
  // Select-all on focus shouldn't alter the selection clipboard or cursor.
  SetClipboardText(ui::CLIPBOARD_TYPE_SELECTION, "123");
  ASSERT_NO_FATAL_FAILURE(Click(ui_controls::MIDDLE,
                                click_location, click_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());
  EXPECT_EQ(base::ASCIIToUTF16("http://www.goo123gle.com/"),
            omnibox_view->GetText());
  EXPECT_EQ(17U, omnibox_view_views->GetCursorPosition());

  // Middle clicking again, with focus, pastes and updates the cursor.
  SetClipboardText(ui::CLIPBOARD_TYPE_SELECTION, "4567");
  ASSERT_NO_FATAL_FAILURE(Click(ui_controls::MIDDLE,
                                click_location, click_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());
  EXPECT_EQ(base::ASCIIToUTF16("http://www.goo4567123gle.com/"),
            omnibox_view->GetText());
  EXPECT_EQ(18U, omnibox_view_views->GetCursorPosition());
}
#endif  // OS_LINUX && !OS_CHROMEOS

IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, SelectAllOnTap) {
  OmniboxView* omnibox_view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &omnibox_view));
  omnibox_view->SetUserText(base::ASCIIToUTF16("http://www.google.com/"));

  // Take the focus away from the omnibox.
  ASSERT_NO_FATAL_FAILURE(TapBrowserWindowCenter());
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  // Tapping in the omnibox should take focus and select all text.
  const gfx::Rect omnibox_bounds = BrowserView::GetBrowserViewForBrowser(
      browser())->GetViewByID(VIEW_ID_OMNIBOX)->GetBoundsInScreen();
  const gfx::Point tap_location = omnibox_bounds.CenterPoint();
  ASSERT_NO_FATAL_FAILURE(Tap(tap_location, tap_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_TRUE(omnibox_view->IsSelectAll());

  // Tapping in another view should clear focus and the selection.
  ASSERT_NO_FATAL_FAILURE(TapBrowserWindowCenter());
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  // Tapping in the omnibox again should take focus and select all text again.
  ASSERT_NO_FATAL_FAILURE(Tap(tap_location, tap_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_TRUE(omnibox_view->IsSelectAll());

  // Tapping another omnibox spot should keep focus and selection.
  omnibox_view->SelectAll(false);
  const gfx::Point tap2_location = omnibox_bounds.origin() +
      gfx::Vector2d(omnibox_bounds.width() / 4, omnibox_bounds.height() / 4);
  ASSERT_NO_FATAL_FAILURE(Tap(tap2_location, tap2_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  // We don't test if the all text is selected because it depends on whether or
  // not there was text under the tap, which appears to be flaky.

  // Take the focus away and tap in the omnibox again, but drag a bit before
  // releasing.  We should focus the omnibox but not select all of its text.
  ASSERT_NO_FATAL_FAILURE(TapBrowserWindowCenter());
  ASSERT_NO_FATAL_FAILURE(Tap(tap_location, tap2_location));
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());
}

IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, SelectAllOnTabToFocus) {
  OmniboxView* omnibox_view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &omnibox_view));
  ui_test_utils::NavigateToURL(browser(), GURL("http://www.google.com/"));
  // RevertAll after navigation to invalidate the selection range saved on blur.
  omnibox_view->RevertAll();
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_FALSE(omnibox_view->IsSelectAll());

  // Pressing tab to focus the omnibox should select all text.
  while (!ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX)) {
    ASSERT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_TAB,
                                                false, false, false, false));
  }
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_TRUE(omnibox_view->IsSelectAll());
}

IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, CloseOmniboxPopupOnTextDrag) {
  OmniboxView* omnibox_view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &omnibox_view));
  OmniboxViewViews* omnibox_view_views =
      static_cast<OmniboxViewViews*>(omnibox_view);

  // Populate suggestions for the omnibox popup.
  AutocompleteController* autocomplete_controller =
      omnibox_view->model()->popup_model()->autocomplete_controller();
  AutocompleteResult& results = autocomplete_controller->result_;
  ACMatches matches;
  AutocompleteMatch match;
  match.destination_url = GURL("http://autocomplete-result/");
  match.allowed_to_be_default_match = true;
  match.type = AutocompleteMatchType::HISTORY_TITLE;
  match.relevance = 500;
  matches.push_back(match);
  match.destination_url = GURL("http://autocomplete-result2/");
  matches.push_back(match);
  results.AppendMatches(AutocompleteInput(), matches);
  results.SortAndCull(
      AutocompleteInput(),
      std::string(),
      TemplateURLServiceFactory::GetForProfile(browser()->profile()));

  // The omnibox popup should open with suggestions displayed.
  omnibox_view->model()->popup_model()->OnResultChanged();
  EXPECT_TRUE(omnibox_view->model()->popup_model()->IsOpen());

  // The omnibox text should be selected.
  EXPECT_TRUE(omnibox_view->IsSelectAll());

  // Simulate a mouse click before dragging the mouse.
  gfx::Point point(omnibox_view_views->x(), omnibox_view_views->y());
  ui::MouseEvent pressed(ui::ET_MOUSE_PRESSED, point, point,
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                         ui::EF_LEFT_MOUSE_BUTTON);
  omnibox_view_views->OnMousePressed(pressed);
  EXPECT_TRUE(omnibox_view->model()->popup_model()->IsOpen());

  // Simulate a mouse drag of the omnibox text, and the omnibox should close.
  ui::MouseEvent dragged(ui::ET_MOUSE_DRAGGED, point, point,
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);
  omnibox_view_views->OnMouseDragged(dragged);

  EXPECT_FALSE(omnibox_view->model()->popup_model()->IsOpen());
}

IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, BackgroundIsOpaque) {
  // The omnibox text should be rendered on an opaque background. Otherwise, we
  // can't use subpixel rendering.
  OmniboxViewViews* view = BrowserView::GetBrowserViewForBrowser(browser())->
      toolbar()->location_bar()->omnibox_view();
  ASSERT_TRUE(view);
  EXPECT_FALSE(view->GetRenderText()->subpixel_rendering_suppressed());
}

// Tests if executing a command hides touch editing handles.
IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest,
                       DeactivateTouchEditingOnExecuteCommand) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableTouchEditing);

  OmniboxView* view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &view));
  OmniboxViewViews* omnibox_view_views = static_cast<OmniboxViewViews*>(view);
  views::TextfieldTestApi textfield_test_api(omnibox_view_views);

  // Put a URL on the clipboard.
  SetClipboardText(ui::CLIPBOARD_TYPE_COPY_PASTE, "http://www.example.com/");

  // Tap to activate touch editing.
  gfx::Point omnibox_center =
      omnibox_view_views->GetBoundsInScreen().CenterPoint();
  Tap(omnibox_center, omnibox_center);
  EXPECT_TRUE(textfield_test_api.touch_selection_controller());

  // Execute a command and check if it deactivate touch editing. Paste & Go is
  // chosen since it is specific to Omnibox and its execution wouldn't be
  // delgated to the base Textfield class.
  omnibox_view_views->ExecuteCommand(IDS_PASTE_AND_GO, ui::EF_NONE);
  EXPECT_FALSE(textfield_test_api.touch_selection_controller());
}

IN_PROC_BROWSER_TEST_F(OmniboxViewViewsTest, FocusedTextInputClient) {
  chrome::FocusLocationBar(browser());
  OmniboxView* view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &view));
  OmniboxViewViews* omnibox_view_views = static_cast<OmniboxViewViews*>(view);
  ui::InputMethod* input_method =
      omnibox_view_views->GetWidget()->GetInputMethod();
  EXPECT_EQ(static_cast<ui::TextInputClient*>(omnibox_view_views),
            input_method->GetTextInputClient());
}
