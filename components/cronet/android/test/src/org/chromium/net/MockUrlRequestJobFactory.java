// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import static junit.framework.Assert.assertTrue;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.net.test.FailurePhase;

/**
 * Helper class to set up url interceptors for testing purposes.
 */
@JNINamespace("cronet")
public final class MockUrlRequestJobFactory {
    /**
     * Sets up URL interceptors.
     */
    public static void setUp() {
        nativeAddUrlInterceptors();
    }

    /**
     * Constructs a mock URL that hangs or fails at certain phase.
     *
     * @param phase at which request fails. It should be a value in
     *              org.chromium.net.test.FailurePhase.
     * @param netError reported by UrlRequestJob. Passing -1, results in hang.
     */
    public static String getMockUrlWithFailure(int phase, int netError) {
        assertTrue(netError < 0);
        switch (phase) {
            case FailurePhase.START:
            case FailurePhase.READ_SYNC:
            case FailurePhase.READ_ASYNC:
                break;
            default:
                throw new IllegalArgumentException(
                        "phase not in org.chromium.net.test.FailurePhase");
        }
        return nativeGetMockUrlWithFailure(phase, netError);
    }

    /**
     * Constructs a mock URL that synchronously responds with data repeated many
     * times.
     *
     * @param data to return in response.
     * @param dataRepeatCount number of times to repeat the data.
     */
    public static String getMockUrlForData(String data, int dataRepeatCount) {
        return nativeGetMockUrlForData(data, dataRepeatCount);
    }

    /**
     * Constructs a mock URL that will fail with an SSL certificate error.
     */
    public static String getMockUrlForSSLCertificateError() {
        return nativeGetMockUrlForSSLCertificateError();
    }

    private static native void nativeAddUrlInterceptors();

    private static native String nativeGetMockUrlWithFailure(int phase, int netError);

    private static native String nativeGetMockUrlForData(String data,
            int dataRepeatCount);

    private static native String nativeGetMockUrlForSSLCertificateError();
}
