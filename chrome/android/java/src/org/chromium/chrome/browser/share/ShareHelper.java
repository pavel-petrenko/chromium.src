// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.share;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.SystemClock;
import android.util.Pair;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.ContextUtils;
import org.chromium.base.PackageManagerUtils;
import org.chromium.base.StrictModeContext;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.components.browser_ui.share.ShareParams;
import org.chromium.components.browser_ui.share.ShareParams.TargetChosenCallback;
import org.chromium.ui.base.WindowAndroid;

import java.util.List;

/**
 * A helper class that provides additional Chrome-specific share functionality.
 */
public class ShareHelper extends org.chromium.components.browser_ui.share.ShareHelper {
    private static boolean sIgnoreActivityNotFoundException;

    private ShareHelper() {}

    /*
     * If true, dont throw an ActivityNotFoundException if it is fired when attempting
     * to intent into lens.
     * @param shouldIgnore Whether to catch the exception.
     */
    @VisibleForTesting
    public static void setIgnoreActivityNotFoundExceptionForTesting(boolean shouldIgnore) {
        sIgnoreActivityNotFoundException = shouldIgnore;
    }

    /**
     * Share directly with the provied share target.
     * @param params The container holding the share parameters.
     * @param component The component to share to, bypassing any UI.
     */
    public static void shareDirectly(
            @NonNull ShareParams params, @NonNull ComponentName component) {
        Intent intent = getShareLinkIntent(params);
        intent.addFlags(Intent.FLAG_ACTIVITY_FORWARD_RESULT | Intent.FLAG_ACTIVITY_PREVIOUS_IS_TOP);
        intent.setComponent(component);
        fireIntent(params.getWindow(), intent, null);
    }

    /**
     * Share directly with the last used share target.
     * @param params The container holding the share parameters.
     */
    public static void shareWithLastUsedComponent(@NonNull ShareParams params) {
        ComponentName component = getLastShareComponentName();
        if (component == null) return;
        assert params.getCallback() == null;
        shareDirectly(params, component);
    }

    /**
     * Share with a picker UI.
     * On LMR1 (22) or later this is the Android system share sheet; on L (21) and below this is a
     * custom share dialog.
     * @param params The share parameters.
     * @param saveLastUsed True if the chosen share component should be saved for future reuse.
     */
    // TODO(crbug/1022172): Should be package-protected once modularization is complete.
    public static void showDefaultShareUi(ShareParams params, boolean saveLastUsed) {
        if (saveLastUsed) {
            params.setCallback(new SaveComponentCallback(params.getCallback()));
        }

        ShareHelper.shareWithUi(params);
    }

    /**
     * Share an image URI with an activity identified by the provided Component Name.
     * @param window The current window.
     * @param name The component name of the activity to share the image with.
     * @param imageUri The url to share with the external activity.
     */
    public static void shareImage(
            final WindowAndroid window, final ComponentName name, Uri imageUri) {
        Intent shareIntent = getShareImageIntent(imageUri);
        if (name == null) {
            if (TargetChosenReceiver.isSupported()) {
                TargetChosenReceiver.sendChooserIntent(
                        window, shareIntent, new SaveComponentCallback(null));
            } else {
                Intent chooserIntent = Intent.createChooser(shareIntent,
                        window.getActivity().get().getString(R.string.share_link_chooser_title));
                fireIntent(window, chooserIntent, null);
            }
        } else {
            shareIntent.setComponent(name);
            fireIntent(window, shareIntent, null);
        }
    }

    /**
     * Share an image URI with Google Lens.
     * @param window The current window.
     * @param imageUri The url to share with the app.
     * @param isIncognito Whether the current tab is in incognito mode.
     * @param srcUrl The 'src' attribute of the image.
     * @param titleOrAltText The 'title' or, if empty, the 'alt' attribute of the image.
     * @param isShoppingIntent Whether the Lens intent is a shopping intent.
     * @param requiresConfirmation Whether the request requires an confirmation dialog.
     */
    public static void shareImageWithGoogleLens(final WindowAndroid window, Uri imageUri,
            boolean isIncognito, String srcUrl, String titleOrAltText, boolean isShoppingIntent,
            boolean requiresConfirmation) {
        Intent shareIntent = LensUtils.getShareWithGoogleLensIntent(
                ContextUtils.getApplicationContext(), imageUri, isIncognito,
                SystemClock.elapsedRealtimeNanos(), srcUrl, titleOrAltText,
                isShoppingIntent ? LensUtils.IntentType.SHOPPING : LensUtils.IntentType.DEFAULT,
                requiresConfirmation);
        try {
            // Pass an empty callback to ensure the triggered activity can identify the source
            // of the intent (startActivityForResult allows app identification).
            fireIntent(window, shareIntent, (w, resultCode, data) -> {});
        } catch (ActivityNotFoundException e) {
            // The initial version check should guarantee that the activity is available. However,
            // the exception may be thrown in test environments after mocking out the version check.
            if (Boolean.TRUE.equals(sIgnoreActivityNotFoundException)) return;
            throw e;
        }
    }

