// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.webkit.ConsoleMessage;
import android.webkit.ValueCallback;

import org.chromium.base.ContentUriUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.content_public.browser.InvalidateTypes;

/**
 * Adapts the AwWebContentsDelegate interface to the AwContentsClient interface.
 * This class also serves a secondary function of routing certain callbacks from the content layer
 * to specific listener interfaces.
 */
class AwWebContentsDelegateAdapter extends AwWebContentsDelegate {
    private static final String TAG = "AwWebContentsDelegateAdapter";

    private final AwContents mAwContents;
    private final AwContentsClient mContentsClient;
    private final AwContentViewClient mContentViewClient;
    private final Context mContext;
    private View mContainerView;

    public AwWebContentsDelegateAdapter(AwContents awContents, AwContentsClient contentsClient,
            AwContentViewClient contentViewClient, Context context, View containerView) {
        mAwContents = awContents;
        mContentsClient = contentsClient;
        mContentViewClient = contentViewClient;
        mContext = context;
        setContainerView(containerView);
    }

    public void setContainerView(View containerView) {
        mContainerView = containerView;
    }

    @Override
    public void onLoadProgressChanged(int progress) {
        mContentsClient.onProgressChanged(progress);
    }

    @Override
    public void handleKeyboardEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            int direction;
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    direction = View.FOCUS_DOWN;
                    break;
                case KeyEvent.KEYCODE_DPAD_UP:
                    direction = View.FOCUS_UP;
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    direction = View.FOCUS_LEFT;
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    direction = View.FOCUS_RIGHT;
                    break;
                default:
                    direction = 0;
                    break;
            }
            if (direction != 0 && tryToMoveFocus(direction)) return;
        }
        mContentsClient.onUnhandledKeyEvent(event);
    }

    @SuppressLint("NewApi")  // View#getLayoutDirection requires API level 17.
    @Override
    public boolean takeFocus(boolean reverse) {
        int direction =
                (reverse == (mContainerView.getLayoutDirection() == View.LAYOUT_DIRECTION_RTL))
                ? View.FOCUS_RIGHT : View.FOCUS_LEFT;
        if (tryToMoveFocus(direction)) return true;
        direction = reverse ? View.FOCUS_UP : View.FOCUS_DOWN;
        return tryToMoveFocus(direction);
    }

    private boolean tryToMoveFocus(int direction) {
        View focus = mContainerView.focusSearch(direction);
        return focus != null && focus != mContainerView && focus.requestFocus();
    }

    @Override
    public boolean addMessageToConsole(int level, String message, int lineNumber,
            String sourceId) {
        ConsoleMessage.MessageLevel messageLevel = ConsoleMessage.MessageLevel.DEBUG;
        switch(level) {
            case LOG_LEVEL_TIP:
                messageLevel = ConsoleMessage.MessageLevel.TIP;
                break;
            case LOG_LEVEL_LOG:
                messageLevel = ConsoleMessage.MessageLevel.LOG;
                break;
            case LOG_LEVEL_WARNING:
                messageLevel = ConsoleMessage.MessageLevel.WARNING;
                break;
            case LOG_LEVEL_ERROR:
                messageLevel = ConsoleMessage.MessageLevel.ERROR;
                break;
            default:
                Log.w(TAG, "Unknown message level, defaulting to DEBUG");
                break;
        }

        boolean result = mContentsClient.onConsoleMessage(
                new ConsoleMessage(message, sourceId, lineNumber, messageLevel));
        if (result && message != null && message.startsWith("[blocked]")) {
            Log.e(TAG, "Blocked URL: " + message);
        }
        return result;
    }

    @Override
    public void onUpdateUrl(String url) {
        // TODO: implement
    }

    @Override
    public void openNewTab(String url, String extraHeaders, byte[] postData, int disposition,
            boolean isRendererInitiated) {
        // This is only called in chrome layers.
        assert false;
    }

    @Override
    public void closeContents() {
        mContentsClient.onCloseWindow();
    }

    @Override
    public void showRepostFormWarningDialog() {
        // TODO(mkosiba) We should be using something akin to the JsResultReceiver as the
        // callback parameter (instead of ContentViewCore) and implement a way of converting
        // that to a pair of messages.
        final int msgContinuePendingReload = 1;
        final int msgCancelPendingReload = 2;

        // TODO(sgurun) Remember the URL to cancel the reload behavior
        // if it is different than the most recent NavigationController entry.
        final Handler handler = new Handler(ThreadUtils.getUiThreadLooper()) {
            @Override
            public void handleMessage(Message msg) {
                if (mAwContents.getNavigationController() == null) return;

                switch(msg.what) {
                    case msgContinuePendingReload: {
                        mAwContents.getNavigationController().continuePendingReload();
                        break;
                    }
                    case msgCancelPendingReload: {
                        mAwContents.getNavigationController().cancelPendingReload();
                        break;
                    }
                    default:
                        throw new IllegalStateException(
                                "WebContentsDelegateAdapter: unhandled message " + msg.what);
                }
            }
        };

        Message resend = handler.obtainMessage(msgContinuePendingReload);
        Message dontResend = handler.obtainMessage(msgCancelPendingReload);
        mContentsClient.onFormResubmission(dontResend, resend);
    }

    @Override
    public void runFileChooser(final int processId, final int renderId, final int modeFlags,
            String acceptTypes, String title, String defaultFilename, boolean capture) {
        AwContentsClient.FileChooserParamsImpl params = new AwContentsClient.FileChooserParamsImpl(
                modeFlags, acceptTypes, title, defaultFilename, capture);

        mContentsClient.showFileChooser(new ValueCallback<String[]>() {
            boolean mCompleted = false;
            @Override
            public void onReceiveValue(String[] results) {
                if (mCompleted) {
                    throw new IllegalStateException("Duplicate showFileChooser result");
                }
                mCompleted = true;
                if (results == null) {
                    nativeFilesSelectedInChooser(
                            processId, renderId, modeFlags, null, null);
                    return;
                }
                GetDisplayNameTask task = new GetDisplayNameTask(
                        mContext.getContentResolver(), processId, renderId, modeFlags, results);
                task.execute();
            }
        }, params);
    }

    @Override
    public boolean addNewContents(boolean isDialog, boolean isUserGesture) {
        return mContentsClient.onCreateWindow(isDialog, isUserGesture);
    }

    @Override
    public void activateContents() {
        mContentsClient.onRequestFocus();
    }

    @Override
    public void navigationStateChanged(int flags) {
        if ((flags & InvalidateTypes.URL) != 0
                && mAwContents.isPopupWindow()
                && mAwContents.hasAccessedInitialDocument()) {
            // Hint the client to show the last committed url, as it may be unsafe to show
            // the pending entry.
            String url = mAwContents.getLastCommittedUrl();
            url = TextUtils.isEmpty(url) ? "about:blank" : url;
            mContentsClient.onPageStarted(url);
            mContentsClient.onLoadResource(url);
            mContentsClient.onProgressChanged(100);
            mContentsClient.onPageFinished(url);
        }
    }

    @Override
    public void toggleFullscreenModeForTab(boolean enterFullscreen) {
        if (enterFullscreen) {
            mContentViewClient.enterFullscreen();
        } else {
            mContentViewClient.exitFullscreen();
        }
    }

    private static class GetDisplayNameTask extends AsyncTask<Void, Void, String[]> {
        final int mProcessId;
        final int mRenderId;
        final int mModeFlags;
        final String[] mFilePaths;
        final ContentResolver mContentResolver;

        public GetDisplayNameTask(ContentResolver contentResolver, int processId, int renderId,
                                  int modeFlags, String[] filePaths) {
            mProcessId = processId;
            mRenderId = renderId;
            mModeFlags = modeFlags;
            mFilePaths = filePaths;
            mContentResolver = contentResolver;
        }

        @Override
        protected String[] doInBackground(Void...voids) {
            String[] displayNames = new String[mFilePaths.length];
            for (int i = 0; i < mFilePaths.length; i++) {
                displayNames[i] = resolveFileName(mFilePaths[i]);
            }
            return displayNames;
        }

        @Override
        protected void onPostExecute(String[] result) {
            nativeFilesSelectedInChooser(mProcessId, mRenderId, mModeFlags, mFilePaths, result);
        }

        /**
         * @return the display name of a path if it is a content URI and is present in the database
         * or an empty string otherwise.
         */
        private String resolveFileName(String filePath) {
            if (mContentResolver == null || filePath == null) return "";
            Uri uri = Uri.parse(filePath);
            return ContentUriUtils.getDisplayName(
                    uri, mContentResolver, MediaStore.MediaColumns.DISPLAY_NAME);
        }
    }
}
