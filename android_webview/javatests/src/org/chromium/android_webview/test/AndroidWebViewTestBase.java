// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.app.Instrumentation;
import android.content.Context;
import android.test.ActivityInstrumentationTestCase2;
import android.view.View;

import org.chromium.android_webview.AndroidWebViewUtil;
import org.chromium.android_webview.AwContents;
import org.chromium.content.browser.ContentSettings;
import org.chromium.content.browser.ContentView;
import org.chromium.content.browser.ContentViewClient;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.LoadUrlParams;
import org.chromium.content.browser.test.CallbackHelper;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * A base class for android_webview tests.
 */
public class AndroidWebViewTestBase
        extends ActivityInstrumentationTestCase2<AndroidWebViewTestRunnerActivity> {
    protected static int WAIT_TIMEOUT_SECONDS = 15;

    public AndroidWebViewTestBase() {
        super(AndroidWebViewTestRunnerActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        final Context context = getActivity();
        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                ContentViewCore.initChromiumBrowserProcess(
                        context, ContentView.MAX_RENDERERS_SINGLE_PROCESS);
            }
        });
    }

    /**
     * Runs a {@link Callable} on the main thread, blocking until it is
     * complete, and returns the result. Calls
     * {@link Instrumentation#waitForIdleSync()} first to help avoid certain
     * race conditions.
     *
     * @param <R> Type of result to return
     */
    public <R> R runTestOnUiThreadAndGetResult(Callable<R> callable)
            throws Throwable {
        FutureTask<R> task = new FutureTask<R>(callable);
        getInstrumentation().waitForIdleSync();
        getInstrumentation().runOnMainSync(task);
        try {
            return task.get();
        } catch (ExecutionException e) {
            // Unwrap the cause of the exception and re-throw it.
            throw e.getCause();
        }
    }

    /**
     * Loads url on the UI thread and blocks until onPageFinished is called.
     */
    protected void loadUrlSync(final ContentViewCore contentViewCore,
                               CallbackHelper onPageFinishedHelper,
                               final String url) throws Throwable {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        loadUrlAsync(contentViewCore, url);
        onPageFinishedHelper.waitForCallback(currentCallCount, 1, WAIT_TIMEOUT_SECONDS,
                TimeUnit.SECONDS);
    }

    /**
     * Loads url on the UI thread but does not block.
     */
    protected void loadUrlAsync(final ContentViewCore contentViewCore,
                                final String url) throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                contentViewCore.loadUrl(new LoadUrlParams(url));
            }
        });
    }

    /**
     * Loads data on the UI thread and blocks until onPageFinished is called.
     */
    protected void loadDataSync(final ContentViewCore contentViewCore,
                                CallbackHelper onPageFinishedHelper,
                                final String data, final String mimeType,
                                final boolean isBase64Encoded) throws Throwable {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        loadDataAsync(contentViewCore, data, mimeType, isBase64Encoded);
        onPageFinishedHelper.waitForCallback(currentCallCount, 1, WAIT_TIMEOUT_SECONDS,
                TimeUnit.SECONDS);
    }

    /**
     * Loads data on the UI thread but does not block.
     */
    protected void loadDataAsync(final ContentViewCore contentViewCore, final String data,
                                 final String mimeType, final boolean isBase64Encoded)
            throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                contentViewCore.loadUrl(LoadUrlParams.createLoadDataParams(
                        data, mimeType, isBase64Encoded));
            }
        });
    }

    protected ContentView createContentView(boolean incognito,
                                            ContentViewClient contentViewClient) {
        int nativeWebContents = AndroidWebViewUtil.createNativeWebContents(incognito);
        ContentView contentView = ContentView.newInstance(getActivity(), nativeWebContents,
                                                          ContentView.PERSONALITY_VIEW);
        contentView.setContentViewClient(contentViewClient);
        return contentView;
    }
}
