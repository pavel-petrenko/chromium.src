// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.video_tutorials;

import android.os.Bundle;

import org.chromium.chrome.browser.SynchronousInitializationActivity;
import org.chromium.chrome.browser.image_fetcher.ImageFetcher;
import org.chromium.chrome.browser.image_fetcher.ImageFetcherConfig;
import org.chromium.chrome.browser.image_fetcher.ImageFetcherFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.video_tutorials.list.TutorialListCoordinator;
import org.chromium.components.browser_ui.util.GlobalDiscardableReferencePool;

/**
 * Activity for displaying a list of video tutorials available to watch.
 */
public class VideoTutorialListActivity extends SynchronousInitializationActivity {
    private TutorialListCoordinator mCoordinator;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.video_tutorial_list);

        Profile profile = Profile.getLastUsedRegularProfile();
        VideoTutorialService videoTutorialService =
                VideoTutorialServiceFactory.getForProfile(profile);
        ImageFetcher imageFetcher =
                ImageFetcherFactory.createImageFetcher(ImageFetcherConfig.IN_MEMORY_WITH_DISK_CACHE,
                        profile, GlobalDiscardableReferencePool.getReferencePool());
        mCoordinator = VideoTutorialServiceFactory.createTutorialListCoordinator(
                findViewById(R.id.recycler_view), videoTutorialService, imageFetcher,
                this::onTutorialSelected);
        findViewById(R.id.close_button).setOnClickListener(v -> finish());
    }

    private void onTutorialSelected(Tutorial tutorial) {
        VideoPlayerActivity.playVideoTutorial(this, tutorial);
    }
}