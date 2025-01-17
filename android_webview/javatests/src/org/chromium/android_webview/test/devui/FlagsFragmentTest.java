// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test.devui;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.replaceText;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.Matchers.allOf;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.lessThan;
import static org.hamcrest.Matchers.not;

import static org.chromium.android_webview.test.devui.DeveloperUiTestUtils.withCount;

import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.support.test.rule.ActivityTestRule;
import android.view.MotionEvent;
import android.view.View;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.IntDef;
import androidx.test.filters.MediumTest;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.common.AwSwitches;
import org.chromium.android_webview.devui.FlagsFragment;
import org.chromium.android_webview.devui.MainActivity;
import org.chromium.android_webview.devui.R;
import org.chromium.android_webview.test.AwJUnit4ClassRunner;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

/**
 * UI tests for {@link FlagsFragment}.
 */
@RunWith(AwJUnit4ClassRunner.class)
@Batch(Batch.PER_CLASS)
public class FlagsFragmentTest {
    @Rule
    public ActivityTestRule mRule =
            new ActivityTestRule<MainActivity>(MainActivity.class, false, false);

    @Before
    public void setUp() throws Exception {
        Intent intent = new Intent();
        intent.putExtra(MainActivity.FRAGMENT_ID_INTENT_EXTRA, MainActivity.FRAGMENT_ID_FLAGS);
        mRule.launchActivity(intent);
    }

    private CallbackHelper getFlagUiSearchBarListener() {
        final CallbackHelper helper = new CallbackHelper();
        FlagsFragment.setFilterListener(() -> { helper.notifyCalled(); });
        return helper;
    }

    private static Matcher<View> withHintText(final Matcher<String> stringMatcher) {
        return new TypeSafeMatcher<View>() {
            @Override
            public boolean matchesSafely(View view) {
                if (!(view instanceof EditText)) {
                    return false;
                }
                String hint = ((EditText) view).getHint().toString();
                return stringMatcher.matches(hint);
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("with hint text: ");
                stringMatcher.describeTo(description);
            }
        };
    }

    private static Matcher<View> withHintText(final String expectedHint) {
        return withHintText(is(expectedHint));
    }

    @IntDef({CompoundDrawable.START, CompoundDrawable.TOP, CompoundDrawable.END,
            CompoundDrawable.BOTTOM})
    private @interface CompoundDrawable {
        int START = 0;
        int TOP = 1;
        int END = 2;
        int BOTTOM = 3;
    }

    private static Matcher<View> compoundDrawableVisible(@CompoundDrawable int position) {
        return new TypeSafeMatcher<View>() {
            @Override
            public boolean matchesSafely(View view) {
                if (!(view instanceof TextView)) {
                    return false;
                }
                Drawable[] compoundDrawables = ((TextView) view).getCompoundDrawablesRelative();
                Drawable endDrawable = compoundDrawables[position];
                return endDrawable != null;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("with drawable in position " + position);
            }
        };
    }

