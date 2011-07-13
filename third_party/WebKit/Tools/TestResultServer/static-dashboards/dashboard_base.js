/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @fileoverview Base JS file for pages that want to parse the results JSON
 * from the testing bots. This deals with generic utility functions, visible
 * history, popups and appending the script elements for the JSON files.
 *
 * The calling page is expected to implement the following "abstract"
 * functions/objects:
 */
var pageLoadStartTime = Date.now();

/**
 * Generates the contents of the dashboard. The page should override this with
 * a function that generates the page assuming all resources have loaded.
 */
function generatePage() {
}

/**
 * Takes a key and a value and sets the currentState[key] = value iff key is
 * a valid hash parameter and the value is a valid value for that key.
 *
 * @return {boolean} Whether the key what inserted into the currentState.
 */
function handleValidHashParameter(key, value) {
  return false;
}

/**
 * Default hash parameters for the page. The page should override this to create
 * default states.
 */
var defaultStateValues = {};


/**
 * The page should override this to modify page state due to
 * changing query parameters.
 * @param {Object} params New or modified query params as key: value.
 * @return {boolean} Whether changing this parameter should cause generatePage to be called.
 */
function handleQueryParameterChange(params) {
  return true;
}

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////
var GTEST_EXPECTATIONS_MAP_ = {
  'P': 'PASS',
  'F': 'FAIL',
  'L': 'FLAKY',
  'N': 'NO DATA',
  'X': 'DISABLED'
};

var LAYOUT_TEST_EXPECTATIONS_MAP_ = {
  'P': 'PASS',
  'N': 'NO DATA',
  'X': 'SKIP',
  'T': 'TIMEOUT',
  'F': 'TEXT',
  'C': 'CRASH',
  'I': 'IMAGE',
  'Z': 'IMAGE+TEXT',
  // We used to glob a bunch of expectations into "O" as OTHER. Expectations
  // are more precise now though and it just means MISSING.
  'O': 'MISSING'
};

var FAILURE_EXPECTATIONS_ = {
  'T': 1,
  'F': 1,
  'C': 1,
  'I': 1,
  'Z': 1
};

// Keys in the JSON files.
var WONTFIX_COUNTS_KEY = 'wontfixCounts';
var FIXABLE_COUNTS_KEY = 'fixableCounts';
var DEFERRED_COUNTS_KEY = 'deferredCounts';
var WONTFIX_DESCRIPTION = 'Tests never to be fixed (WONTFIX)';
var FIXABLE_DESCRIPTION = 'All tests for this release';
var DEFERRED_DESCRIPTION = 'All deferred tests (DEFER)';
var FIXABLE_COUNT_KEY = 'fixableCount';
var ALL_FIXABLE_COUNT_KEY = 'allFixableCount';
var CHROME_REVISIONS_KEY = 'chromeRevision';
var WEBKIT_REVISIONS_KEY = 'webkitRevision';
var TIMESTAMPS_KEY = 'secondsSinceEpoch';
var BUILD_NUMBERS_KEY = 'buildNumbers';
var TESTS_KEY = 'tests';

// These should match the testtype uploaded to test-results.appspot.com.
// See http://test-results.appspot.com/testfile.
var TEST_TYPES = ['app_unittests', 'base_unittests', 'browser_tests',
    'cache_invalidation_unittests', 'courgette_unittests',
    'crypto_unittests', 'googleurl_unittests', 'gpu_unittests',
    'installer_util_unittests', 'interactive_ui_tests', 'ipc_tests',
    'jingle_unittests', 'layout-tests', 'media_unittests',
    'mini_installer_test', 'nacl_ui_tests', 'net_unittests',
    'printing_unittests', 'remoting_unittests', 'safe_browsing_tests',
    'sync_unit_tests', 'sync_integration_tests',
    'test_shell_tests', 'ui_tests', 'unit_tests'];

var RELOAD_REQUIRING_PARAMETERS = ['showAllRuns', 'group', 'testType'];

