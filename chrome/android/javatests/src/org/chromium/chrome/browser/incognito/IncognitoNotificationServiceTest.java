// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.incognito;

import static org.junit.Assert.assertEquals;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.Context;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.util.Pair;

import androidx.test.filters.MediumTest;

import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tab.TabStateFileManager;
import org.chromium.chrome.browser.tabmodel.TestTabModelDirectory;
import org.chromium.chrome.browser.tabmodel.TestTabModelDirectory.TabStateInfo;
import org.chromium.chrome.browser.tabpersistence.TabStateDirectory;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.test.util.Criteria;
import org.chromium.content_public.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.io.File;
import java.util.concurrent.Callable;

/**
 * Tests for the Incognito Notification service.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class IncognitoNotificationServiceTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private void createTabOnUiThread() {
        TestThreadUtils.runOnUiThreadBlocking(
                (Runnable) () -> mActivityTestRule.getActivity().getTabCreator(true).createNewTab(
                                new LoadUrlParams("about:blank"), TabLaunchType.FROM_CHROME_UI,
                                null));
    }

    private void sendClearIncognitoIntent() throws CanceledException {
        PendingIntent clearIntent =
                IncognitoNotificationService
                        .getRemoveAllIncognitoTabsIntent(InstrumentationRegistry.getTargetContext())
                        .getPendingIntent();
        clearIntent.send();
    }

    @Test
    @Feature("Incognito")
    @MediumTest
    public void testSingleRunningChromeTabbedActivity()
            throws InterruptedException, CanceledException {
        mActivityTestRule.startMainActivityOnBlankPage();

        createTabOnUiThread();
        createTabOnUiThread();

        CriteriaHelper.pollUiThread(() -> {
            Criteria.checkThat(
                    mActivityTestRule.getActivity().getTabModelSelector().getModel(true).getCount(),
                    Matchers.is(2));
        });

        final Profile incognitoProfile =
                TestThreadUtils.runOnUiThreadBlockingNoException(new Callable<Profile>() {
                    @Override
                    public Profile call() {
                        return mActivityTestRule.getActivity()
                                .getTabModelSelector()
                                .getModel(true)
                                .getProfile();
                    }
                });
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertTrue(incognitoProfile.isOffTheRecord());
            Assert.assertTrue(incognitoProfile.isNativeInitialized());
        });

        sendClearIncognitoIntent();

        CriteriaHelper.pollUiThread(() -> {
            Criteria.checkThat(
                    mActivityTestRule.getActivity().getTabModelSelector().getModel(true).getCount(),
                    Matchers.is(0));
        });
        CriteriaHelper.pollUiThread(() -> !incognitoProfile.isNativeInitialized());
    }

    @Test
    @Feature("Incognito")
    @MediumTest
    @DisabledTest
    // https://crbug.com/1033835
    public void testNoAliveProcess() throws Exception {
        Context context = InstrumentationRegistry.getTargetContext();
        final TestTabModelDirectory tabbedModeDirectory = new TestTabModelDirectory(
                context, "tabs", String.valueOf(0));

        // Add a couple non-incognito tabs (their filenames use a different prefix, so we do not
        // need to worry about ID space collisions with the generated incognito tabs).
        tabbedModeDirectory.writeTabStateFile(TestTabModelDirectory.V2_DUCK_DUCK_GO);
        tabbedModeDirectory.writeTabStateFile(TestTabModelDirectory.V2_BAIDU);

        // Generate a few incognito tabs (using arbitrary data from an existing TabState
        // definition).
        for (int i = 0; i < 3; i++) {
            TabStateInfo incognitoInfo = new TabStateInfo(
                    true,
                    TestTabModelDirectory.V2_TEXTAREA.version,
                    i,
                    TestTabModelDirectory.V2_TEXTAREA.url,
                    TestTabModelDirectory.V2_TEXTAREA.title,
                    TestTabModelDirectory.V2_TEXTAREA.encodedTabState);
            tabbedModeDirectory.writeTabStateFile(incognitoInfo);
        }

        TabStateDirectory.setBaseStateDirectoryForTests(tabbedModeDirectory.getBaseDirectory());

        File[] tabbedModeFiles = tabbedModeDirectory.getDataDirectory().listFiles();
        Assert.assertNotNull(tabbedModeFiles);
        Assert.assertEquals(5, tabbedModeFiles.length);

        int incognitoCount = 0;
        int normalCount = 0;
        for (File tabbedModeFile : tabbedModeFiles) {
            Pair<Integer, Boolean> tabFileInfo =
                    TabStateFileManager.parseInfoFromFilename(tabbedModeFile.getName());
            if (tabFileInfo.second) incognitoCount++;
            else normalCount++;
        }
        Assert.assertEquals(2, normalCount);
        Assert.assertEquals(3, incognitoCount);

        sendClearIncognitoIntent();

        CriteriaHelper.pollInstrumentationThread(() -> {
            File[] actualTabbedModeFiles = tabbedModeDirectory.getDataDirectory().listFiles();
            if (actualTabbedModeFiles == null) return;
            int actualIncognitoCount = 0;
            for (File tabbedModeFile : actualTabbedModeFiles) {
                Pair<Integer, Boolean> tabFileInfo =
                        TabStateFileManager.parseInfoFromFilename(tabbedModeFile.getName());
                if (tabFileInfo != null && tabFileInfo.second) actualIncognitoCount++;
            }
            Criteria.checkThat(actualIncognitoCount, Matchers.is(0));
        });

        CriteriaHelper.pollInstrumentationThread(() -> {
            File[] actualTabbedModeFiles = tabbedModeDirectory.getDataDirectory().listFiles();
            Criteria.checkThat(actualTabbedModeFiles, Matchers.notNullValue());
            int actualNormalCount = 0;
            for (File tabbedModeFile : actualTabbedModeFiles) {
                Pair<Integer, Boolean> tabFileInfo =
                        TabStateFileManager.parseInfoFromFilename(tabbedModeFile.getName());
                if (tabFileInfo != null && !tabFileInfo.second) actualNormalCount++;
            }
            Criteria.checkThat(actualNormalCount, Matchers.is(2));
        });

        TestThreadUtils.runOnUiThreadBlocking(
                () -> Assert.assertFalse(LibraryLoader.getInstance().isInitialized()));
    }

    @Test
    @MediumTest
    @Feature("Incognito")
    @TargetApi(Build.VERSION_CODES.M)
    @SuppressLint("NewApi")
    public void testCloseAllIncognitoNotificationIsDisplayed() {
        mActivityTestRule.startMainActivityOnBlankPage();

        createTabOnUiThread();
        createTabOnUiThread();

        CriteriaHelper.pollUiThread(() -> {
            Criteria.checkThat(
                    mActivityTestRule.getActivity().getTabModelSelector().getModel(true).getCount(),
                    Matchers.is(2));
        });

        Context context = ContextUtils.getApplicationContext();
        NotificationManager nm =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        assertEquals(1, nm.getActiveNotifications().length);
    }
}
