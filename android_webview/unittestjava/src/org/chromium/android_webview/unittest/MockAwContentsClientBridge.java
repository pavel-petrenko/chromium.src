// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.unittest;

import org.chromium.android_webview.AwContentsClientBridge;
import org.chromium.android_webview.ClientCertLookupTable;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.net.AndroidKeyStore;
import org.chromium.net.AndroidPrivateKey;
import org.chromium.net.DefaultAndroidKeyStore;

class MockAwContentsClientBridge extends AwContentsClientBridge {

    private int mId;
    private String[] mKeyTypes;

    public MockAwContentsClientBridge() {
        super(new DefaultAndroidKeyStore(), new ClientCertLookupTable());
    }

    @Override
    protected void selectClientCertificate(final int id, final String[] keyTypes,
            byte[][] encodedPrincipals, final String host, final int port) {
        mId = id;
        mKeyTypes = keyTypes;
    }

    @CalledByNative
    private static MockAwContentsClientBridge getAwContentsClientBridge() {
        return new MockAwContentsClientBridge();
    }

    @CalledByNative
    private String[] getKeyTypes() {
        return mKeyTypes;
    }

    @CalledByNative
    private int getRequestId() {
        return mId;
    }

    @CalledByNative
    private AndroidPrivateKey createTestPrivateKey() {
        return new AndroidPrivateKey() {
            @Override
            public AndroidKeyStore getKeyStore() {
                return null;
            }
        };
    }

    @CalledByNative
    private byte[][] createTestCertChain() {
        return new byte[][]{{1}};
    }
}
