// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.robolectric;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.view.DisplayCutout;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;

import org.chromium.android_webview.AwDisplayCutoutController;
import org.chromium.android_webview.AwDisplayCutoutController.Insets;
import org.chromium.base.Log;
import org.chromium.base.test.util.Feature;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * JUnit tests for AwDisplayCutoutController.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class AwDisplayCutoutControllerTest {
    private static final String TAG = "DisplayCutoutTest";
    private static final boolean DEBUG = false;

    private InOrder mInOrder;
    private Context mContext;

    @Mock
    private AwDisplayCutoutController.Delegate mDelegate;
    @Mock
    private WindowInsets mWindowInsets;
    @Mock
    private DisplayCutout mDisplayCutout;
    @Mock
    private View mView;
    @Mock
    private View mAnotherView;

    @Mock
    private ViewGroup mParentView;
    @Mock
    private ViewGroup mRootView;

    private View.OnApplyWindowInsetsListener mListener;
    private int[] mLocationOnScreen = {0, 0};
    private int mViewWidth;
    private int mViewHeight;

    private Matrix mGlobalTransformMatrix;

    private float mDipScale;
    private int mDisplayWidth;
    private int mDisplayHeight;

    private AwDisplayCutoutController mController;

    public AwDisplayCutoutControllerTest() {
        if (DEBUG) ShadowLog.stream = System.out; // allows logging
    }

    @Before
    public void setUp() {
        if (DEBUG) Log.i(TAG, "setUp");
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;

        // Set up default values.
        setWindowInsets(new Rect(20, 40, 60, 80));
        mDipScale = 2.0f;
        mViewWidth = 300;
        mViewHeight = 400;
        mDisplayWidth = 300;
        mDisplayHeight = 400;
        mGlobalTransformMatrix = new Matrix(); // identity matrix

        // Set up the view.
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                int[] loc = (int[]) (invocation.getArguments()[0]);
                loc[0] = mLocationOnScreen[0];
                loc[1] = mLocationOnScreen[1];
                return null;
            }
        })
                .when(mView)
                .getLocationOnScreen(any(int[].class));
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                mListener = (View.OnApplyWindowInsetsListener) (invocation.getArguments()[0]);
                return null;
            }
        })
                .when(mView)
                .setOnApplyWindowInsetsListener(any(View.OnApplyWindowInsetsListener.class));
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                mListener.onApplyWindowInsets(mView, mWindowInsets);
                return null;
            }
        })
                .when(mView)
                .requestApplyInsets();

        when(mView.getMeasuredWidth()).thenReturn(mViewWidth);
        when(mView.getMeasuredHeight()).thenReturn(mViewHeight);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Matrix matrix = (Matrix) (invocation.getArguments()[0]);
                matrix.set(mGlobalTransformMatrix);
                return null;
            }
        })
                .when(mView)
                .transformMatrixToGlobal(any(Matrix.class));

        // Set up the root view.
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                int[] loc = (int[]) (invocation.getArguments()[0]);
                loc[0] = mLocationOnScreen[0];
                loc[1] = mLocationOnScreen[1];
                return null;
            }
        })
                .when(mRootView)
                .getLocationOnScreen(any(int[].class));
        when(mRootView.getMeasuredWidth()).thenReturn(mViewWidth);
        when(mRootView.getMeasuredHeight()).thenReturn(mViewHeight);
        when(mView.getRootView()).thenReturn(mRootView);

        // Set up the delegate.
        when(mDelegate.getDipScale()).thenReturn(mDipScale);
        when(mDelegate.getDisplayWidth()).thenReturn(mDisplayWidth);
        when(mDelegate.getDisplayHeight()).thenReturn(mDisplayHeight);

        mInOrder = inOrder(mDelegate, mView, mAnotherView);

        mController = new AwDisplayCutoutController(mDelegate, mView);

        mInOrder.verify(mView).setOnApplyWindowInsetsListener(
                any(View.OnApplyWindowInsetsListener.class));

        mInOrder.verifyNoMoreInteractions();
    }

    private void setWindowInsets(Rect insets) {
        when(mDisplayCutout.getSafeInsetLeft()).thenReturn(insets.left);
        when(mDisplayCutout.getSafeInsetTop()).thenReturn(insets.top);
        when(mDisplayCutout.getSafeInsetRight()).thenReturn(insets.right);
        when(mDisplayCutout.getSafeInsetBottom()).thenReturn(insets.bottom);
        // Note that prior to Android Q, there is no way to build WindowInsets.
        when(mWindowInsets.getDisplayCutout()).thenReturn(mDisplayCutout);
    }

    @After
    public void tearDown() {
        if (DEBUG) Log.i(TAG, "tearDown");
        mInOrder.verifyNoMoreInteractions();
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOnApplyWindowInsets() {
        mController.onApplyWindowInsets(mWindowInsets);

        mInOrder.verify(mView).getLocationOnScreen(any(int[].class));
        mInOrder.verify(mView).getMeasuredWidth();
        mInOrder.verify(mView).getMeasuredHeight();
        mInOrder.verify(mDelegate).getDisplayWidth();
        mInOrder.verify(mDelegate).getDisplayHeight();
        mInOrder.verify(mDelegate).getDipScale();

        // Note that DIP of 2.0 is applied, so the values are halved.
        mInOrder.verify(mDelegate).setDisplayCutoutSafeArea(eq(new Insets(10, 20, 30, 40)));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOnApplyWindowInsets_NotOccupyingFullDisplay() {
        // View is not occupying the entire display, so no insets applied.
        when(mView.getMeasuredHeight()).thenReturn(mDisplayHeight / 2);

        mController.onApplyWindowInsets(mWindowInsets);

        mInOrder.verify(mView).getLocationOnScreen(any(int[].class));
        mInOrder.verify(mView).getMeasuredWidth();
        mInOrder.verify(mView).getMeasuredHeight();
        mInOrder.verify(mDelegate).getDisplayWidth();
        mInOrder.verify(mDelegate).getDisplayHeight();

        mInOrder.verify(mDelegate).setDisplayCutoutSafeArea(eq(new Insets(0, 0, 0, 0)));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOnApplyWindowInsets_NotOccupyingFullWindow() {
        // View is not occupying the entire window, so no insets applied.
        when(mRootView.getMeasuredHeight()).thenReturn(mViewHeight / 2);

        mController.onApplyWindowInsets(mWindowInsets);

        mInOrder.verify(mView).getLocationOnScreen(any(int[].class));
        mInOrder.verify(mView).getMeasuredWidth();
        mInOrder.verify(mView).getMeasuredHeight();
        mInOrder.verify(mDelegate).getDisplayWidth();
        mInOrder.verify(mDelegate).getDisplayHeight();

        mInOrder.verify(mDelegate).setDisplayCutoutSafeArea(eq(new Insets(0, 0, 0, 0)));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOnApplyWindowInsets_ParentLayoutRotated() {
        mGlobalTransformMatrix.postRotate(30.0f);

        mController.onApplyWindowInsets(mWindowInsets);

        mInOrder.verify(mView).getLocationOnScreen(any(int[].class));
        mInOrder.verify(mView).getMeasuredWidth();
        mInOrder.verify(mView).getMeasuredHeight();
        mInOrder.verify(mDelegate).getDisplayWidth();
        mInOrder.verify(mDelegate).getDisplayHeight();

        mInOrder.verify(mDelegate).setDisplayCutoutSafeArea(eq(new Insets(0, 0, 0, 0)));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOnSizeChanged() {
        mController.onSizeChanged();

        mInOrder.verify(mView).requestApplyInsets();

        mInOrder.verify(mView).getLocationOnScreen(any(int[].class));
        mInOrder.verify(mView).getMeasuredWidth();
        mInOrder.verify(mView).getMeasuredHeight();
        mInOrder.verify(mDelegate).getDisplayWidth();
        mInOrder.verify(mDelegate).getDisplayHeight();
        mInOrder.verify(mDelegate).getDipScale();

        // Note that DIP of 2.0 is applied, so the values are halved.
        mInOrder.verify(mDelegate).setDisplayCutoutSafeArea(eq(new Insets(10, 20, 30, 40)));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testOnAttachedToWindow() {
        mController.onAttachedToWindow();

        mInOrder.verify(mView).requestApplyInsets();

        mInOrder.verify(mView).getLocationOnScreen(any(int[].class));
        mInOrder.verify(mView).getMeasuredWidth();
        mInOrder.verify(mView).getMeasuredHeight();
        mInOrder.verify(mDelegate).getDisplayWidth();
        mInOrder.verify(mDelegate).getDisplayHeight();
        mInOrder.verify(mDelegate).getDipScale();

        // Note that DIP of 2.0 is applied, so the values are halved.
        mInOrder.verify(mDelegate).setDisplayCutoutSafeArea(eq(new Insets(10, 20, 30, 40)));
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testChangeContainerView_doesNotTriggerOriginalView() {
        mController.registerContainerView(mAnotherView);

        mInOrder.verify(mAnotherView)
                .setOnApplyWindowInsetsListener(any(View.OnApplyWindowInsetsListener.class));

        // Switching to another container view.
        mController.setCurrentContainerView(mAnotherView);

        mInOrder.verify(mAnotherView).requestApplyInsets();

        mController.onAttachedToWindow();

        // Note that mView methods are not triggered.
        mInOrder.verify(mAnotherView).requestApplyInsets();
    }
}