    /**
     * Set the icon and the title for the menu item used for direct share.
     * @param context The activity context used to retrieve resources.
     * @param item The menu item that is used for direct share
     */
    public static void configureDirectShareMenuItem(Context context, MenuItem item) {
        Intent shareIntent = getShareLinkAppCompatibilityIntent();
        Pair<Drawable, CharSequence> directShare = getShareableIconAndName(shareIntent);
        Drawable directShareIcon = directShare.first;
        CharSequence directShareTitle = directShare.second;

        item.setIcon(directShareIcon);
        if (directShareTitle != null) {
            item.setTitle(
                    context.getString(R.string.accessibility_menu_share_via, directShareTitle));
        }
    }

    /**
     * Get the icon and name of the most recently shared app by certain app.
     * @param shareIntent Intent used to get list of apps support sharing.
     * @return The Image and the String of the recently shared Icon.
     */
    public static Pair<Drawable, CharSequence> getShareableIconAndName(Intent shareIntent) {
        Drawable directShareIcon = null;
        CharSequence directShareTitle = null;

        final ComponentName component = getLastShareComponentName();
        boolean isComponentValid = false;
        if (component != null) {
            shareIntent.setPackage(component.getPackageName());
            List<ResolveInfo> resolveInfoList =
                    PackageManagerUtils.queryIntentActivities(shareIntent, 0);
            for (ResolveInfo info : resolveInfoList) {
                ActivityInfo ai = info.activityInfo;
                if (component.equals(new ComponentName(ai.applicationInfo.packageName, ai.name))) {
                    isComponentValid = true;
                    break;
                }
            }
        }
        if (isComponentValid) {
            boolean retrieved = false;
            final PackageManager pm = ContextUtils.getApplicationContext().getPackageManager();
            try {
                // TODO(dtrainor): Make asynchronous and have a callback to update the menu.
                // https://crbug.com/729737
                try (StrictModeContext ignored = StrictModeContext.allowDiskReads()) {
                    directShareIcon = pm.getActivityIcon(component);
                    directShareTitle = pm.getActivityInfo(component, 0).loadLabel(pm);
                }
                retrieved = true;
            } catch (NameNotFoundException exception) {
                // Use the default null values.
            }
            RecordHistogram.recordBooleanHistogram(
                    "Android.IsLastSharedAppInfoRetrieved", retrieved);
        }

        return new Pair<>(directShareIcon, directShareTitle);
    }

    /**
     * Stores the component selected for sharing last time share was called by certain app.
     *
     * This method is public since it is used in tests to avoid creating share dialog.
     * @param component The {@link ComponentName} of the app selected for sharing.
     */
    @VisibleForTesting
    public static void setLastShareComponentName(ComponentName component) {
        SharedPreferencesManager preferencesManager = SharedPreferencesManager.getInstance();
        preferencesManager.writeString(
                ChromePreferenceKeys.SHARING_LAST_SHARED_PACKAGE_NAME, component.getPackageName());
        preferencesManager.writeString(
                ChromePreferenceKeys.SHARING_LAST_SHARED_CLASS_NAME, component.getClassName());
    }

    /**
     * A {@link TargetChosenCallback} that wraps another callback, forwarding calls to it, and
     * saving the chosen component.
     */
    static class SaveComponentCallback implements TargetChosenCallback {
        private TargetChosenCallback mOriginalCallback;

        public SaveComponentCallback(@Nullable TargetChosenCallback originalCallback) {
            mOriginalCallback = originalCallback;
        }

        @Override
        public void onTargetChosen(ComponentName chosenComponent) {
            setLastShareComponentName(chosenComponent);
            if (mOriginalCallback != null) mOriginalCallback.onTargetChosen(chosenComponent);
        }

        @Override
        public void onCancel() {
            if (mOriginalCallback != null) mOriginalCallback.onCancel();
        }
    }

    /**
     * Returns an Intent to retrieve all the apps that support sharing {@code fileContentType}.
     */
    public static Intent createShareFileAppCompatibilityIntent(String fileContentType) {
        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        intent.setType(fileContentType);
        return intent;
    }

    /**
     * Creates an Intent to share an image.
     * @param imageUri The Uri of the image.
     * @return The Intent used to share the image.
     */
    public static Intent getShareImageIntent(Uri imageUri) {
        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        intent.setType("image/jpeg");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.putExtra(Intent.EXTRA_STREAM, imageUri);
        return intent;
    }

    /**
     * Gets the {@link ComponentName} of the app that was used to last share.
     */
    @Nullable
    public static ComponentName getLastShareComponentName() {
        SharedPreferencesManager preferencesManager = SharedPreferencesManager.getInstance();
        String packageName = preferencesManager.readString(
                ChromePreferenceKeys.SHARING_LAST_SHARED_PACKAGE_NAME, null);
        String className = preferencesManager.readString(
                ChromePreferenceKeys.SHARING_LAST_SHARED_CLASS_NAME, null);
        return createComponentName(packageName, className);
    }

    private static ComponentName createComponentName(String packageName, String className) {
        if (packageName == null || className == null) return null;
        return new ComponentName(packageName, className);
    }
}