// Enum for indexing into the run-length encoded results in the JSON files.
// 0 is where the count is length is stored. 1 is the value.
var RLE = {
  LENGTH: 0,
  VALUE: 1
}

// The server that hosts the builder results json files.
var TEST_RESULTS_SERVER = 'http://test-results.appspot.com/';

/**
 * @return {boolean} Whether the value represents a failing result.
 */
function isFailingResult(value) {
  return 'FSTOCIZ'.indexOf(value) != -1;
}

/**
 * Takes a key and a value and sets the currentState[key] = value iff key is
 * a valid hash parameter and the value is a valid value for that key. Handles
 * cross-dashboard parameters then falls back to calling
 * handleValidHashParameter for dashboard-specific parameters.
 *
 * @return {boolean} Whether the key what inserted into the currentState.
 */
function handleValidHashParameterWrapper(key, value) {
  switch(key) {
    case 'testType':
      validateParameter(currentState, key, value,
          function() {
            return TEST_TYPES.indexOf(value) != -1;
          });
      return true;

    case 'group':
      validateParameter(currentState, key, value,
          function() {
            return value in LAYOUT_TESTS_BUILDER_GROUPS;
          });
      return true;

    // FIXME: remove support for this parameter once the waterfall starts to
    // pass in the builder name instead.
    case 'master':
      validateParameter(currentState, key, value,
          function() {
            return value in LEGACY_BUILDER_MASTERS_TO_GROUPS;
          });
      return true;

    case 'builder':
      validateParameter(currentState, key, value,
          function() {
            return value in builders;
          });
      return true;

    case 'useTestData':
    case 'showAllRuns':
      currentState[key] = value == 'true';
      return true;

    case 'buildDir':
      currentState['testType'] = 'layout-test-results';
      if (value === 'Debug' || value == 'Release') {
        currentState[key] = value;
        return true;
      } else {
        return false;
      }

    default:
      return handleValidHashParameter(key, value);
  }
}

var defaultCrossDashboardStateValues = {
  group: '@DEPS - chromium.org',
  showAllRuns: false,
  testType: 'layout-tests',
  buildDir : ''
}

// Generic utility functions.
function $(id) {
  return document.getElementById(id);
}

function stringContains(a, b) {
  return a.indexOf(b) != -1;
}

function caseInsensitiveContains(a, b) {
  return a.match(new RegExp(b, 'i'));
}

function startsWith(a, b) {
  return a.indexOf(b) == 0;
}

function endsWith(a, b) {
  return a.lastIndexOf(b) == a.length - b.length;
}

function isValidName(str) {
  return str.match(/[A-Za-z0-9\-\_,]/);
}

function trimString(str) {
  return str.replace(/^\s+|\s+$/g, '');
}

function request(url, success, error, opt_isBinaryData) {
  console.log('XMLHttpRequest: ' + url);
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  if (opt_isBinaryData)
    xhr.overrideMimeType('text/plain; charset=x-user-defined');
  xhr.onreadystatechange = function(e) {
    if (xhr.readyState == 4) {
      if (xhr.status == 200)
        success(xhr);
      else
        error(xhr);
    }
  }
  xhr.send();
}

function validateParameter(state, key, value, validateFn) {
  if (validateFn()) {
    state[key] = value;
  } else {
    console.log(key + ' value is not valid: ' + value);
  }
}

/**
 * Parses window.location.hash and set the currentState values appropriately.
 */
