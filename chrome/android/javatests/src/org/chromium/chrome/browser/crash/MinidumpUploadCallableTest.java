// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.crash;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.preferences.privacy.CrashReportingPermissionManager;
import org.chromium.chrome.browser.util.HttpURLConnectionFactory;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Calendar;

/**
 * Unittests for {@link MinidumpUploadCallable}.
 */
public class MinidumpUploadCallableTest extends CrashTestCase {
    private static final String BOUNDARY = "TESTBOUNDARY";
    private static final String CRASH_ID = "IMACRASHID";
    private static final String LOG_FILE_NAME = "chromium_renderer-123_log.dmp224";
    private File mTestUpload;
    private File mUploadLog;
    private File mExpectedFileAfterUpload;

    private static class TestHttpURLConnection extends HttpURLConnection {
        private static final String EXPECTED_CONTENT_TYPE_VALUE =
                String.format(MinidumpUploadCallable.CONTENT_TYPE_TMPL, BOUNDARY);

        /**
         * The value of the "Content-Type" property if the property has been set.
         */
        private String mContentTypePropertyValue = "";

        TestHttpURLConnection(URL url) {
            super(url);
            assertEquals(MinidumpUploadCallable.CRASH_URL_STRING, url.toString());
        }

        @Override
        public void disconnect() {
            // Check that the "Content-Type" property has been set and the property's value.
            assertEquals(EXPECTED_CONTENT_TYPE_VALUE, mContentTypePropertyValue);
        }

        @Override
        public InputStream getInputStream() {
            return new ByteArrayInputStream(CRASH_ID.getBytes());
        }

        @Override
        public OutputStream getOutputStream() {
            return new ByteArrayOutputStream();
        }

        @Override
        public int getResponseCode() {
            return 200;
        }

        @Override
        public String getResponseMessage() {
            return null;
        }

        @Override
        public boolean usingProxy() {
            return false;
        }

        @Override
        public void connect() {
        }

        @Override
        public void setRequestProperty(String key, String value) {
            if (key.equals("Content-Type")) {
                mContentTypePropertyValue = value;
            }
        }
    }

    private static class TestHttpURLConnectionFactory implements HttpURLConnectionFactory {
        @Override
        public HttpURLConnection createHttpURLConnection(String url) {
            try {
                return new TestHttpURLConnection(new URL(url));
            } catch (IOException e) {
                return null;
            }
        }
    }

    private static class FailHttpURLConnectionFactory implements HttpURLConnectionFactory {
        @Override
        public HttpURLConnection createHttpURLConnection(String url) {
            fail();
            return null;
        }
    }

    private static class MockCrashReportingPermissionManager
            implements CrashReportingPermissionManager {
        private final boolean mIsPermitted;
        private final boolean mIsLimitted;

        MockCrashReportingPermissionManager(boolean isPermitted, boolean isLimitted) {
            mIsPermitted = isPermitted;
            mIsLimitted = isLimitted;
        }

        @Override
        public boolean isUploadPermitted() {
            return mIsPermitted;
        }

        @Override
        public boolean isUploadLimited() {
            return mIsLimitted;
        }
    }

    /**
     * This class calls |getInstrumentation| which cannot be done in a static context.
     */
    private class MockMinidumpUploadCallable extends MinidumpUploadCallable {
        private Calendar mCalendar;

        MockMinidumpUploadCallable(
                HttpURLConnectionFactory httpURLConnectionFactory,
                CrashReportingPermissionManager permManager) {
            super(mTestUpload, mUploadLog, httpURLConnectionFactory, permManager,
                    PreferenceManager.getDefaultSharedPreferences(
                            getInstrumentation().getTargetContext()));
            mCalendar = Calendar.getInstance();
            mCalendar.clear();
            mCalendar.set(Calendar.YEAR, 2014);
            mCalendar.set(Calendar.DAY_OF_YEAR, 14);
        }

        @Override
        protected int getCurrentDay() {
            return super.getCurrentDay(mCalendar);
        }

        @Override
        protected int getFirstDayOfCurrentWeek() {
            return super.getFirstDayOfCurrentWeek(mCalendar);
        }
    }

    private void createMinidumpFile() throws Exception {
        mTestUpload = new File(mCrashDir, LOG_FILE_NAME);
        setUpMinidumpFile(mTestUpload, BOUNDARY);
    }

    @SuppressFBWarnings("RV_RETURN_VALUE_IGNORED_BAD_PRACTICE")
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mUploadLog = new File(mCacheDir, CrashFileManager.CRASH_DUMP_LOGFILE);
        // Delete all logs from previous runs if possible.
        mUploadLog.delete();

