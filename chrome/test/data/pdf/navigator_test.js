// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {eventToPromise} from 'chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/_test_resources/webui/test_util.m.js';
import {NavigatorDelegate, PdfNavigator, WindowOpenDisposition} from 'chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/navigator.js';
import {OpenPdfParamsParser} from 'chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/open_pdf_params_parser.js';
import {PDFScriptingAPI} from 'chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/pdf_scripting_api.js';

import {getZoomableViewport, MockDocumentDimensions, MockElement, MockSizer, MockViewportChangedCallback, testAsync} from './test_util.js';

/** @implements {NavigatorDelegate} */
class MockNavigatorDelegate {
  constructor() {
    this.navigateInCurrentTabCalled = false;
    this.navigateInNewTabCalled = false;
    this.navigateInNewWindowCalled = false;
    this.url = undefined;
  }

  /** @override */
  navigateInCurrentTab(url) {
    this.navigateInCurrentTabCalled = true;
    this.url = url || '<called, but no url set>';
  }

  /** @override */
  navigateInNewTab(url) {
    this.navigateInNewTabCalled = true;
    this.url = url || '<called, but no url set>';
  }

  /** @override */
  navigateInNewWindow(url) {
    this.navigateInNewWindowCalled = true;
    this.url = url || '<called, but no url set>';
  }

  reset() {
    this.navigateInCurrentTabCalled = false;
    this.navigateInNewTabCalled = false;
    this.navigateInNewWindowCalled = false;
    this.url = undefined;
  }
}

/**
 * Given a |navigator|, navigate to |url| in the current tab, a new tab, or
 * a new window depending on the value of |disposition|. Use
 * |viewportChangedCallback| and |navigatorDelegate| to check the callbacks,
 * and that the navigation to |expectedResultUrl| happened.
 * @param {!PdfNavigator} navigator
 * @param {string} url
 * @param {!WindowOpenDisposition} disposition
 * @param {(string|undefined)} expectedResultUrl
 * @param {!MockViewportChangedCallback} viewportChangedCallback
 * @param {!MockNavigatorDelegate} navigatorDelegate
 */
function doNavigationUrlTest(
    navigator, url, disposition, expectedResultUrl, viewportChangedCallback,
    navigatorDelegate) {
  viewportChangedCallback.reset();
  navigatorDelegate.reset();
  navigator.navigate(url, disposition);
  chrome.test.assertFalse(viewportChangedCallback.wasCalled);
  chrome.test.assertEq(expectedResultUrl, navigatorDelegate.url);
  if (expectedResultUrl === undefined) {
    return;
  }
  switch (disposition) {
    case WindowOpenDisposition.CURRENT_TAB:
      chrome.test.assertTrue(navigatorDelegate.navigateInCurrentTabCalled);
      break;
    case WindowOpenDisposition.NEW_BACKGROUND_TAB:
      chrome.test.assertTrue(navigatorDelegate.navigateInNewTabCalled);
      break;
    case WindowOpenDisposition.NEW_WINDOW:
      chrome.test.assertTrue(navigatorDelegate.navigateInNewWindowCalled);
      break;
    default:
      break;
  }
}

/**
 * Helper function to run doNavigationUrlTest() for the current tab, a new
 * tab, and a new window.
 * @param {string} originalUrl
 * @param {string} url
 * @param {(string|undefined)} expectedResultUrl
 */
function doNavigationUrlTests(originalUrl, url, expectedResultUrl) {
  const mockWindow = new MockElement(100, 100, null);
  const mockSizer = new MockSizer();
  const mockViewportChangedCallback = new MockViewportChangedCallback();
  const viewport = getZoomableViewport(mockWindow, mockSizer, 0, 1, 0);
  viewport.setViewportChangedCallback(mockViewportChangedCallback.callback);

  const paramsParser = new OpenPdfParamsParser(function(name) {
    return Promise.resolve(
        {messageId: 'getNamedDestination_1', pageNumber: -1});
  });

  const navigatorDelegate = new MockNavigatorDelegate();
  const navigator =
      new PdfNavigator(originalUrl, viewport, paramsParser, navigatorDelegate);

  doNavigationUrlTest(
      navigator, url, WindowOpenDisposition.CURRENT_TAB, expectedResultUrl,
      mockViewportChangedCallback, navigatorDelegate);
  doNavigationUrlTest(
      navigator, url, WindowOpenDisposition.NEW_BACKGROUND_TAB,
      expectedResultUrl, mockViewportChangedCallback, navigatorDelegate);
  doNavigationUrlTest(
      navigator, url, WindowOpenDisposition.NEW_WINDOW, expectedResultUrl,
      mockViewportChangedCallback, navigatorDelegate);
}

