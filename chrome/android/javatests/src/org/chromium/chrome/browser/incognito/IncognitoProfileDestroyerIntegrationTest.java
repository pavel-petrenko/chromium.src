// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.incognito;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import androidx.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.JniMocker;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.util.concurrent.ExecutionException;

/**
 * Integration tests for {@link IncognitoProfileDestroyer}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class IncognitoProfileDestroyerIntegrationTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    @Rule
    public JniMocker jniMocker = new JniMocker();

    private TabModel mIncognitoTabModel;

    @Mock
    ProfileManager.Observer mMockProfileManagerObserver;

    @Before
    public void setUp() throws InterruptedException {
        MockitoAnnotations.initMocks(this);

        mActivityTestRule.startMainActivityOnBlankPage();
        ProfileManager.addObserver(mMockProfileManagerObserver);
        mIncognitoTabModel = mActivityTestRule.getActivity().getTabModelSelector().getModel(true);
    }

    @Test
    @MediumTest
    @Feature({"OffTheRecord"})
    public void test_closeOnlyTab_profileDestroyed() throws ExecutionException {
        // Open a single incognito tab
        Tab onlyTab = mActivityTestRule.newIncognitoTabFromMenu();

        // Verify the tab is opened and the TabModel now has an incognito Profile
        assertEquals(1, mIncognitoTabModel.getCount());
        Profile incognitoProfile =
                TestThreadUtils.runOnUiThreadBlocking(() -> mIncognitoTabModel.getProfile());
        assertNotNull(incognitoProfile);
        verify(mMockProfileManagerObserver, never()).onProfileDestroyed(any());

        // Close the incognito tab
        TestThreadUtils.runOnUiThreadBlocking(() -> mIncognitoTabModel.closeTab(onlyTab));

        // Verify the incognito Profile was destroyed
        verify(mMockProfileManagerObserver).onProfileDestroyed(eq(incognitoProfile));
        incognitoProfile =
                TestThreadUtils.runOnUiThreadBlocking(() -> mIncognitoTabModel.getProfile());
        assertNull(incognitoProfile);
    }

    @Test
    @MediumTest
    @Feature({"OffTheRecord"})
    public void test_closeOneOfTwoTabs_profileNotDestroyed() throws ExecutionException {
        // Open two incognito tabs
        Tab firstTab = mActivityTestRule.newIncognitoTabFromMenu();
        mActivityTestRule.newIncognitoTabFromMenu();

        // Verify the tabs are opened and the TabModel now has an incognito Profile
        assertEquals(2, mIncognitoTabModel.getCount());
        Profile incognitoProfile =
                TestThreadUtils.runOnUiThreadBlocking(() -> mIncognitoTabModel.getProfile());
        assertNotNull(incognitoProfile);
        verify(mMockProfileManagerObserver, never()).onProfileDestroyed(any());

        // Close one incognito tab
        TestThreadUtils.runOnUiThreadBlocking(() -> mIncognitoTabModel.closeTab(firstTab));

        // Verify the incognito Profile was not destroyed
        verify(mMockProfileManagerObserver, never()).onProfileDestroyed(any());
        incognitoProfile =
                TestThreadUtils.runOnUiThreadBlocking(() -> mIncognitoTabModel.getProfile());
        assertNotNull(incognitoProfile);
    }
}