function parseParameters(parameterStr) {
  oldState = currentState;
  currentState = {};

  var hash = window.location.hash;
  var paramsList = hash ? hash.substring(1).split('&') : [];
  var paramsMap = {};
  var invalidKeys = [];
  for (var i = 0; i < paramsList.length; i++) {
    var thisParam = paramsList[i].split('=');
    if (thisParam.length != 2) {
      console.log('Invalid query parameter: ' + paramsList[i]);
      continue;
    }

    paramsMap[thisParam[0]] = decodeURIComponent(thisParam[1]);
  }

  function parseParam(key) {
    var value = paramsMap[key];
    if (!handleValidHashParameterWrapper(key, value))
      invalidKeys.push(key + '=' + value);
  }

  // Parse builder param last, since the list of valid builders depends on the
  // other parameters.
  for (var key in paramsMap) {
    if (key != 'builder')
      parseParam(key);
  }
  if ('builder' in paramsMap) {
    if (!builders) {
      var tempState = {};
      for (var key in currentState) {
        tempState[key] = currentState[key];
      }
      fillMissingValues(tempState, defaultCrossDashboardStateValues);
      fillMissingValues(tempState, defaultStateValues);
      initBuilders(tempState);
    }
    parseParam('builder');
  }

  if (invalidKeys.length)
    console.log("Invalid query parameters: " + invalidKeys.join(','));

  var diffState = diffStates();

  fillMissingValues(currentState, defaultCrossDashboardStateValues);
  fillMissingValues(currentState, defaultStateValues);

  // Some parameters require loading different JSON files when the value changes. Do a reload.
  if (oldState) {
    for (var key in currentState) {
      if (oldState[key] != currentState[key] && RELOAD_REQUIRING_PARAMETERS.indexOf(key) != -1)
        window.location.reload();
    }
  }

  return diffState;
}

/**
 * Find the difference of currentState with oldState.
 * @return {Object} key:values of the new or changed params.
 */
function diffStates() {
  // If there is no old state, everything in the current state is new.
  if (!oldState) {
    return currentState;
  }

  var changedParams = {};
  for (curKey in currentState) {
    var oldVal = oldState[curKey];
    var newVal = currentState[curKey];
    // Add new keys or changed values.
    if (!oldVal || oldVal != newVal) {
      changedParams[curKey] = newVal;
    }
  }
  return changedParams;
}

function getDefaultValue(key) {
  if (key in defaultStateValues)
    return defaultStateValues[key];
  return defaultCrossDashboardStateValues[key];
}

function fillMissingValues(to, from) {
  for (var state in from) {
    if (!(state in to))
      to[state] = from[state];
  }
}

/**
 * Load a script.
 * @param {string} path Path to the script to load.
 * @param {Function=} opt_onError Optional function to call if the script
 *     does not load.
 * @param {Function=} opt_onLoad Optional function to call when the script
 *     is loaded.  Called with the script element as its 1st argument.
 */
function appendScript(path, opt_onError, opt_onLoad) {
  var script = document.createElement('script');
  script.src = path;
  if (opt_onLoad) {
    script.onreadystatechange = function() {
      if (this.readyState == 'complete') opt_onLoad(script);
    };
    script.onload = function() { opt_onLoad(script); };
  }
  if (opt_onError) {
    script.onerror = opt_onError;
  }
  document.getElementsByTagName('head')[0].appendChild(script);
}

// Permalinkable state of the page.
var currentState;

// Saved value of previous currentState. This is used to detect changing from
// one set of builders to another, which requires reloading the page.
var oldState;

// Parse cross-dashboard parameters before using them.
parseParameters();

function isLayoutTestResults() {
  return currentState.testType == 'layout-tests';
}

function getCurrentBuilderGroup(opt_state) {
  var state = opt_state || currentState;
  switch (state.testType) {
    case 'layout-tests':
      return LAYOUT_TESTS_BUILDER_GROUPS[state.group]
      break;
    case 'app_unittests':
    case 'base_unittests':
    case 'browser_tests':
    case 'cacheinvalidation_unittests':
    case 'courgette_unittests':
    case 'crypto_unittests':
    case 'googleurl_unittests':
    case 'gpu_unittests':
    case 'installer_util_unittests':
    case 'interactive_ui_tests':
    case 'ipc_tests':
    case 'jingle_unittests':
    case 'media_unittests':
    case 'mini_installer_test':
    case 'nacl_ui_tests':
    case 'net_unittests':
    case 'printing_unittests':
    case 'remoting_unittests':
    case 'safe_browsing_tests':
    case 'sync_unit_tests':
    case 'sync_integration_tests':
    case 'test_shell_tests':
    case 'ui_tests':
    case 'unit_tests':
      return G_TESTS_BUILDER_GROUP;
      break;
    default:
      console.log('invalid testType parameter: ' + state.testType);
  }
}