        createMinidumpFile();
        mExpectedFileAfterUpload = new File(
                mCrashDir,
                mTestUpload.getName().replaceFirst("\\.dmp", ".up"));
    }

    @SuppressFBWarnings("RV_RETURN_VALUE_IGNORED_BAD_PRACTICE")
    @Override
    protected void tearDown() throws Exception {
        if (mTestUpload.exists()) mTestUpload.delete();
        super.tearDown();
    }

    @SmallTest
    @Feature({"Android-AppBase"})
    public void testCallWhenCurrentlyPermitted() throws Exception {
        CrashReportingPermissionManager testPermManager =
                new MockCrashReportingPermissionManager(true, false);

        HttpURLConnectionFactory httpURLConnectionFactory = new TestHttpURLConnectionFactory();

        MinidumpUploadCallable minidumpUploadCallable =
                new MockMinidumpUploadCallable(httpURLConnectionFactory, testPermManager);
        assertTrue(minidumpUploadCallable.call());
        assertTrue(mExpectedFileAfterUpload.exists());
        assertValidUploadLogEntry();
    }

    @SmallTest
    @Feature({"Android-AppBase"})
    public void testCallPermittedButNotUnderCurrentCircumstances() throws Exception {
        CrashReportingPermissionManager testPermManager =
                new MockCrashReportingPermissionManager(false, false);

        HttpURLConnectionFactory httpURLConnectionFactory = new FailHttpURLConnectionFactory();

        MinidumpUploadCallable minidumpUploadCallable =
                new MockMinidumpUploadCallable(httpURLConnectionFactory, testPermManager);
        assertFalse(minidumpUploadCallable.call());
        assertFalse(mExpectedFileAfterUpload.exists());
    }

    @SmallTest
    @Feature({"Android-AppBase"})
    public void testCrashUploadSizeConstraints() throws Exception {
        CrashReportingPermissionManager testPermManager =
                new MockCrashReportingPermissionManager(true, true);

        HttpURLConnectionFactory httpURLConnectionFactory = new TestHttpURLConnectionFactory();
        extendUploadFile(MinidumpUploadCallable.LOG_SIZE_LIMIT_BYTES);

        MinidumpUploadCallable minidumpUploadCallable =
                new MockMinidumpUploadCallable(httpURLConnectionFactory, testPermManager);
        setUpCrashPreferences(minidumpUploadCallable.getCurrentDay(),
                MinidumpUploadCallable.LOG_UPLOAD_LIMIT_PER_DAY - 1,
                minidumpUploadCallable.getFirstDayOfCurrentWeek(), 0);
        assertFalse(minidumpUploadCallable.call());
        assertFalse(mExpectedFileAfterUpload.exists());
    }

    @SmallTest
    @Feature({"Android-AppBase"})
    public void testCrashUploadSizeNotLimited() throws Exception {
        CrashReportingPermissionManager testPermManager =
                new MockCrashReportingPermissionManager(true, false);

        HttpURLConnectionFactory httpURLConnectionFactory = new TestHttpURLConnectionFactory();
        extendUploadFile(MinidumpUploadCallable.LOG_SIZE_LIMIT_BYTES);

        MinidumpUploadCallable minidumpUploadCallable =
                new MockMinidumpUploadCallable(httpURLConnectionFactory, testPermManager);
        setUpCrashPreferences(minidumpUploadCallable.getCurrentDay(),
                MinidumpUploadCallable.LOG_UPLOAD_LIMIT_PER_DAY - 1,
                minidumpUploadCallable.getFirstDayOfCurrentWeek(), 0);
        assertTrue(minidumpUploadCallable.call());
        assertTrue(mExpectedFileAfterUpload.exists());
    }

    @SmallTest
    @Feature({"Android-AppBase"})
    public void testCrashUploadFrequencyConstraints() throws Exception {
        CrashReportingPermissionManager testPermManager =
                new MockCrashReportingPermissionManager(true, true);

        HttpURLConnectionFactory httpURLConnectionFactory = new TestHttpURLConnectionFactory();

        MinidumpUploadCallable minidumpUploadCallable =
                new MockMinidumpUploadCallable(httpURLConnectionFactory, testPermManager);
        setUpCrashPreferences(minidumpUploadCallable.getCurrentDay(),
                MinidumpUploadCallable.LOG_UPLOAD_LIMIT_PER_DAY,
                minidumpUploadCallable.getFirstDayOfCurrentWeek(), 0);
        assertFalse(minidumpUploadCallable.call());
        assertFalse(mExpectedFileAfterUpload.exists());

        setUpCrashPreferences(minidumpUploadCallable.getCurrentDay(),
                MinidumpUploadCallable.LOG_UPLOAD_LIMIT_PER_DAY - 1,
                minidumpUploadCallable.getFirstDayOfCurrentWeek(), 0);
        assertTrue(minidumpUploadCallable.call());
        assertTrue(mExpectedFileAfterUpload.exists());

        // After a successful upload we need to the create upload file again.
        createMinidumpFile();

        setUpCrashPreferences(minidumpUploadCallable.getCurrentDay() - 1,
                MinidumpUploadCallable.LOG_UPLOAD_LIMIT_PER_DAY,
                minidumpUploadCallable.getFirstDayOfCurrentWeek(), 0);
        assertTrue(minidumpUploadCallable.call());
        assertTrue(mExpectedFileAfterUpload.exists());
    }

    @SmallTest
    @Feature({"Android-AppBase"})
    public void testCrashUploadWeeklyConstraints() throws Exception {
        CrashReportingPermissionManager testPermManager =
                new MockCrashReportingPermissionManager(true, true);

        HttpURLConnectionFactory httpURLConnectionFactory = new TestHttpURLConnectionFactory();

        MinidumpUploadCallable minidumpUploadCallable =
                new MockMinidumpUploadCallable(httpURLConnectionFactory, testPermManager);
        int fileSize = MinidumpUploadCallable.LOG_SIZE_LIMIT_BYTES / 10;
        extendUploadFile(fileSize);
        // This shouldn't work as weekly size cannot allow one more |fileSize| upload.
        setUpCrashPreferences(minidumpUploadCallable.getCurrentDay(), 2,
                minidumpUploadCallable.getFirstDayOfCurrentWeek(),
                MinidumpUploadCallable.LOG_WEEKLY_SIZE_LIMIT_BYTES - fileSize / 2);
        assertFalse(minidumpUploadCallable.call());
        assertFalse(mExpectedFileAfterUpload.exists());

        // This upload should be allowed although there is not enough weekly limit left, because as
        // the week changes the weekly limit should get reset.
        setUpCrashPreferences(minidumpUploadCallable.getCurrentDay() - 1,
                MinidumpUploadCallable.LOG_UPLOAD_LIMIT_PER_DAY - 1,
                minidumpUploadCallable.getFirstDayOfCurrentWeek() - 7,
                MinidumpUploadCallable.LOG_WEEKLY_SIZE_LIMIT_BYTES - fileSize / 2);
        assertTrue(minidumpUploadCallable.call());
        assertTrue(mExpectedFileAfterUpload.exists());

        // After a successful upload we need to create the upload file again.
        createMinidumpFile();
        extendUploadFile(fileSize);

        // This upload should be allowed as there is enough weekly limit left.
        assertTrue(minidumpUploadCallable.call());
        assertTrue(mExpectedFileAfterUpload.exists());
    }

    private void setUpCrashPreferences(int lastDay, int count, int lastWeek, long totalSize) {
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(
                getInstrumentation().getTargetContext());
        pref.edit()
                .putInt(MinidumpUploadCallable.PREF_LAST_UPLOAD_DAY, lastDay)
                .putInt(MinidumpUploadCallable.PREF_DAY_UPLOAD_COUNT, count)
                .putInt(MinidumpUploadCallable.PREF_LAST_UPLOAD_WEEK, lastWeek)
                .putLong(MinidumpUploadCallable.PREF_WEEK_UPLOAD_SIZE, totalSize)
                .apply();
    }

    private void extendUploadFile(int numBytes) throws FileNotFoundException, IOException {
        FileOutputStream stream = null;
        try {
            stream = new FileOutputStream(mTestUpload, true);
            byte[] buf = new byte[numBytes];
            stream.write(buf);
            stream.flush();
        } finally {
            if (stream != null) stream.close();
        }
    }

    private void assertValidUploadLogEntry() throws IOException {
        File logfile = new File(mCacheDir, CrashFileManager.CRASH_DUMP_LOGFILE);
        BufferedReader input =  new BufferedReader(new FileReader(logfile));
        String line = null;
        String lastEntry = null;
        while ((line = input.readLine()) != null) {
            lastEntry = line;
        }
        input.close();

        assertNotNull("We do not have a single entry in uploads.log", lastEntry);
        int seperator = lastEntry.indexOf(',');

        long time = Long.parseLong(lastEntry.substring(0, seperator));
        long now = System.currentTimeMillis() / 1000; // Timestamp was in seconds.

        // Sanity check on the time stamp (within an hour).
        // Chances are the write and the check should have less than 1 second in between.
        assertTrue(time <= now);
        assertTrue(time > now - 60 * 60);

        String id = lastEntry.substring(seperator + 1, lastEntry.length());
        assertEquals(id, CRASH_ID);
    }
}