    // Click a TextView at the start/end/top/bottom. Does not check if any CompoundDrawable drawable
    // is in that position, it just sends a touch event for those coordinates.
    private static void tapCompoundDrawableOnUiThread(
            TextView view, @CompoundDrawable int position) {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            long downTime = SystemClock.uptimeMillis();
            long eventTime = downTime + 50;
            float x = view.getWidth() / 2.0f;
            float y = view.getHeight() / 2.0f;
            if (position == CompoundDrawable.START) {
                x = 0.0f;
            } else if (position == CompoundDrawable.END) {
                x = view.getWidth();
            } else if (position == CompoundDrawable.TOP) {
                y = 0.0f;
            } else if (position == CompoundDrawable.BOTTOM) {
                y = view.getHeight();
            }

            int metaState = 0; // no modifier keys (ex. alt/control), this is just a touch event
            view.dispatchTouchEvent(MotionEvent.obtain(
                    downTime, eventTime, MotionEvent.ACTION_UP, x, y, metaState));
        });
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testSearchEmptyByDefault() throws Throwable {
        onView(withId(R.id.flag_search_bar)).check(matches(withText("")));
        onView(withId(R.id.flag_search_bar)).check(matches(withHintText("Search flags")));

        // Magnifier should always be visible, "clear text" icon should be hidden
        onView(withId(R.id.flag_search_bar))
                .check(matches(compoundDrawableVisible(CompoundDrawable.START)));
        onView(withId(R.id.flag_search_bar))
                .check(matches(not(compoundDrawableVisible(CompoundDrawable.END))));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testSearchByName() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("logging"));
        helper.waitForCallback(searchBarChangeCount, 1);
        onView(allOf(withId(R.id.flag_name), withText(AwSwitches.WEBVIEW_VERBOSE_LOGGING)))
                .check(matches(isDisplayed()));
        onView(withId(R.id.flags_list)).check(matches(withCount(1)));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testSearchByDescription() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("highlight the contents"));
        helper.waitForCallback(searchBarChangeCount, 1);
        onView(allOf(withId(R.id.flag_name), withText(AwSwitches.HIGHLIGHT_ALL_WEBVIEWS)))
                .check(matches(isDisplayed()));
        onView(withId(R.id.flags_list)).check(matches(withCount(1)));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testCaseInsensitive() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("LOGGING"));
        helper.waitForCallback(searchBarChangeCount, 1);
        onView(allOf(withId(R.id.flag_name), withText(AwSwitches.WEBVIEW_VERBOSE_LOGGING)))
                .check(matches(isDisplayed()));
        onView(withId(R.id.flags_list)).check(matches(withCount(1)));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testMultipleResults() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        ListView flagsList = mRule.getActivity().findViewById(R.id.flags_list);
        int totalNumFlags = flagsList.getCount();

        // This assumes:
        //  * There will always be > 1 flag which mentions WebView explicitly (ex.
        //    HIGHLIGHT_ALL_WEBVIEWS and WEBVIEW_VERBOSE_LOGGING)
        //  * There will always be >= 1 flag which does not mention WebView in its description (ex.
        //    --show-composited-layer-borders).
        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("webview"));
        helper.waitForCallback(searchBarChangeCount, 1);
        onView(withId(R.id.flags_list))
                .check(matches(withCount(allOf(greaterThan(1), lessThan(totalNumFlags)))));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testClearingSearchShowsAllFlags() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        ListView flagsList = mRule.getActivity().findViewById(R.id.flags_list);
        int totalNumFlags = flagsList.getCount();

        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("logging"));
        helper.waitForCallback(searchBarChangeCount, 1);
        onView(withId(R.id.flags_list)).check(matches(withCount(1)));

        onView(withId(R.id.flag_search_bar)).perform(replaceText(""));
        helper.waitForCallback(searchBarChangeCount, 2);
        onView(withId(R.id.flags_list)).check(matches(withCount(totalNumFlags)));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testTappingClearButtonClearsText() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("logging"));
        helper.waitForCallback(searchBarChangeCount, 1);

        // "x" icon should visible if there's some text
        onView(withId(R.id.flag_search_bar))
                .check(matches(compoundDrawableVisible(CompoundDrawable.END)));

        EditText searchBar = mRule.getActivity().findViewById(R.id.flag_search_bar);
        tapCompoundDrawableOnUiThread(searchBar, CompoundDrawable.END);

        // "x" icon disappears when text is cleared
        onView(withId(R.id.flag_search_bar)).check(matches(withText("")));
        onView(withId(R.id.flag_search_bar))
                .check(matches(not(compoundDrawableVisible(CompoundDrawable.END))));

        // Magnifier icon should still be visible
        onView(withId(R.id.flag_search_bar))
                .check(matches(compoundDrawableVisible(CompoundDrawable.START)));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testDeletingTextHidesClearTextButton() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("logging"));
        helper.waitForCallback(searchBarChangeCount, 1);

        // "x" icon should visible if there's some text
        onView(withId(R.id.flag_search_bar))
                .check(matches(compoundDrawableVisible(CompoundDrawable.END)));

        onView(withId(R.id.flag_search_bar)).perform(replaceText(""));

        // "x" icon disappears when text is cleared
        onView(withId(R.id.flag_search_bar)).check(matches(withText("")));
        onView(withId(R.id.flag_search_bar))
                .check(matches(not(compoundDrawableVisible(CompoundDrawable.END))));

        // Magnifier icon should still be visible
        onView(withId(R.id.flag_search_bar))
                .check(matches(compoundDrawableVisible(CompoundDrawable.START)));
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testElsewhereOnSearchBarDoesNotClearText() throws Throwable {
        CallbackHelper helper = getFlagUiSearchBarListener();

        int searchBarChangeCount = helper.getCallCount();
        onView(withId(R.id.flag_search_bar)).perform(replaceText("logging"));
        helper.waitForCallback(searchBarChangeCount, 1);

        EditText searchBar = mRule.getActivity().findViewById(R.id.flag_search_bar);
        tapCompoundDrawableOnUiThread(searchBar, CompoundDrawable.TOP);

        // EditText should not be cleared
        onView(withId(R.id.flag_search_bar)).check(matches(withText("logging")));

        // "x" icon is still visible
        onView(withId(R.id.flag_search_bar))
                .check(matches(compoundDrawableVisible(CompoundDrawable.END)));
    }
}