function getBuilderMaster(builderName) {
  return BUILDER_TO_MASTER[builderName];
}

function isTipOfTreeWebKitBuilder() {
  return getCurrentBuilderGroup().isToTWebKit;
}

var defaultBuilderName, builders, expectationsBuilder;
function initBuilders(state) {
  if (state.buildDir) {
    // If buildDir is set, point to the results.json and expectations.json in the
    // local tree. Useful for debugging changes to the python JSON generator.
    defaultBuilderName = 'DUMMY_BUILDER_NAME';
    builders = {'DUMMY_BUILDER_NAME': ''};
    var loc = document.location.toString();
    var offset = loc.indexOf('webkit/');
  } else {
    // FIXME: remove support for mapping from the master parameter to the group
    // one once the waterfall starts to pass in the builder name instead.
    if (state.master) {
      state.group = LEGACY_BUILDER_MASTERS_TO_GROUPS[state.master];
      window.location.hash = window.location.hash.replace('master=' + state.master, 'group=' + state.group);
      delete state.master;
    }
    getCurrentBuilderGroup(state).setup();
  }
}
initBuilders(currentState);

// Append JSON script elements.
var resultsByBuilder = {};
var expectationsByTest = {};
function ADD_RESULTS(builds) {
  for (var builderName in builds) {
    if (builderName != 'version')
      resultsByBuilder[builderName] = builds[builderName];
  }

  handleResourceLoad();
}

function getPathToBuilderResultsFile(builderName) {
  return TEST_RESULTS_SERVER + 'testfile?builder=' + builderName +
      '&master=' + getBuilderMaster(builderName).name +
      '&testtype=' + currentState.testType + '&name=';
}

// Only webkit tests have a sense of test expectations.
var waitingOnExpectations = isLayoutTestResults() && !isTreeMap();
if (waitingOnExpectations) {
  function ADD_EXPECTATIONS(expectations) {
    waitingOnExpectations = false;
    expectationsByTest = expectations;
    handleResourceLoad();
  }
}

function isTreeMap() {
  return endsWith(window.location.pathname, 'treemap.html');
}

function appendJSONScriptElementFor(builderName) {
  var resultsFilename;
  if (isTreeMap())
    resultsFilename = 'times_ms.json';
  else if (currentState.showAllRuns)
    resultsFilename = 'results.json';
  else
    resultsFilename = 'results-small.json';

  appendScript(getPathToBuilderResultsFile(builderName) + resultsFilename,
      partial(handleResourceLoadError, builderName),
      partial(handleScriptLoaded, builderName));
}

function appendJSONScriptElements() {
  clearErrors();

  if (isTreeMap())
    return;

  parseParameters();

  if (currentState.useTestData) {
    appendScript('flakiness_dashboard_tests.js');
    return;
  }

  for (var builderName in builders) {
    appendJSONScriptElementFor(builderName);
  }

  // Grab expectations file from the fastest builder, which is Linux release
  // right now.  Could be changed to any other builder if needed.
  if (waitingOnExpectations) {
    appendScript(getPathToBuilderResultsFile(expectationsBuilder) +
        'expectations.json');
  }
}

var hasDoneInitialPageGeneration = false;
// String of error messages to display to the user.
var errorMessages = '';

function handleResourceLoad() {
  // In case we load a results.json that's not in the list of builders,
  // make sure to only call handleLocationChange once from the resource loads.
  if (!hasDoneInitialPageGeneration)
    handleLocationChange();
}