const tests = [
  /**
   * Test navigation within the page, opening a url in the same tab and
   * opening a url in a new tab.
   */
  function testNavigate() {
    const mockWindow = new MockElement(100, 100, null);
    const mockSizer = new MockSizer();
    const mockCallback = new MockViewportChangedCallback();
    const viewport = getZoomableViewport(mockWindow, mockSizer, 0, 1, 0);
    viewport.setViewportChangedCallback(mockCallback.callback);

    const paramsParser = new OpenPdfParamsParser(function(destination) {
      if (destination === 'US') {
        return Promise.resolve(
            {messageId: 'getNamedDestination_1', pageNumber: 0});
      } else if (destination === 'UY') {
        return Promise.resolve(
            {messageId: 'getNamedDestination_2', pageNumber: 2});
      } else {
        return Promise.resolve(
            {messageId: 'getNamedDestination_3', pageNumber: -1});
      }
    });
    const url = 'http://xyz.pdf';

    const navigatorDelegate = new MockNavigatorDelegate();
    const navigator =
        new PdfNavigator(url, viewport, paramsParser, navigatorDelegate);

    const documentDimensions = new MockDocumentDimensions();
    documentDimensions.addPage(100, 100);
    documentDimensions.addPage(200, 200);
    documentDimensions.addPage(100, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    testAsync(async () => {
      mockCallback.reset();
      let navigatingDone =
          eventToPromise('navigate-for-testing', navigator.getEventTarget());
      // This should move viewport to page 0.
      navigator.navigate(url + '#US', WindowOpenDisposition.CURRENT_TAB);
      await navigatingDone;
      chrome.test.assertTrue(mockCallback.wasCalled);
      chrome.test.assertEq(0, viewport.position.x);
      chrome.test.assertEq(0, viewport.position.y);

      mockCallback.reset();
      navigatorDelegate.reset();
      navigatingDone =
          eventToPromise('navigate-for-testing', navigator.getEventTarget());
      // This should open "http://xyz.pdf#US" in a new tab. So current tab
      // viewport should not update and viewport position should remain same.
      navigator.navigate(url + '#US', WindowOpenDisposition.NEW_BACKGROUND_TAB);
      await navigatingDone;
      chrome.test.assertFalse(mockCallback.wasCalled);
      chrome.test.assertTrue(navigatorDelegate.navigateInNewTabCalled);
      chrome.test.assertEq(0, viewport.position.x);
      chrome.test.assertEq(0, viewport.position.y);

      mockCallback.reset();
      navigatingDone =
          eventToPromise('navigate-for-testing', navigator.getEventTarget());
      // This should move viewport to page 2.
      navigator.navigate(url + '#UY', WindowOpenDisposition.CURRENT_TAB);
      await navigatingDone;
      chrome.test.assertTrue(mockCallback.wasCalled);
      chrome.test.assertEq(0, viewport.position.x);
      chrome.test.assertEq(300, viewport.position.y);

      mockCallback.reset();
      navigatorDelegate.reset();
      navigatingDone =
          eventToPromise('navigate-for-testing', navigator.getEventTarget());
      // #ABC is not a named destination in the page so viewport should not
      // update, and the viewport position should remain same as testNavigate3's
      // navigating results, as this link will open in the same tab.
      navigator.navigate(url + '#ABC', WindowOpenDisposition.CURRENT_TAB);
      await navigatingDone;
      chrome.test.assertFalse(mockCallback.wasCalled);
      chrome.test.assertTrue(navigatorDelegate.navigateInCurrentTabCalled);
      chrome.test.assertEq(0, viewport.position.x);
      chrome.test.assertEq(300, viewport.position.y);
    });
  },
  /**
   * Test opening a url in the same tab, in a new tab, and in a new window for
   * a http:// url as the current location. The destination url may not have
   * a valid scheme, so the navigator must determine the url by following
   * similar heuristics as Adobe Acrobat Reader.
   */
  function testNavigateForLinksWithoutScheme() {
    const url = 'http://www.example.com/subdir/xyz.pdf';

    // Sanity check.
    doNavigationUrlTests(
        url, 'https://www.foo.com/bar.pdf', 'https://www.foo.com/bar.pdf');

    // Open relative links.
    doNavigationUrlTests(
        url, 'foo/bar.pdf', 'http://www.example.com/subdir/foo/bar.pdf');
    doNavigationUrlTests(
        url, 'foo.com/bar.pdf',
        'http://www.example.com/subdir/foo.com/bar.pdf');
    doNavigationUrlTests(
        url, '../www.foo.com/bar.pdf',
        'http://www.example.com/www.foo.com/bar.pdf');

    // Open an absolute link.
    doNavigationUrlTests(
        url, '/foodotcom/bar.pdf', 'http://www.example.com/foodotcom/bar.pdf');

    // Open a http url without a scheme.
    doNavigationUrlTests(
        url, 'www.foo.com/bar.pdf', 'http://www.foo.com/bar.pdf');

    // Test three dots.
    doNavigationUrlTests(
        url, '.../bar.pdf', 'http://www.example.com/subdir/.../bar.pdf');

    // Test forward slashes.
    doNavigationUrlTests(url, '..\\bar.pdf', 'http://www.example.com/bar.pdf');
    doNavigationUrlTests(
        url, '.\\bar.pdf', 'http://www.example.com/subdir/bar.pdf');
    doNavigationUrlTests(
        url, '\\bar.pdf', 'http://www.example.com/subdir//bar.pdf');

    // Regression test for https://crbug.com/569040
    doNavigationUrlTests(
        url, 'http://something.else/foo#page=5',
        'http://something.else/foo#page=5');

    chrome.test.succeed();
  },
  /**
   * Test opening a url in the same tab, in a new tab, and in a new window with
   * a file:/// url as the current location.
   */
  function testNavigateFromLocalFile() {
    const url = 'file:///some/path/to/myfile.pdf';

    // Open an absolute link.
    doNavigationUrlTests(
        url, '/foodotcom/bar.pdf', 'file:///foodotcom/bar.pdf');

    chrome.test.succeed();
  },

  function testNavigateInvalidUrls() {
    const url = 'https://example.com/some-web-document.pdf';

    // From non-file: to file:
    doNavigationUrlTests(url, 'file:///bar.pdf', undefined);

    doNavigationUrlTests(url, 'chrome://version', undefined);

    doNavigationUrlTests(
        url, 'javascript:// this is not a document.pdf', undefined);

    doNavigationUrlTests(
        url, 'this-is-not-a-valid-scheme://path.pdf', undefined);

    doNavigationUrlTests(url, '', undefined);

    chrome.test.succeed();
  }
];

const scriptingAPI = new PDFScriptingAPI(window, window);
scriptingAPI.setLoadCompleteCallback(function() {
  chrome.test.runTests(tests);
});
