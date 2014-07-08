// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.framehost;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.content_public.browser.NavigationController;

/**
 * The NavigationControllerImpl Java wrapper to allow communicating with the native
 * NavigationControllerImpl object.
 */
@JNINamespace("content")
// TODO(tedchoc): Remove the package restriction once this class moves to a non-public content
//                package whose visibility will be enforced via DEPS.
/* package */ class NavigationControllerImpl implements NavigationController {

    private long mNativeNavigationControllerAndroid;

    private NavigationControllerImpl(long nativeNavigationControllerAndroid) {
        mNativeNavigationControllerAndroid = nativeNavigationControllerAndroid;
    }

    @CalledByNative
    private static NavigationControllerImpl create(long nativeNavigationControllerAndroid) {
        return new NavigationControllerImpl(nativeNavigationControllerAndroid);
    }

    @CalledByNative
    private void destroy() {
        mNativeNavigationControllerAndroid = 0;
    }

    @Override
    public boolean canGoBack() {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanGoBack(mNativeNavigationControllerAndroid);
    }

    @Override
    public boolean canGoForward() {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanGoForward(mNativeNavigationControllerAndroid);
    }

    @Override
    public boolean canGoToOffset(int offset) {
        return mNativeNavigationControllerAndroid != 0
                && nativeCanGoToOffset(mNativeNavigationControllerAndroid, offset);
    }

    @Override
    public void goToOffset(int offset) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoToOffset(mNativeNavigationControllerAndroid, offset);
        }
    }

    @Override
    public void goToNavigationIndex(int index) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoToNavigationIndex(mNativeNavigationControllerAndroid, index);
        }
    }

    @Override
    public void goBack() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoBack(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void goForward() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeGoForward(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void loadIfNecessary() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeLoadIfNecessary(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void requestRestoreLoad() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeRequestRestoreLoad(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void reload(boolean checkForRepost) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeReload(mNativeNavigationControllerAndroid, checkForRepost);
        }
    }

    @Override
    public void reloadIgnoringCache(boolean checkForRepost) {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeReloadIgnoringCache(mNativeNavigationControllerAndroid, checkForRepost);
        }
    }

    @Override
    public void cancelPendingReload() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeCancelPendingReload(mNativeNavigationControllerAndroid);
        }
    }

    @Override
    public void continuePendingReload() {
        if (mNativeNavigationControllerAndroid != 0) {
            nativeContinuePendingReload(mNativeNavigationControllerAndroid);
        }
    }

    private native boolean nativeCanGoBack(long nativeNavigationControllerAndroid);
    private native boolean nativeCanGoForward(long nativeNavigationControllerAndroid);
    private native void nativeLoadIfNecessary(long nativeNavigationControllerAndroid);
    private native void nativeRequestRestoreLoad(long nativeNavigationControllerAndroid);
    private native boolean nativeCanGoToOffset(
            long nativeNavigationControllerAndroid, int offset);
    private native void nativeGoBack(long nativeNavigationControllerAndroid);
    private native void nativeGoForward(long nativeNavigationControllerAndroid);
    private native void nativeGoToOffset(long nativeNavigationControllerAndroid, int offset);
    private native void nativeGoToNavigationIndex(
            long nativeNavigationControllerAndroid, int index);
    private native void nativeCancelPendingReload(long nativeNavigationControllerAndroid);
    private native void nativeContinuePendingReload(long nativeNavigationControllerAndroid);
    private native void nativeReload(long nativeNavigationControllerAndroid,
            boolean checkForRepost);
    private native void nativeReloadIgnoringCache(long nativeNavigationControllerAndroid,
            boolean checkForRepost);
}
