// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Build;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Base64;
import android.view.View;
import android.webkit.WebChromeClient;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwContents.VisualStateCallback;
import org.chromium.android_webview.AwWebResourceResponse;
import org.chromium.android_webview.test.util.CommonResources;
import org.chromium.android_webview.test.util.GraphicsTestUtils;
import org.chromium.android_webview.test.util.JavascriptEventObserver;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.test.util.CallbackHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.LoadUrlParams;

import java.io.ByteArrayInputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Visual state related tests.
 */
@MinAndroidSdkLevel(Build.VERSION_CODES.KITKAT)
public class VisualStateTest extends AwTestBase {
    private static final String WAIT_FOR_JS_TEST_URL =
            "file:///android_asset/visual_state_waits_for_js_test.html";
    private static final String ON_PAGE_COMMIT_VISIBLE_TEST_URL =
            "file:///android_asset/visual_state_on_page_commit_visible_test.html";
    private static final String FULLSCREEN_TEST_URL =
            "file:///android_asset/visual_state_during_fullscreen_test.html";
    private static final String UPDATE_COLOR_CONTROL_ID = "updateColorControl";
    private static final String ENTER_FULLSCREEN_CONTROL_ID = "enterFullscreenControl";

    private TestAwContentsClient mContentsClient = new TestAwContentsClient();

    private static class DelayedInputStream extends FilterInputStream {
        private CountDownLatch mLatch = new CountDownLatch(1);

        DelayedInputStream(InputStream in) {
            super(in);
        }

        @Override
        @SuppressWarnings("Finally")
        public int read() throws IOException {
            try {
                mLatch.await();
            } finally {
                return super.read();
            }
        }

        @Override
        @SuppressWarnings("Finally")
        public int read(byte[] buffer, int byteOffset, int byteCount) throws IOException {
            try {
                mLatch.await();
            } finally {
                return super.read(buffer, byteOffset, byteCount);
            }
        }

        public void allowReads() {
            mLatch.countDown();
        }
    }

    private static class SlowBlueImage extends AwWebResourceResponse {
        // This image delays returning data for 1 (scaled) second in order to simlate a slow network
        // connection.
        public static final long IMAGE_LOADING_DELAY_MS = scaleTimeout(1000);
        public SlowBlueImage() throws Throwable {
            super("image/png", "utf-8",
                    new DelayedInputStream(new ByteArrayInputStream(
                            Base64.decode(CommonResources.BLUE_PNG_BASE64, Base64.DEFAULT))));
        }