function handleScriptLoaded(builderName, script) {
  // We need this work-around for webkit.org/b/50589.
  if (!resultsByBuilder[builderName]) {
    var error = new Error("Builder data was empty");
    error.target = script;
    handleResourceLoadError(builderName, error);
  }
}

/**
 * Handle resource loading errors - 404s, 500s, etc.  Recover from the error to
 * still show as much data as possible, but show some UI about the failure, and
 * do not try using this resource again until user refreshes.
 *
 * @param {string} builderName Name of builder that the resource failed for.
 * @param {Event} e The error event.
 */
function handleResourceLoadError(builderName, e) {
  var error = e.target.src + ' failed to load for ' + builderName + '.';

  if (isLayoutTestResults()) {
    console.error(error);
    addError(error);
  } else {
    // Avoid to show error/warning messages for non-layout tests. We may be
    // checking the builders that are not running the tests.
    console.info('info:' + error);
  }

  // Remove this builder from builders, so we don't try to use the
  // data that isn't there.
  delete builders[builderName];

  // Change the default builder name if it has been deleted.
  if (defaultBuilderName == builderName) {
    defaultBuilderName = null;
    for (var availableBuilderName in builders) {
      defaultBuilderName = availableBuilderName;
      defaultStateValues.builder = availableBuilderName;
      break;
    }
    if (!defaultBuilderName) {
      var error = 'No tests results found for ' + currentState.testType +
          '. Reload the page to try fetching it again.';
      console.error(error);
      addError(error);
    }
  }

  // Proceed as if the resource had loaded.
  handleResourceLoad();
}


/**
 * Record a new error message.
 * @param {string} errorMsg The message to show to the user.
 */
function addError(errorMsg) {
  errorMessages += errorMsg + '<br />';
}

/**
 * Clear out error and warning messages.
 */
function clearErrors() {
  errorMessages = '';
}

/**
 * If there are errors, show big and red UI for errors so as to be noticed.
 */
function showErrors() {
  var errors = $('errors');

  if (!errorMessages) {
    if (errors)
      errors.parentNode.removeChild(errors);
    return;
  }

  if (!errors) {
    errors = document.createElement('H2');
    errors.style.color = 'red';
    errors.id = 'errors';
    document.body.appendChild(errors);
  }

  errors.innerHTML = errorMessages;
}

/**
 * @return {boolean} Whether the json files have all completed loading.
 */
function haveJsonFilesLoaded() {
  if (waitingOnExpectations)
    return false;

  if (isTreeMap())
    return true;

  for (var builder in builders) {
    if (!resultsByBuilder[builder])
      return false;
  }
  return true;
}

function handleLocationChange() {
  if(!haveJsonFilesLoaded()) {
    return;
  }

  hasDoneInitialPageGeneration = true;

  var params = parseParameters();
  var shouldGeneratePage = true;
  if (Object.keys(params).length)
    shouldGeneratePage = handleQueryParameterChange(params);

  var newHash = getPermaLinkURLHash();
  var winHash = window.location.hash || "#";
  // Make sure the location is the same as the state we are using internally.
  // These get out of sync if processQueryParamChange changed state.
  if (newHash != winHash) {
    // This will cause another hashchange, and when we loop
    // back through here next time, we'll go through generatePage.
    window.location.hash = newHash;
  } else if (shouldGeneratePage) {
    generatePage();
  }
}

window.onhashchange = handleLocationChange;

/**
 * Sets the page state. Takes varargs of key, value pairs.
 */
function setQueryParameter(var_args) {
  var state = Object.create(currentState);
  for (var i = 0; i < arguments.length; i += 2) {
    var key = arguments[i];
    state[key] = arguments[i + 1];
  }
  // Note: We use window.location.hash rather that window.location.replace
  // because of bugs in Chrome where extra entries were getting created
  // when back button was pressed and full page navigation was occuring.
  // TODO: file those bugs.
  window.location.hash = getPermaLinkURLHash(state);
}

