// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab.state;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.chrome.browser.tab.MockTab;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.WebContentsState;
import org.chromium.chrome.test.ChromeBrowserTestRule;

import java.nio.ByteBuffer;
import java.util.concurrent.Semaphore;

/**
 * Test relating to {@link CriticalPersistedTabData}
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CriticalPersistedTabDataTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    private static final int TAB_ID = 1;
    private static final int PARENT_ID = 2;
    private static final int ROOT_ID = 3;
    private static final int CONTENT_STATE_VERSION = 42;
    private static final byte[] WEB_CONTENTS_STATE_BYTES = {9, 10};
    private static final WebContentsState WEB_CONTENTS_STATE =
            new WebContentsState(ByteBuffer.allocateDirect(WEB_CONTENTS_STATE_BYTES.length));
    private static final long TIMESTAMP = 203847028374L;
    private static final String APP_ID = "AppId";
    private static final String OPENER_APP_ID = "OpenerAppId";
    private static final int THEME_COLOR = 5;
    private static final Integer LAUNCH_TYPE_AT_CREATION = 3;

    static {
        WEB_CONTENTS_STATE.buffer().put(WEB_CONTENTS_STATE_BYTES);
    }

    private CriticalPersistedTabData mCriticalPersistedTabData;

    private MockPersistedTabDataStorage mStorage;

    private static Tab mockTab(int id, boolean isEncrypted) {
        return new MockTab(id, isEncrypted);
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        PersistedTabDataConfiguration.setUseTestConfig(true);
        mStorage =
                (MockPersistedTabDataStorage) PersistedTabDataConfiguration.getTestConfig().storage;
    }

    @SmallTest
    @Test
    public void testNonEncryptedSaveRestore() throws InterruptedException {
        testSaveRestoreDelete(false);
    }

    @SmallTest
    @Test
    public void testEncryptedSaveRestoreDelete() throws InterruptedException {
        testSaveRestoreDelete(true);
    }

    private void testSaveRestoreDelete(boolean isEncrypted) throws InterruptedException {
        final Semaphore semaphore = new Semaphore(0);
        Callback<CriticalPersistedTabData> callback = new Callback<CriticalPersistedTabData>() {
            @Override
            public void onResult(CriticalPersistedTabData res) {
                mCriticalPersistedTabData = res;
                if (mCriticalPersistedTabData != null) {
                    mCriticalPersistedTabData.setIsStorageRetrievalEnabled(true);
                }
                semaphore.release();
            }
        };
        final Semaphore saveSemaphore = new Semaphore(0);
        ThreadUtils.runOnUiThreadBlocking(() -> {
            CriticalPersistedTabData criticalPersistedTabData =
                    new CriticalPersistedTabData(mockTab(TAB_ID, isEncrypted), "", "", PARENT_ID,
                            ROOT_ID, TIMESTAMP, WEB_CONTENTS_STATE, CONTENT_STATE_VERSION,
                            OPENER_APP_ID, THEME_COLOR, LAUNCH_TYPE_AT_CREATION);
            mStorage.setSemaphore(saveSemaphore);
            criticalPersistedTabData.saveForTesting();
            acquireSemaphore(saveSemaphore);
            CriticalPersistedTabData.from(mockTab(TAB_ID, isEncrypted), callback);
        });
        semaphore.acquire();
        Assert.assertNotNull(mCriticalPersistedTabData);
        Assert.assertEquals(mCriticalPersistedTabData.getParentId(), PARENT_ID);
        Assert.assertEquals(mCriticalPersistedTabData.getRootId(), ROOT_ID);
        Assert.assertEquals(mCriticalPersistedTabData.getTimestampMillis(), TIMESTAMP);
        Assert.assertEquals(
                mCriticalPersistedTabData.getContentStateVersion(), CONTENT_STATE_VERSION);
        Assert.assertEquals(mCriticalPersistedTabData.getOpenerAppId(), OPENER_APP_ID);
        Assert.assertEquals(mCriticalPersistedTabData.getThemeColor(), THEME_COLOR);
        Assert.assertEquals(
                mCriticalPersistedTabData.getTabLaunchTypeAtCreation(), LAUNCH_TYPE_AT_CREATION);
        Assert.assertArrayEquals(CriticalPersistedTabData.getContentStateByteArray(
                                         mCriticalPersistedTabData.getWebContentsState().buffer()),
                WEB_CONTENTS_STATE_BYTES);
        Semaphore deleteSemaphore = new Semaphore(0);
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mStorage.setSemaphore(deleteSemaphore);
            mCriticalPersistedTabData.delete();
            acquireSemaphore(deleteSemaphore);
            CriticalPersistedTabData.from(mockTab(TAB_ID, isEncrypted), callback);
        });
        semaphore.acquire();
        Assert.assertNull(mCriticalPersistedTabData);
        // TODO(crbug.com/1060232) test restored.save() after restored.delete()
        // Also cover
        // - Multiple (different) TAB_IDs being stored in the same storage.
        // - Tests for doing multiple operations on the same TAB_ID:
        //   - save() multiple times
        //  - restore() multiple times
        //  - delete() multiple times
    }

    private static void acquireSemaphore(Semaphore semaphore) {
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            // Throw Runtime exception to make catching InterruptedException unnecessary
            throw new RuntimeException(e);
        }
    }
}