        @Override
        public InputStream getData() {
            final DelayedInputStream stream = (DelayedInputStream) super.getData();
            ThreadUtils.postOnUiThreadDelayed(new Runnable() {
                @Override
                public void run() {
                    stream.allowReads();
                }
            }, IMAGE_LOADING_DELAY_MS);
            return stream;
        }
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testVisualStateCallbackIsReceived() throws Throwable {
        AwTestContainerView testContainer = createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents awContents = testContainer.getAwContents();
        loadUrlSync(
                awContents, mContentsClient.getOnPageFinishedHelper(), CommonResources.ABOUT_HTML);
        final CallbackHelper ch = new CallbackHelper();
        final int chCount = ch.getCallCount();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                final long requestId = 0x123456789abcdef0L; // ensure requestId is not truncated.
                awContents.insertVisualStateCallback(requestId,
                        new AwContents.VisualStateCallback() {
                            @Override
                            public void onComplete(long id) {
                                assertEquals(requestId, id);
                                ch.notifyCalled();
                            }
                        });
            }
        });
        ch.waitForCallback(chCount);
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testVisualStateCallbackWaitsForContentsToBeOnScreen() throws Throwable {
        // This test loads a page with a blue background color. It then waits for the DOM tree
        // in blink to contain the contents of the blue page (which happens when the onPageFinished
        // event is received). It then flushes the contents and verifies that the blue page
        // background color is drawn when the flush callback is received.
        final LoadUrlParams bluePageUrl = createTestPageUrl("blue");
        final CountDownLatch testFinishedSignal = new CountDownLatch(1);

        final AtomicReference<AwContents> awContentsRef = new AtomicReference<>();
        final AwTestContainerView testView =
                createAwTestContainerViewOnMainSync(new TestAwContentsClient() {
                    @Override
                    public void onPageFinished(String url) {
                        if (bluePageUrl.getUrl().equals(url)) {
                            final long requestId = 10;
                            awContentsRef.get().insertVisualStateCallback(requestId,
                                    new VisualStateCallback() {
                                        @Override
                                        public void onComplete(long id) {
                                            assertEquals(requestId, id);
                                            Bitmap blueScreenshot = GraphicsTestUtils
                                                    .drawAwContents(
                                                            awContentsRef.get(), 1, 1);
                                            assertEquals(Color.BLUE, blueScreenshot.getPixel(0, 0));
                                            testFinishedSignal.countDown();
                                        }
                                    });
                        }
                    }
                });
        final AwContents awContents = testView.getAwContents();
        awContentsRef.set(awContents);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                awContents.setBackgroundColor(Color.RED);
                awContents.loadUrl(bluePageUrl);

                // We have just loaded the blue page, but the graphics pipeline is asynchronous
                // so at this point the WebView still draws red, ie. the View background color.
                // Only when the flush callback is received will we know for certain that the
                // blue page contents are on screen.
                Bitmap redScreenshot = GraphicsTestUtils.drawAwContents(
                        awContentsRef.get(), 1, 1);
                assertEquals(Color.RED, redScreenshot.getPixel(0, 0));
            }
        });

        assertTrue(testFinishedSignal.await(AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testOnPageCommitVisible() throws Throwable {
        // This test loads a page with a blue background color. It then waits for the DOM tree
        // in blink to contain the contents of the blue page (which happens when the onPageFinished
        // event is received). It then flushes the contents and verifies that the blue page
        // background color is drawn when the flush callback is received.
        final CountDownLatch testFinishedSignal = new CountDownLatch(1);
        final CountDownLatch pageCommitCallbackOccurred = new CountDownLatch(1);

        final AtomicReference<AwContents> awContentsRef = new AtomicReference<>();
        final AwTestContainerView testView =
                createAwTestContainerViewOnMainSync(new TestAwContentsClient() {
                    @Override
                    public void onPageCommitVisible(String url) {
                        Bitmap bitmap =
                                GraphicsTestUtils.drawAwContents(awContentsRef.get(), 256, 256);
                        assertEquals(Color.GREEN, bitmap.getPixel(128, 128));
                        pageCommitCallbackOccurred.countDown();
                    }

                    @Override
                    public AwWebResourceResponse shouldInterceptRequest(
                            AwWebResourceRequest request) {
                        if (request.url.equals("intercepted://blue.png")) {
                            try {
                                return new SlowBlueImage();
                            } catch (Throwable t) {
                                return null;
                            }
                        }
                        return null;
                    }

                    @Override
                    public void onPageFinished(String url) {
                        super.onPageFinished(url);
                        awContentsRef.get().insertVisualStateCallback(
                                10, new VisualStateCallback() {
                                    @Override
                                    public void onComplete(long id) {
                                        Bitmap bitmap = GraphicsTestUtils.drawAwContents(
                                                awContentsRef.get(), 256, 256);
                                        assertEquals(Color.BLUE, bitmap.getPixel(128, 128));
                                        testFinishedSignal.countDown();
                                    }
                                });
                    }
                });

        final AwContents awContents = testView.getAwContents();
        awContentsRef.set(awContents);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                awContents.setBackgroundColor(Color.RED);

                awContents.loadUrl(new LoadUrlParams(ON_PAGE_COMMIT_VISIBLE_TEST_URL));

                // We have just loaded the blue page, but the graphics pipeline is asynchronous
                // so at this point the WebView still draws red, ie. the View background color.
                // Only when the flush callback is received will we know for certain that the
                // blue page contents are on screen.
                Bitmap bitmap = GraphicsTestUtils.drawAwContents(
                        awContentsRef.get(), 256, 256);
                assertEquals(Color.RED, bitmap.getPixel(128, 128));
            }
        });

        assertTrue(pageCommitCallbackOccurred.await(
                AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
        assertTrue(testFinishedSignal.await(AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testVisualStateCallbackWaitsForJs() throws Throwable {
        // This test checks that when a VisualStateCallback completes the results of executing
        // any block of JS prior to the time at which the callback was inserted will be visible
        // in the next draw. For that it loads a page that changes the background color of
        // the page from JS when a button is clicked.
        final CountDownLatch readyToUpdateColor = new CountDownLatch(1);
        final CountDownLatch testFinishedSignal = new CountDownLatch(1);

        final AtomicReference<AwContents> awContentsRef = new AtomicReference<>();
        TestAwContentsClient awContentsClient = new TestAwContentsClient() {
            @Override
            public void onPageFinished(String url) {
                super.onPageFinished(url);
                awContentsRef.get().insertVisualStateCallback(10,
                        new VisualStateCallback() {
                            @Override
                            public void onComplete(long id) {
                                Bitmap blueScreenshot = GraphicsTestUtils.drawAwContents(
                                        awContentsRef.get(), 100, 100);
                                assertEquals(Color.BLUE, blueScreenshot.getPixel(50, 50));
                                readyToUpdateColor.countDown();
                            }
                        });
            }
        };
        final AwTestContainerView testView =
                createAwTestContainerViewOnMainSync(awContentsClient);
        final AwContents awContents = testView.getAwContents();
        awContentsRef.set(awContents);
        final ContentViewCore contentViewCore = testView.getContentViewCore();
        enableJavaScriptOnUiThread(awContents);

        // JS will notify this observer once it has changed the background color of the page.
        final JavascriptEventObserver jsObserver = new JavascriptEventObserver();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                jsObserver.register(contentViewCore, "jsObserver");
            }
        });

        loadUrlSync(awContents,
                awContentsClient.getOnPageFinishedHelper(), WAIT_FOR_JS_TEST_URL);

        assertTrue(readyToUpdateColor.await(AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
        DOMUtils.clickNode(
                VisualStateTest.this, contentViewCore, UPDATE_COLOR_CONTROL_ID);
        assertTrue(jsObserver.waitForEvent(WAIT_TIMEOUT_MS));

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                awContents.insertVisualStateCallback(20,
                        new VisualStateCallback() {
                            @Override
                            public void onComplete(long id) {
                                Bitmap redScreenshot = GraphicsTestUtils.drawAwContents(
                                        awContents, 100, 100);
                                assertEquals(Color.RED, redScreenshot.getPixel(50, 50));
                                testFinishedSignal.countDown();
                            }
                        });

            }
        });

        assertTrue(testFinishedSignal.await(AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testVisualStateCallbackFromJsDuringFullscreenTransitions() throws Throwable {
        // This test checks that VisualStateCallbacks are delivered correctly during
        // fullscreen transitions. It loads a page, clicks a button to enter fullscreen,
        // then inserts a VisualStateCallback once notified from JS and verifies that when the
        // callback is received the fullscreen contents are rendered correctly in the next draw.
        final CountDownLatch readyToEnterFullscreenSignal = new CountDownLatch(1);
        final CountDownLatch testFinishedSignal = new CountDownLatch(1);

        final AtomicReference<AwContents> awContentsRef = new AtomicReference<>();
        final FullScreenVideoTestAwContentsClient awContentsClient =
                new FullScreenVideoTestAwContentsClient(
                        getActivity(), isHardwareAcceleratedTest()) {
            @Override
            public void onPageFinished(String url) {
                super.onPageFinished(url);
                awContentsRef.get().insertVisualStateCallback(10, new VisualStateCallback() {
                    @Override
                    public void onComplete(long id) {
                        Bitmap blueScreenshot =
                                GraphicsTestUtils.drawAwContents(awContentsRef.get(), 100, 100);
                        assertEquals(Color.BLUE, blueScreenshot.getPixel(50, 50));
                        readyToEnterFullscreenSignal.countDown();
                    }
                });
            }
        };
        final AwTestContainerView testView = createAwTestContainerViewOnMainSync(awContentsClient);
        final AwContents awContents = testView.getAwContents();
        awContentsRef.set(awContents);
        final ContentViewCore contentViewCore = testView.getContentViewCore();
        enableJavaScriptOnUiThread(awContents);
        awContents.getSettings().setFullscreenSupported(true);

        // JS will notify this observer once it has entered fullscreen.
        final JavascriptEventObserver jsObserver = new JavascriptEventObserver();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                jsObserver.register(contentViewCore, "jsObserver");
            }
        });

        loadUrlSync(awContents, awContentsClient.getOnPageFinishedHelper(), FULLSCREEN_TEST_URL);

        assertTrue(readyToEnterFullscreenSignal.await(
                AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
        DOMUtils.clickNode(VisualStateTest.this, contentViewCore, ENTER_FULLSCREEN_CONTROL_ID);
        assertTrue(jsObserver.waitForEvent(WAIT_TIMEOUT_MS));

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                awContents.insertVisualStateCallback(20, new VisualStateCallback() {
                    @Override
                    public void onComplete(long id) {
                        // NOTE: We cannot use drawAwContents here because the web contents
                        // are rendered into the custom view while in fullscreen.
                        Bitmap redScreenshot = GraphicsTestUtils.drawView(
                                awContentsClient.getCustomView(), 100, 100);
                        assertEquals(Color.RED, redScreenshot.getPixel(50, 50));
                        testFinishedSignal.countDown();
                    }
                });
            }
        });

        assertTrue(testFinishedSignal.await(AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testVisualStateCallbackWhenContainerViewDetached()
            throws Throwable {
        final CountDownLatch readyToEnterFullscreenSignal = new CountDownLatch(1);
        final CountDownLatch hasCustomViewSignal = new CountDownLatch(1);
        final CountDownLatch testFinishedSignal = new CountDownLatch(1);

        final AtomicReference<AwContents> awContentsRef = new AtomicReference<>();
        final AtomicReference<View> customViewRef = new AtomicReference<>();

        final TestAwContentsClient awContentsClient = new TestAwContentsClient() {
            @Override
            public void onPageFinished(String url) {
                super.onPageFinished(url);
                readyToEnterFullscreenSignal.countDown();
            }

            @Override
            public void onShowCustomView(
                    final View customView, WebChromeClient.CustomViewCallback callback) {
                // Please note that we don't attach the custom view to the window here
                // (awContentsClient is an instance of TestAwContentsClient, not
                // FullScreenVideoTestAwContentsClient).
                customView.setClipBounds(new Rect(0, 0, 100, 100));
                customView.measure(100, 100);
                customView.layout(0, 0, 100, 100);
                customViewRef.set(customView);
                hasCustomViewSignal.countDown();
            }
        };
        final AwTestContainerView testView = createAwTestContainerViewOnMainSync(awContentsClient);
        final AwContents awContents = testView.getAwContents();
        awContentsRef.set(awContents);
        final ContentViewCore contentViewCore = testView.getContentViewCore();
        enableJavaScriptOnUiThread(awContents);
        awContents.getSettings().setFullscreenSupported(true);

        // JS will notify this observer once it has entered fullscreen.
        final JavascriptEventObserver jsObserver = new JavascriptEventObserver();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                jsObserver.register(contentViewCore, "jsObserver");
            }
        });

        loadUrlSync(awContents, awContentsClient.getOnPageFinishedHelper(), FULLSCREEN_TEST_URL);

        assertTrue(readyToEnterFullscreenSignal.await(
                AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
        DOMUtils.clickNode(VisualStateTest.this, contentViewCore, ENTER_FULLSCREEN_CONTROL_ID);

        assertTrue(hasCustomViewSignal.await(AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
        assertTrue(jsObserver.waitForEvent(WAIT_TIMEOUT_MS));

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                awContents.insertVisualStateCallback(20, new VisualStateCallback() {
                    @Override
                    public void onComplete(long id) {
                        assertFalse(customViewRef.get().isAttachedToWindow());
                        // NOTE: We cannot use drawAwContents here because the web contents
                        // are rendered into the custom view while in fullscreen.
                        Bitmap redScreenshot = GraphicsTestUtils.drawView(
                                customViewRef.get(), 100, 100);
                        assertEquals(Color.RED, redScreenshot.getPixel(50, 50));
                        testFinishedSignal.countDown();
                    }
                });
            }
        });

        assertTrue(testFinishedSignal.await(AwTestBase.WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS));
    }

    private static final LoadUrlParams createTestPageUrl(String backgroundColor) {
        return LoadUrlParams.createLoadDataParams(
                "<html><body bgcolor=" + backgroundColor + "></body></html>", "text/html", false);
    }
}
