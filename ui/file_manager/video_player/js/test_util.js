// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Returns if the specified file is being played.
 *
 * @param {string} filename Name of audio file to be checked. This must be same
 *     as entry.name() of the audio file.
 * @return {boolean} True if the video is playing, false otherwise.
 */
test.util.sync.isPlaying = function(filename) {
  for (var appId in window.background.appWindows) {
    var contentWindow = window.background.appWindows[appId].contentWindow;
    if (contentWindow &&
        contentWindow.document.title === filename) {
      var element = contentWindow.document.querySelector('video[src]');
      if (element && !element.paused)
        return true;
    }
  }
  return false;
};

/**
 * Loads the mock of the cast extension.
 *
 * @param {Window} contentWindow Video player window to be chacked toOB.
 */
test.util.sync.loadMockCastExtension = function(contentWindow) {
  var script = contentWindow.document.createElement('script');
  script.src =
      'chrome-extension://ljoplibgfehghmibaoaepfagnmbbfiga/' +
      'cast_extension_mock/load.js';
  contentWindow.document.body.appendChild(script);
};

/**
 * Opens the main Files.app's window and waits until it is ready.
 *
 * @param {!Array<string>} urls URLs to be opened.
 * @param {function(string)} callback Completion callback with the new window's
 *     App ID.
 */
test.util.async.openVideoPlayer = function(urls, callback) {
  openVideoPlayerWindow(urls).then(callback);
};

// Register the test utils.
test.util.registerRemoteTestUtils();
