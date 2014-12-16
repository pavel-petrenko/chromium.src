// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Stub out the metrics package.
 * @type {!Object.<!string, !Function>}
 */
var metrics = {
  recordTime: function() {},
  recordValue: function() {}
};

/** @type {!importer.DefaultMediaScanner} */
var scanner;

/** @type {!importer.TestImportHistory} */
var importHistory;

// Set up the test components.
function setUp() {

  importHistory = new importer.TestImportHistory();
  scanner = new importer.DefaultMediaScanner(importHistory);
}

/**
 * Creates a subdirectory within a temporary file system for testing.
 *
 * @param {string} directoryName Name of the test directory to create.  Must be
 *     unique within this test suite.
 */
function makeTestFileSystemRoot(directoryName) {
  function makeTestFilesystem() {
    return new Promise(function(resolve, reject) {
      window.webkitRequestFileSystem(
          window.TEMPORARY,
          1024 * 1024,
          resolve,
          reject);
    });
  }

  return makeTestFilesystem()
      .then(
          // Create a directory, pretend that's the root.
          function(fs) {
            return new Promise(function(resolve, reject) {
              fs.root.getDirectory(
                    directoryName,
                    {
                      create: true,
                      exclusive: true
                    },
                  resolve,
                  reject);
            });
          });
}

/**
 * Creates a set of files in the given directory.
 * @param {!Array<!Array|string>} filenames A (potentially nested) array of
 *     strings, reflecting a directory structure.
 * @param {!DirectoryEntry} dir The root of the directory tree.
 * @return {!Promise.<!DirectoryEntry>} The root of the newly populated
 *     directory tree.
 */
function populateDir(filenames, dir) {
  return Promise.all(
      filenames.map(function(filename) {
        if (filename instanceof Array) {
          return new Promise(function(resolve, reject) {
            dir.getDirectory(filename[0], {create: true}, resolve, reject);
          }).then(populateDir.bind(null, filename));
        } else {
          return new Promise(function(resolve, reject) {
            dir.getFile(filename, {create: true}, resolve, reject);
          });
        }
      })).then(function() { return dir; });
}

/**
 * Verifies that scanning an empty filesystem produces an empty list.
 */
function testEmptySourceList() {
  assertThrows(
    function() {
      scanner.scan([]);
    });
}

/**
 * Verifies that scanning a simple single-level directory structure works.
 */
function testEmptyScanResults(callback) {
  var filenames = [
    'happy',
    'thoughts'
  ];
  reportPromise(
      makeTestFileSystemRoot('testEmptyScanResults')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scan([root]).whenFinished();
              })
          .then(assertResults.bind(null, [])),
      callback);
}

/**
 * Verifies that scanning a simple single-level directory structure works.
 */
function testSingleLevel(callback) {
  var filenames = [
    'foo',
    'foo.jpg',
    'bar.gif',
    'baz.avi',
    'foo.mp3',
    'bar.txt'
  ];
  var expectedFiles = [
    '/testSingleLevel/foo.jpg',
    '/testSingleLevel/bar.gif',
    '/testSingleLevel/baz.avi'
  ];
  reportPromise(
      makeTestFileSystemRoot('testSingleLevel')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scan([root]).whenFinished();
              })
          .then(assertResults.bind(null, expectedFiles)),
      callback);
}

/**
 * Verifies that scanning ignores previously imported entries.
 */
function testIgnoresPreviousImports(callback) {
  importHistory.importedPaths[
      '/testIgnoresPreviousImports/oldimage1234.jpg'] =
          [importer.Destination.GOOGLE_DRIVE];
  var filenames = [
    'oldimage1234.jpg',
    'foo.jpg',
    'bar.gif',
    'baz.avi'
  ];
  var expectedFiles = [
    '/testIgnoresPreviousImports/foo.jpg',
    '/testIgnoresPreviousImports/bar.gif',
    '/testIgnoresPreviousImports/baz.avi'
  ];
  reportPromise(
      makeTestFileSystemRoot('testIgnoresPreviousImports')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scan([root]).whenFinished();
              })
          .then(assertResults.bind(null, expectedFiles)),
      callback);
}

function testMultiLevel(callback) {
  var filenames = [
    'foo.jpg',
    'bar',
    [
      'foo.0',
      'bar.0.jpg'
    ],
    [
      'foo.1',
      'bar.1.gif',
      [
        'foo.1.0',
        'bar.1.0.avi'
      ]
    ]
  ];
  var expectedFiles = [
    '/testMultiLevel/foo.jpg',
    '/testMultiLevel/foo.0/bar.0.jpg',
    '/testMultiLevel/foo.1/bar.1.gif',
    '/testMultiLevel/foo.1/foo.1.0/bar.1.0.avi'
  ];

  reportPromise(
      makeTestFileSystemRoot('testMultiLevel')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scan([root]).whenFinished();
              })
          .then(assertResults.bind(null, expectedFiles)),
      callback);
}

function testMultipleDirectories(callback) {
  var filenames = [
    'foo',
    'bar',
    [
      'foo.0',
      'bar.0.jpg'
    ],
    [
      'foo.1',
      'bar.1.jpg',
    ]
  ];
  // Expected file paths from the scan.  We're scanning the two subdirectories
  // only.
  var expectedFiles = [
    '/testMultipleDirectories/foo.0/bar.0.jpg',
    '/testMultipleDirectories/foo.1/bar.1.jpg'
  ];

  var getDirectory = function(root, dirname) {
    return new Promise(function(resolve, reject) {
      root.getDirectory(
          dirname, {create: false}, resolve, reject);
    });
  };

  reportPromise(
      makeTestFileSystemRoot('testMultipleDirectories')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directories.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return Promise.all(['foo.0', 'foo.1'].map(
                    getDirectory.bind(null, root))).then(
                        function(directories) {
                          return scanner.scan(directories).whenFinished();
                        });
              })
          .then(assertResults.bind(null, expectedFiles)),
      callback);
}

/**
 * Verifies the results of the media scan are as expected.
 * @param {!impoter.ScanResults} results
 */
function assertResults(expected, results) {
  assertFileEntryPathsEqual(expected, results.getFileEntries());
  // TODO(smckay): Add coverage for getScanDurationMs && getTotalBytes.
}
