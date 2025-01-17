// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.app.appmenu;

import android.content.pm.ActivityInfo;
import android.support.test.InstrumentationRegistry;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ListView;

import androidx.test.filters.SmallTest;

import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.task.PostTask;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisableIf;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.compositor.layouts.EmptyOverviewModeObserver;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.share.ShareUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuTestSupport;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.ChromeRenderTestRule;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.test.util.Criteria;
import org.chromium.content_public.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.test.util.TestThreadUtils;
import org.chromium.ui.test.util.UiDisableIf;
import org.chromium.ui.test.util.UiRestriction;

import java.io.IOException;
import java.util.concurrent.TimeoutException;

/**
 * Tests tabbed mode app menu popup.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TabbedAppMenuTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    @Rule
    public ChromeRenderTestRule mRenderTestRule =
            ChromeRenderTestRule.Builder.withPublicCorpus().build();

    private static final String TEST_URL = UrlUtils.encodeHtmlDataUri("<html>foo</html>");
    private static final String TEST_URL2 = UrlUtils.encodeHtmlDataUri("<html>bar</html>");

    private AppMenuHandler mAppMenuHandler;

    int mLastSelectedItemId = -1;
    private Callback<MenuItem> mItemSelectedCallback =
            (item) -> mLastSelectedItemId = item.getItemId();

    @Before
    public void setUp() {
        // We need list selection; ensure we are not in touch mode.
        InstrumentationRegistry.getInstrumentation().setInTouchMode(false);

        mActivityTestRule.startMainActivityWithURL(TEST_URL);

        AppMenuTestSupport.overrideOnOptionItemSelectedListener(
                mActivityTestRule.getAppMenuCoordinator(), mItemSelectedCallback);
        mAppMenuHandler = mActivityTestRule.getAppMenuCoordinator().getAppMenuHandler();

        showAppMenuAndAssertMenuShown();

        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> getListView().setSelection(0));
        CriteriaHelper.pollInstrumentationThread(
                () -> Criteria.checkThat(getCurrentFocusedRow(), Matchers.is(0)));
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    /**
     * Verify opening a new tab from the menu.
     */
    @Test
    @SmallTest
    @Feature({"Browser", "Main"})
    public void testMenuNewTab() {
        final int tabCountBefore = mActivityTestRule.getActivity().getCurrentTabModel().getCount();
        ChromeTabUtils.newTabFromMenu(InstrumentationRegistry.getInstrumentation(),
                (ChromeTabbedActivity) mActivityTestRule.getActivity());
        final int tabCountAfter = mActivityTestRule.getActivity().getCurrentTabModel().getCount();
        Assert.assertTrue("Expected: " + (tabCountBefore + 1) + " Got: " + tabCountAfter,
                tabCountBefore + 1 == tabCountAfter);
    }

    /**
     * Test bounds when accessing the menu through the keyboard.
     * Make sure that the menu stays open when trying to move past the first and last items.
     */
    @Test
    @SmallTest
    @Feature({"Browser", "Main"})
    public void testKeyboardMenuBoundaries() {
        moveToBoundary(false, true);
        Assert.assertEquals(getCount() - 1, getCurrentFocusedRow());
        moveToBoundary(true, true);
        Assert.assertEquals(0, getCurrentFocusedRow());
        moveToBoundary(false, true);
        Assert.assertEquals(getCount() - 1, getCurrentFocusedRow());
    }

    /**
     * Test that typing ENTER immediately opening the menu works.
     */
    @Test
    @SmallTest
    @Feature({"Browser", "Main"})
    public void testKeyboardMenuEnterOnOpen() {
        hitEnterAndAssertAppMenuDismissed();
    }

    /**
     * Test that hitting ENTER past the top item doesn't crash Chrome.
     */
    @Test
    @SmallTest
    @Feature({"Browser", "Main"})
    public void testKeyboardEnterAfterMovePastTopItem() {
        moveToBoundary(true, true);
        Assert.assertEquals(0, getCurrentFocusedRow());
        hitEnterAndAssertAppMenuDismissed();
    }

    /**
     * Test that hitting ENTER past the bottom item doesn't crash Chrome.
     * Catches regressions for http://crbug.com/181067
     */
    @Test
    @SmallTest
    @Feature({"Browser", "Main"})
    public void testKeyboardEnterAfterMovePastBottomItem() {
        moveToBoundary(false, true);
        Assert.assertEquals(getCount() - 1, getCurrentFocusedRow());
        hitEnterAndAssertAppMenuDismissed();
    }

    /**
     * Test that hitting ENTER on the top item actually triggers the top item.
     * Catches regressions for https://crbug.com/191239 for shrunken menus.
     */
    /*
    @SmallTest
    @Feature({"Browser", "Main"})
    */
    @Test
    @DisabledTest(message = "crbug.com/945861")
    public void testKeyboardMenuEnterOnTopItemLandscape() {
        mActivityTestRule.getActivity().setRequestedOrientation(
                ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        showAppMenuAndAssertMenuShown();
        moveToBoundary(true, false);
        Assert.assertEquals(0, getCurrentFocusedRow());
        hitEnterAndAssertAppMenuDismissed();
    }

    /**
     * Test that hitting ENTER on the top item doesn't crash Chrome.
     */
    @Test
    @SmallTest
    @Feature({"Browser", "Main"})
    public void testKeyboardMenuEnterOnTopItemPortrait() {
        mActivityTestRule.getActivity().setRequestedOrientation(
                ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        showAppMenuAndAssertMenuShown();
        moveToBoundary(true, false);
        Assert.assertEquals(0, getCurrentFocusedRow());
        hitEnterAndAssertAppMenuDismissed();
    }

    @Test
    @SmallTest
    @Feature({"Browser", "Main"})
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    public void testHideMenuOnToggleOverview() throws TimeoutException {
        CallbackHelper overviewModeFinishedShowingCallback = new CallbackHelper();
        OverviewModeBehavior.OverviewModeObserver overviewModeObserver =
                new EmptyOverviewModeObserver() {
                    @Override
                    public void onOverviewModeFinishedShowing() {
                        overviewModeFinishedShowingCallback.notifyCalled();
                    }
                };

        // App menu is shown during setup.
        Assert.assertTrue("App menu should be showing.", mAppMenuHandler.isAppMenuShowing());
        Assert.assertFalse("Overview shouldn't be showing.",
                mActivityTestRule.getActivity().getOverviewModeBehavior().overviewVisible());

        TestThreadUtils.runOnUiThreadBlocking(() -> {
            mActivityTestRule.getActivity().getLayoutManager().addOverviewModeObserver(
                    overviewModeObserver);
            mActivityTestRule.getActivity().getLayoutManager().showOverview(false);
        });
        overviewModeFinishedShowingCallback.waitForCallback(0);

        Assert.assertTrue("Overview should be showing.",
                mActivityTestRule.getActivity().getOverviewModeBehavior().overviewVisible());
        Assert.assertFalse("App menu shouldn't be showing.", mAppMenuHandler.isAppMenuShowing());
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertTrue("App menu should be allowed to show.",
                    AppMenuTestSupport.shouldShowAppMenu(
                            mActivityTestRule.getAppMenuCoordinator()));
        });
        showAppMenuAndAssertMenuShown();

        TestThreadUtils.runOnUiThreadBlocking(
                () -> mActivityTestRule.getActivity().getLayoutManager().hideOverview(false));
        Assert.assertFalse("Overview shouldn't be showing.",
                mActivityTestRule.getActivity().getOverviewModeBehavior().overviewVisible());
        CriteriaHelper.pollUiThread(
                () -> !mAppMenuHandler.isAppMenuShowing(), "App menu shouldn't be showing.");
    }

    @Test
    @SmallTest
    @Feature({"Browser", "Main", "Bookmark", "RenderTest"})
    @DisableFeatures({ChromeFeatureList.TABBED_APP_OVERFLOW_MENU_REGROUP,
            ChromeFeatureList.TABBED_APP_OVERFLOW_MENU_ICONS})
    @DisableIf.Device(type = {UiDisableIf.TABLET}) // See https://crbug.com/1065043.
    public void
    testBookmarkMenuItem() throws IOException {
        MenuItem bookmarkStar =
                AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                        .findItem(R.id.bookmark_this_page_id);
        Assert.assertFalse("Bookmark item should not be checked.", bookmarkStar.isChecked());
        Assert.assertEquals("Incorrect content description.",
                mActivityTestRule.getActivity().getString(R.string.menu_bookmark),
                bookmarkStar.getTitleCondensed());
        mRenderTestRule.render(getListView().getChildAt(0), "icon_row");

        TestThreadUtils.runOnUiThreadBlocking(() -> mAppMenuHandler.hideAppMenu());
        AppMenuPropertiesDelegateImpl.setPageBookmarkedForTesting(true);
        showAppMenuAndAssertMenuShown();
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        bookmarkStar = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                               .findItem(R.id.bookmark_this_page_id);
        Assert.assertTrue("Bookmark item should be checked.", bookmarkStar.isChecked());
        Assert.assertEquals("Incorrect content description for bookmarked page.",
                mActivityTestRule.getActivity().getString(R.string.edit_bookmark),
                bookmarkStar.getTitleCondensed());
        mRenderTestRule.render(getListView().getChildAt(0), "icon_row_page_bookmarked");

        AppMenuPropertiesDelegateImpl.setPageBookmarkedForTesting(null);
    }

    @Test
    @SmallTest
    @Feature({"Browser", "Main", "RenderTest"})
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    @EnableFeatures({ChromeFeatureList.TABBED_APP_OVERFLOW_MENU_REGROUP + "<Study"})
    @CommandLineFlags.Add({"force-fieldtrials=Study/Group",
            "force-fieldtrial-params=Study.Group:action_bar/backward_button"})
    public void
    testBackButtonMenuItem() throws IOException {
        MenuItem backArrow = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                                     .findItem(R.id.backward_menu_id);
        Assert.assertFalse("Backward button item should be disabled.", backArrow.isEnabled());
        Assert.assertEquals("Incorrect content description.",
                mActivityTestRule.getActivity().getString(R.string.back),
                backArrow.getTitleCondensed());
        mRenderTestRule.render(getListView().getChildAt(0), "icon_row_backward_diabled");

        TestThreadUtils.runOnUiThreadBlocking(() -> mAppMenuHandler.hideAppMenu());
        mActivityTestRule.loadUrl(TEST_URL2);
        showAppMenuAndAssertMenuShown();
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        backArrow = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                            .findItem(R.id.backward_menu_id);
        Assert.assertTrue("Backward button item should be enabled.", backArrow.isEnabled());

        LinearLayout actionBar = (LinearLayout) getListView().getChildAt(0);
        Assert.assertEquals(5, actionBar.getChildCount());

        mRenderTestRule.render(getListView().getChildAt(0), "icon_row_backward_enabled");

        selectMenuItem(R.id.backward_menu_id);
        TestThreadUtils.runOnUiThreadBlocking(() -> mAppMenuHandler.hideAppMenu());
        ShareUtils shareUtils = new ShareUtils();
        CriteriaHelper.pollUiThread(() -> {
            Tab tab = mActivityTestRule.getActivity().getActivityTab();
            Criteria.checkThat(tab, Matchers.notNullValue());
            Criteria.checkThat(tab.getUrlString(), Matchers.is(TEST_URL));
            Criteria.checkThat(shareUtils.shouldEnableShare(tab), Matchers.is(false));
        });
        showAppMenuAndAssertMenuShown();
        backArrow = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                            .findItem(R.id.backward_menu_id);
        Assert.assertFalse("Backward button item should be disabled.", backArrow.isEnabled());
    }

    @Test
    @SmallTest
    @Feature({"Browser", "Main", "RenderTest"})
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    @EnableFeatures({ChromeFeatureList.TABBED_APP_OVERFLOW_MENU_REGROUP + "<Study"})
    @CommandLineFlags.Add({"force-fieldtrials=Study/Group",
            "force-fieldtrial-params=Study.Group:action_bar/share_button"})
    public void
    testShareButtonMenuItem() throws IOException {
        MenuItem shareButton = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                                       .findItem(R.id.share_menu_button_id);
        Assert.assertFalse("Share button item should be disabled.", shareButton.isEnabled());
        Assert.assertEquals("Incorrect content description.",
                mActivityTestRule.getActivity().getString(R.string.share),
                shareButton.getTitleCondensed());
        mRenderTestRule.render(getListView().getChildAt(0), "icon_row_share_diabled");

        TestThreadUtils.runOnUiThreadBlocking(() -> mAppMenuHandler.hideAppMenu());
        mActivityTestRule.loadUrl(mActivityTestRule.getTestServer().getURL(
                "/chrome/test/data/android/contextualsearch/tap_test.html"));
        showAppMenuAndAssertMenuShown();
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        shareButton = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                              .findItem(R.id.share_menu_button_id);
        Assert.assertTrue("Share button item should be enabled.", shareButton.isEnabled());

        LinearLayout actionBar = (LinearLayout) getListView().getChildAt(0);
        Assert.assertEquals(5, actionBar.getChildCount());
        mRenderTestRule.render(getListView().getChildAt(0), "icon_row_share_enabled");
    }

    @Test
    @SmallTest
    @Feature({"Browser", "Main", "RenderTest"})
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    @EnableFeatures({ChromeFeatureList.TABBED_APP_OVERFLOW_MENU_THREE_BUTTON_ACTIONBAR + "<Study"})
    @CommandLineFlags.Add({"force-fieldtrials=Study/Group",
            "force-fieldtrial-params=Study.Group:three_button_action_bar/action_chip_view"})
    public void
    testActionChipViewMenuItem() throws IOException {
        LinearLayout actionBar = (LinearLayout) getListView().getChildAt(0);
        Assert.assertEquals(3, actionBar.getChildCount());
        mRenderTestRule.render(getListView().getChildAt(0), "icon_row_three_buttons");

        int downloadRowIndex = findIndexOfMenuItemById(R.id.downloads_row_menu_id);
        Assert.assertNotEquals("No download row found.", -1, downloadRowIndex);
        mRenderTestRule.render(
                getListView().getChildAt(downloadRowIndex), "download_row_action_chip_view");

        MenuItem bookmarkRow = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                                       .findItem(R.id.all_bookmarks_row_menu_id);
        MenuItem bookmarkMenuItem = bookmarkRow.getSubMenu().getItem(1);
        Assert.assertFalse("Bookmark item should not be checked.", bookmarkMenuItem.isChecked());
        int bookmarkRowIndex = findIndexOfMenuItemById(R.id.all_bookmarks_row_menu_id);
        Assert.assertTrue("No bookmark row found.", bookmarkRowIndex != -1);
        mRenderTestRule.render(
                getListView().getChildAt(bookmarkRowIndex), "bookmark_row_action_chip_view");

        TestThreadUtils.runOnUiThreadBlocking(() -> mAppMenuHandler.hideAppMenu());
        AppMenuPropertiesDelegateImpl.setPageBookmarkedForTesting(true);
        showAppMenuAndAssertMenuShown();
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        bookmarkRow = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                              .findItem(R.id.all_bookmarks_row_menu_id);
        bookmarkMenuItem = bookmarkRow.getSubMenu().getItem(1);
        Assert.assertTrue("Bookmark item should be checked.", bookmarkMenuItem.isChecked());
        mRenderTestRule.render(getListView().getChildAt(bookmarkRowIndex),
                "bookmark_row_action_chip_view_bookmarked");

        AppMenuPropertiesDelegateImpl.setPageBookmarkedForTesting(null);
    }

    @Test
    @SmallTest
    @Feature({"Browser", "Main", "RenderTest"})
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    @EnableFeatures({ChromeFeatureList.TABBED_APP_OVERFLOW_MENU_THREE_BUTTON_ACTIONBAR + "<Study"})
    @CommandLineFlags.Add({"force-fieldtrials=Study/Group",
            "force-fieldtrial-params=Study.Group:three_button_action_bar/destination_chip_view"})
    public void
    testDestinationChipViewMenuItem() throws IOException {
        LinearLayout actionBar = (LinearLayout) getListView().getChildAt(0);
        Assert.assertEquals(3, actionBar.getChildCount());
        mRenderTestRule.render(getListView().getChildAt(0), "icon_row_three_buttons");

        int downloadRowIndex = findIndexOfMenuItemById(R.id.downloads_row_menu_id);
        Assert.assertNotEquals("No download row found.", -1, downloadRowIndex);
        mRenderTestRule.render(
                getListView().getChildAt(downloadRowIndex), "download_row_destination_chip_view");

        MenuItem bookmarkRow = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                                       .findItem(R.id.all_bookmarks_row_menu_id);
        MenuItem bookmarkMenuItem = bookmarkRow.getSubMenu().getItem(1);
        Assert.assertFalse("Bookmark item should not be checked.", bookmarkMenuItem.isChecked());
        int bookmarkRowIndex = findIndexOfMenuItemById(R.id.all_bookmarks_row_menu_id);
        Assert.assertTrue("No bookmark row found.", bookmarkRowIndex != -1);
        mRenderTestRule.render(
                getListView().getChildAt(bookmarkRowIndex), "bookmark_row_destination_chip_view");

        TestThreadUtils.runOnUiThreadBlocking(() -> mAppMenuHandler.hideAppMenu());
        AppMenuPropertiesDelegateImpl.setPageBookmarkedForTesting(true);
        showAppMenuAndAssertMenuShown();
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        bookmarkRow = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator())
                              .findItem(R.id.all_bookmarks_row_menu_id);
        bookmarkMenuItem = bookmarkRow.getSubMenu().getItem(1);
        Assert.assertTrue("Bookmark item should be checked.", bookmarkMenuItem.isChecked());
        mRenderTestRule.render(getListView().getChildAt(bookmarkRowIndex),
                "bookmark_row_destination_chip_view_bookmarked");

        AppMenuPropertiesDelegateImpl.setPageBookmarkedForTesting(null);
    }

    @Test
    @SmallTest
    @Feature({"Browser", "Main", "RenderTest"})
    @EnableFeatures({ChromeFeatureList.TABBED_APP_OVERFLOW_MENU_REGROUP})
    public void testDividerLineMenuItem() throws IOException {
        int firstDividerLineIndex = findIndexOfMenuItemById(R.id.divider_line_id);
        Assert.assertTrue("No divider line found.", firstDividerLineIndex != -1);
        mRenderTestRule.render(getListView().getChildAt(firstDividerLineIndex), "divider_line");
    }

    private void showAppMenuAndAssertMenuShown() {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AppMenuTestSupport.showAppMenu(mActivityTestRule.getAppMenuCoordinator(), null, false);
            Assert.assertTrue(mAppMenuHandler.isAppMenuShowing());
        });
    }

    private void hitEnterAndAssertAppMenuDismissed() {
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        pressKey(KeyEvent.KEYCODE_ENTER);
        CriteriaHelper.pollInstrumentationThread(
                () -> !mAppMenuHandler.isAppMenuShowing(), "AppMenu did not dismiss");
    }

    private void moveToBoundary(boolean towardsTop, boolean movePast) {
        // Move to the boundary.
        final int end = towardsTop ? 0 : getCount() - 1;
        int increment = towardsTop ? -1 : 1;
        for (int index = getCurrentFocusedRow(); index != end; index += increment) {
            pressKey(towardsTop ? KeyEvent.KEYCODE_DPAD_UP : KeyEvent.KEYCODE_DPAD_DOWN);
            final int expectedPosition = index + increment;
            CriteriaHelper.pollInstrumentationThread(() -> {
                Criteria.checkThat(getCurrentFocusedRow(), Matchers.is(expectedPosition));
            });
        }

        // Try moving past it by one.
        if (movePast) {
            pressKey(towardsTop ? KeyEvent.KEYCODE_DPAD_UP : KeyEvent.KEYCODE_DPAD_DOWN);
            CriteriaHelper.pollInstrumentationThread(
                    () -> Criteria.checkThat(getCurrentFocusedRow(), Matchers.is(end)));
        }

        // The menu should stay open.
        Assert.assertTrue(mAppMenuHandler.isAppMenuShowing());
    }

    private void pressKey(final int keycode) {
        final View view = getListView();
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            view.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keycode));
            view.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keycode));
        });
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    private int getCurrentFocusedRow() {
        ListView listView = getListView();
        if (listView == null) return ListView.INVALID_POSITION;
        return listView.getSelectedItemPosition();
    }

    private int getCount() {
        ListView listView = getListView();
        if (listView == null) return 0;
        return listView.getCount();
    }

    private ListView getListView() {
        return AppMenuTestSupport.getListView(mActivityTestRule.getAppMenuCoordinator());
    }

    private void selectMenuItem(int id) {
        CriteriaHelper.pollUiThread(
                () -> { mActivityTestRule.getActivity().onMenuOrKeyboardAction(id, true); });
    }

    private int findIndexOfMenuItemById(int id) {
        Menu menu = AppMenuTestSupport.getMenu(mActivityTestRule.getAppMenuCoordinator());
        int firstMenuItemIndex = -1;
        boolean foundMenuItem = false;
        for (int i = 0; i < menu.size(); i++) {
            MenuItem item = menu.getItem(i);
            if (item.isVisible()) {
                firstMenuItemIndex++;
            }
            if (item.getItemId() == id) {
                foundMenuItem = true;
                break;
            }
        }

        return foundMenuItem ? firstMenuItemIndex : -1;
    }
}
