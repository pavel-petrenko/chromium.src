// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.display_cutout;

import android.annotation.TargetApi;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.support.test.InstrumentationRegistry;

import org.hamcrest.Matchers;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.webapps.WebDisplayMode;
import org.chromium.chrome.browser.webapps.WebappActivity;
import org.chromium.content_public.browser.test.util.Criteria;
import org.chromium.content_public.browser.test.util.CriteriaHelper;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Custom test rule for simulating a {@link WebappActivity} with a Display Cutout.
 */
@TargetApi(Build.VERSION_CODES.P)
public class WebappDisplayCutoutTestRule extends DisplayCutoutTestRule<WebappActivity> {
    /** Test data for the test webapp. */
    private static final String WEBAPP_ID = "webapp_id";
    private static final String WEBAPP_NAME = "webapp name";
    private static final String WEBAPP_SHORT_NAME = "webapp short name";

    /** The maximum waiting time to start {@link WebActivity} in ms. */
    private static final long STARTUP_TIMEOUT = 10000L;

    /**
     * Contains test specific configuration for launching {@link WebappActivity}.
     */
    @Target(ElementType.METHOD)
    @Retention(RetentionPolicy.RUNTIME)
    public @interface TestConfiguration {
        @WebDisplayMode
        int displayMode();
    }

    public WebappDisplayCutoutTestRule() {
        super(WebappActivity.class);
    }

    @Override
    public Statement apply(final Statement base, Description description) {
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                TestConfiguration config = description.getAnnotation(TestConfiguration.class);

                startWebappActivity(config.displayMode());
                setUp();

                base.evaluate();
                tearDown();
            }
        };
    }

    private void startWebappActivity(@WebDisplayMode int displayMode) {
        Intent intent =
                new Intent(InstrumentationRegistry.getTargetContext(), WebappActivity.class);
        intent.setData(Uri.parse(WebappActivity.WEBAPP_SCHEME + "://" + WEBAPP_ID));
        intent.putExtra(ShortcutHelper.EXTRA_ID, WEBAPP_ID);
        intent.putExtra(ShortcutHelper.EXTRA_URL, getTestURL());
        intent.putExtra(ShortcutHelper.EXTRA_NAME, WEBAPP_NAME);
        intent.putExtra(ShortcutHelper.EXTRA_SHORT_NAME, WEBAPP_SHORT_NAME);
        intent.putExtra(ShortcutHelper.EXTRA_DISPLAY_MODE, displayMode);

        launchActivity(intent);

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        CriteriaHelper.pollInstrumentationThread(() -> {
            Criteria.checkThat(getActivity().getActivityTab(), Matchers.notNullValue());
            Criteria.checkThat(getActivity().getActivityTab().isLoading(), Matchers.is(false));
        }, STARTUP_TIMEOUT, CriteriaHelper.DEFAULT_POLLING_INTERVAL);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        waitForActivityNativeInitializationComplete();
    }
}
