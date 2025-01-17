// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.password_manager;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.password_check.PasswordCheckFactory;
import org.chromium.chrome.browser.password_check.PasswordCheckReferrer;
import org.chromium.ui.base.WindowAndroid;

/**
 * A utitily class for launching the password leak check.
 */
public class PasswordCheckupLauncher {
    @CalledByNative
    private static void launchCheckupInAccountWithWindowAndroid(
            String checkupUrl, WindowAndroid windowAndroid) {
        if (windowAndroid.getContext().get() == null) return; // Window not available yet/anymore.
        ChromeActivity activity = (ChromeActivity) windowAndroid.getActivity().get();
        launchCheckupInAccountWithActivity(checkupUrl, activity);
    }

    @CalledByNative
    private static void launchLocalCheckup(WindowAndroid windowAndroid) {
        assert ChromeFeatureList.isEnabled(ChromeFeatureList.PASSWORD_CHECK);
        if (windowAndroid.getContext().get() == null) return; // Window not available yet/anymore.
        PasswordCheckFactory.getOrCreate().showUi(
                windowAndroid.getContext().get(), PasswordCheckReferrer.LEAK_DIALOG);
    }

    @CalledByNative
    private static void launchCheckupInAccountWithActivity(String checkupUrl, Activity activity) {
        if (tryLaunchingNativePasswordCheckup(activity)) return;
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(checkupUrl));
        intent.setPackage(activity.getPackageName());
        activity.startActivity(intent);
    }

    private static boolean tryLaunchingNativePasswordCheckup(Activity activity) {
        GooglePasswordManagerUIProvider googlePasswordManagerUIProvider =
                AppHooks.get().createGooglePasswordManagerUIProvider();
        if (googlePasswordManagerUIProvider == null) return false;
        return googlePasswordManagerUIProvider.launchPasswordCheckup(activity);
    }
}