function getPermaLinkURLHash(opt_state) {
  var state = opt_state || currentState;
  return '#' + joinParameters(state);
}

function joinParameters(stateObject) {
  var state = [];
  for (var key in stateObject) {
    var value = stateObject[key];
    if (value != getDefaultValue(key))
      state.push(key + '=' + encodeURIComponent(value));
  }
  return state.join('&');
}

function logTime(msg, startTime) {
  console.log(msg + ': ' + (Date.now() - startTime));
}

function hidePopup() {
  var popup = $('popup');
  if (popup)
    popup.parentNode.removeChild(popup);
}

function showPopup(e, html) {
  var popup = $('popup');
  if (!popup) {
    popup = document.createElement('div');
    popup.id = 'popup';
    document.body.appendChild(popup);
  }

  // Set html first so that we can get accurate size metrics on the popup.
  popup.innerHTML = html;

  var targetRect = e.target.getBoundingClientRect();

  var x = Math.min(targetRect.left - 10,
      document.documentElement.clientWidth - popup.offsetWidth);
  x = Math.max(0, x);
  popup.style.left = x + document.body.scrollLeft + 'px';

  var y = targetRect.top + targetRect.height;
  if (y + popup.offsetHeight > document.documentElement.clientHeight) {
    y = targetRect.top - popup.offsetHeight;
  }
  y = Math.max(0, y);
  popup.style.top = y + document.body.scrollTop + 'px';
}

/**
 * Create a new function with some of its arguements
 * pre-filled.
 * Taken from goog.partial in the Closure library.
 * @param {Function} fn A function to partially apply.
 * @param {...*} var_args Additional arguments that are partially
 *     applied to fn.
 * @return {!Function} A partially-applied form of the function bind() was
 *     invoked as a method of.
 */
function partial(fn, var_args) {
  var args = Array.prototype.slice.call(arguments, 1);
  return function() {
    // Prepend the bound arguments to the current arguments.
    var newArgs = Array.prototype.slice.call(arguments);
    newArgs.unshift.apply(newArgs, args);
    return fn.apply(this, newArgs);
  };
};

/**
 * Returns the keys of the object/map/hash.
 * Taken from goog.object.getKeys in the Closure library.
 *
 * @param {Object} obj The object from which to get the keys.
 * @return {!Array.<string>} Array of property keys.
 */
function getKeys(obj) {
  var res = [];
  for (var key in obj) {
    res.push(key);
  }
  return res;
}

/**
 * Returns the appropriate expectatiosn map for the current testType.
 */
function getExpectationsMap() {
  return isLayoutTestResults() ? LAYOUT_TEST_EXPECTATIONS_MAP_ :
      GTEST_EXPECTATIONS_MAP_;
}

function toggleQueryParameter(param) {
  setQueryParameter(param, !currentState[param]);
}

function checkboxHTML(queryParameter, label, isChecked, opt_extraJavaScript) {
  var js = opt_extraJavaScript || '';
  return '<label style="padding-left: 2em">' +
      '<input type="checkbox" onchange="toggleQueryParameter(\'' + queryParameter + '\');' + js + '" ' +
          (isChecked ? 'checked' : '') + '>' + label +
      '</label>';
}

function selectHTML(label, queryParameter, options) {
  var html = '<label style="padding-left: 2em">' +
      label + ': ' +
      '<select onchange="setQueryParameter(\'' +
          queryParameter + '\', this[this.selectedIndex].value)">';

  for (var i = 0; i < options.length; i++) {
    var value = options[i];
    html += '<option value="' + value + '" ' +
      (currentState[queryParameter] == value ? 'selected' : '') +
      '>' + value + '</option>'
  }
  html += '</select><label>';
  return html;
}

/**
 * Returns the HTML for the select element to switch to different testTypes.
 */
