// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var pass = chrome.test.callbackPass;

var width = 0;
var height = 0;

var deltaWidth = 20;
var deltaHeight = 30;

function checkRestoreWithBounds(theWindow) {
  chrome.test.assertEq('normal', theWindow.state);
  chrome.test.assertEq(width, theWindow.width);
  chrome.test.assertEq(height, theWindow.height);
  chrome.windows.remove(theWindow.id, pass());
}

function checkMaximized(theWindow) {
  if (theWindow.type == 'panel') {
    // Maximize is the same as restore for panels.
    chrome.test.assertEq('normal', theWindow.state);
    chrome.test.assertEq(width, theWindow.width);
    chrome.test.assertEq(height, theWindow.height);
  } else {
    chrome.test.assertEq('maximized', theWindow.state);
    chrome.test.assertTrue(width < theWindow.width ||
                           height < theWindow.height);
  }

  width += deltaWidth;
  height += deltaHeight;
  chrome.windows.update(theWindow.id,
      {'state': 'normal', 'width': width, 'height': height},
      pass(checkRestoreWithBounds));
}

function checkRestored(theWindow) {
  chrome.test.assertEq('normal', theWindow.state);
  chrome.test.assertEq(width, theWindow.width);
  chrome.test.assertEq(height, theWindow.height);

  chrome.windows.update(theWindow.id, {'state': 'maximized'}, pass(checkMaximized));
}

function checkMinimized(theWindow) {
  chrome.test.assertEq('minimized', theWindow.state);
  chrome.windows.update(theWindow.id, {'state': 'normal'}, pass(checkRestored));
}

function minimizeWindow(theWindow) {
  chrome.test.assertEq('normal', theWindow.state);
  width = theWindow.width;
  height = theWindow.height;
  chrome.windows.update(theWindow.id, {'state': 'minimized'}, pass(checkMinimized));
}

function testWindowState(windowType) {
  // Specifying size prevents 'panel' windows from computing size asynchronously. It ensures
  // panel sizes stay fixed through the test.
  chrome.windows.create({'url': 'hello.html', 'type': windowType, 'width':200, 'height':300 },
    pass(minimizeWindow));
}

chrome.test.runTests([
  function changeWindowState() {
    testWindowState('normal');
  },
  function changePopupWindowState() {
    testWindowState('popup');
  },
  function changePanelWindowState() {
    testWindowState('panel');
  }
]);