function getHTMLForTestTypeSwitcher(opt_noBuilderMenu, opt_extraHtml, opt_includeNoneBuilder) {
  var html = '<div style="border-bottom:1px dashed">';
  html += '' +
      htmlForDashboardLink('Stats', 'aggregate_results.html') +
      htmlForDashboardLink('Timeline', 'timeline_explorer.html') +
      htmlForDashboardLink('Results', 'flakiness_dashboard.html') +
      htmlForDashboardLink('Treemap', 'treemap.html');

  html += selectHTML('Test type', 'testType', TEST_TYPES);

  if (!opt_noBuilderMenu) {
    var buildersForMenu = Object.keys(builders);
    if (opt_includeNoneBuilder)
      buildersForMenu.unshift('--------------');
    html += selectHTML('Builder', 'builder', buildersForMenu);
  }

  if (isLayoutTestResults()) {
    html += selectHTML('Group', 'group', getKeys(LAYOUT_TESTS_BUILDER_GROUPS));
  }

  if (!isTreeMap()) {
    html += checkboxHTML('showAllRuns', 'Show all runs', currentState.showAllRuns);
  }

  if (opt_extraHtml) {
    html += opt_extraHtml;
  }
  return html + '</div>';
}

function selectBuilder(builder) {
  setQueryParameter('builder', builder);
}

function loadDashboard(fileName) {
  var pathName = window.location.pathname;
  pathName = pathName.substring(0, pathName.lastIndexOf('/') + 1);
  window.location = pathName + fileName + window.location.hash;
}

function htmlForTopLink(html, onClick, isSelected) {
  var cssText = isSelected ? 'font-weight: bold;' :
      'color:blue;text-decoration:underline;cursor:pointer;';
  cssText += 'margin: 0 5px;';
  return '<span style="' + cssText +
      '" onclick="' + onClick + '">' + html + '</span>';
}

function htmlForDashboardLink(html, fileName) {
  var pathName = window.location.pathname;
  var currentFileName = pathName.substring(pathName.lastIndexOf('/') + 1);
  var isSelected = currentFileName == fileName;
  var onClick = 'loadDashboard(\'' + fileName + '\')';
  return htmlForTopLink(html, onClick, isSelected);
}

function getRevisionLink(results, index, key, singleUrlTemplate, rangeUrlTemplate) {
  var currentRevision = parseInt(results[key][index], 10);
  var previousRevision = parseInt(results[key][index + 1], 10);

  function getSingleUrl() {
    return singleUrlTemplate.replace('<rev>', currentRevision);
  }

  function getRangeUrl() {
    return rangeUrlTemplate.replace('<rev1>', currentRevision).
        replace('<rev2>', previousRevision + 1);
  }

  if (currentRevision == previousRevision) {
    return 'At <a href="' + getSingleUrl() + '">r' + currentRevision  + '</a>';
  } else if (currentRevision - previousRevision == 1) {
    return '<a href="' + getSingleUrl() + '">r' + currentRevision  + '</a>';
  } else {
    return '<a href="' + getRangeUrl() + '">r' +
        (previousRevision + 1) + ' to r' +
        currentRevision + '</a>';
  }
}

function getChromiumRevisionLink(results, index) {
  return getRevisionLink(
      results,
      index,
      CHROME_REVISIONS_KEY,
      'http://src.chromium.org/viewvc/chrome?view=rev&revision=<rev>',
      'http://build.chromium.org/f/chromium/perf/dashboard/ui/changelog.html?url=/trunk/src&range=<rev2>:<rev1>&mode=html');
}

function getWebKitRevisionLink(results, index) {
  return getRevisionLink(
      results,
      index,
      WEBKIT_REVISIONS_KEY,
      'http://trac.webkit.org/changeset/<rev>',
      'http://trac.webkit.org/log/trunk/?rev=<rev1>&stop_rev=<rev2>&limit=100&verbose=on');
}

/**
 * "Decompresses" the RLE-encoding of test results so that we can query it
 * by build index and test name.
 *
 * @param {Object} results results for the current builder
 * @return Object with these properties:
 *   - testNames: array mapping test index to test names.
 *   - resultsByBuild: array of builds, for each build a (sparse) array of test
 *                     results by test index.
 *   - flakyTests: array with the boolean value true at test indices that are
 *                 considered flaky (more than one single-build failure).
 *   - flakyDeltasByBuild: array of builds, for each build a count of flaky
 *                         test results by expectation, as well as a total.
 */
function getTestResultsByBuild(builderResults) {
  var builderTestResults = builderResults[TESTS_KEY];
  var buildCount = builderResults[FIXABLE_COUNTS_KEY].length;
  var testResultsByBuild = new Array(buildCount);
  var flakyDeltasByBuild = new Array(buildCount);

  // Pre-sizing the test result arrays for each build saves us ~250ms
  var testCount = 0;
  for (var testName in builderTestResults) {
    testCount++;
  }
  for (var i = 0; i < buildCount; i++) {
    testResultsByBuild[i] = new Array(testCount);
    testResultsByBuild[i][testCount - 1] = undefined;

    flakyDeltasByBuild[i] = {};
  }

  // Using indices instead of the full test names for each build saves us
  // ~1500ms
  var testIndex = 0;
  var testNames = new Array(testCount);
  var flakyTests = new Array(testCount);

  // Decompress and "invert" test results (by build instead of by test) and
  // determine which are flaky.
  for (var testName in builderTestResults) {
    var oneBuildFailureCount = 0;

    testNames[testIndex] = testName;
    var testResults = builderTestResults[testName].results;
    for (var i = 0, rleResult, currentBuildIndex = 0;
         (rleResult = testResults[i]) && currentBuildIndex < buildCount;
         i++) {
      var count = rleResult[RLE.LENGTH];
      var value = rleResult[RLE.VALUE];

      if (count == 1 && value in FAILURE_EXPECTATIONS_) {
        oneBuildFailureCount++;
      }

      for (var j = 0; j < count; j++) {
        testResultsByBuild[currentBuildIndex++][testIndex] = value;
        if (currentBuildIndex == buildCount) {
          break;
        }
      }
    }

    if (oneBuildFailureCount > 2) {
      flakyTests[testIndex] = true;
    }

    testIndex++;
  }

  // Now that we know which tests are flaky, count the test results that are
  // from flaky tests for each build.
  testIndex = 0;
  for (var testName in builderTestResults) {
    if (!flakyTests[testIndex++]) {
      continue;
    }

    var testResults = builderTestResults[testName].results;
    for (var i = 0, rleResult, currentBuildIndex = 0;
        (rleResult = testResults[i]) && currentBuildIndex < buildCount;
        i++) {
      var count = rleResult[RLE.LENGTH];
      var value = rleResult[RLE.VALUE];

      for (var j = 0; j < count; j++) {
        var buildTestResults = flakyDeltasByBuild[currentBuildIndex++];
        function addFlakyDelta(key) {
          if (!(key in buildTestResults)) {
            buildTestResults[key] = 0;
          }
          buildTestResults[key]++;
        }
        addFlakyDelta(value);
        if (value != 'P' && value != 'N') {
          addFlakyDelta('total');
        }
        if (currentBuildIndex == buildCount) {
          break;
        }
      }
    }
  }

  return {
    testNames: testNames,
    resultsByBuild: testResultsByBuild,
    flakyTests: flakyTests,
    flakyDeltasByBuild: flakyDeltasByBuild
  };
}


appendJSONScriptElements();

document.addEventListener('mousedown', function(e) {
      // Clear the open popup, unless the click was inside the popup.
      var popup = $('popup');
      if (popup && e.target != popup &&
          !(popup.compareDocumentPosition(e.target) & 16)) {
        hidePopup();
      }
    }, false);

window.addEventListener('load', function() {
      // This doesn't seem totally accurate as there is a race between
      // onload firing and the last script tag being executed.
      logTime('Time to load JS', pageLoadStartTime);
    }, false);
