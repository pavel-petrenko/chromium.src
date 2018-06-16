// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Namespace for the Camera app.
 */
var camera = camera || {};

/**
 * Creates the Camera App main object.
 * @constructor
 */
camera.Camera = function() {
  /**
   * @type {camera.Camera.Context}
   * @private
   */
  this.context_ = new camera.Camera.Context(
      this.onAspectRatio_.bind(this),
      this.onError_.bind(this),
      this.onErrorRecovered_.bind(this));

  /**
   * @type {Array.<camera.ViewsStack>}
   * @private
   */
  this.viewsStack_ = new camera.Camera.ViewsStack();

  /**
   * @type {camera.Router}
   * @private
   */
  this.router_ = new camera.Router(
      this.navigateById_.bind(this),
      this.viewsStack_.pop.bind(this.viewsStack_));

  /**
   * @type {camera.views.Camera}
   * @private
   */
  this.cameraView_ = new camera.views.Camera(this.context_, this.router_);

  /**
   * @type {camera.views.Album}
   * @private
   */
  this.albumView_ = new camera.views.Album(this.context_, this.router_);

  /**
   * @type {camera.views.Browser}
   * @private
   */
  this.browserView_ = new camera.views.Browser(this.context_, this.router_);

  /**
   * @type {camera.views.Dialog}
   * @private
   */
  this.dialogView_ = new camera.views.Dialog(this.context_, this.router_);

  /**
   * @type {?number}
   * @private
   */
  this.resizeCompleteTimer_ = null;

  /**
   * @type {camera.util.TooltipManager}
   * @private
   */
  this.tooltipManager_ = new camera.util.TooltipManager();

  // End of properties. Seal the object.
  Object.seal(this);

  // Handle key presses to make the Camera app accessible via the keyboard.
  document.body.addEventListener('keydown', this.onKeyPressed_.bind(this));

  // Handle window resize.
  window.addEventListener('resize', this.onWindowResize_.bind(this));

  // Animate the error icon.
  document.querySelector('#error .icon').addEventListener('mousedown',
      this.onErrorIconClicked_.bind(this));
  document.querySelector('#error .icon').addEventListener('touchstart',
      this.onErrorIconClicked_.bind(this));

  // Set the localized window title.
  document.title = chrome.i18n.getMessage('name');
};

/**
 * Creates context for the views.
 *
 * @param {function(number)} onAspectRatio Callback to be called, when setting a
 *     new aspect ratio. The argument is the aspect ratio.
 * @param {function(string, string, opt_string)} onError Callback to be called,
 *     when an error occurs. Arguments: identifier, first line, second line.
 * @param {function(string)} onErrorRecovered Callback to be called,
 *     when the error goes away. The argument is the error id.
 * @constructor
 */
camera.Camera.Context = function(onAspectRatio, onError, onErrorRecovered) {
  camera.View.Context.call(this);

  /**
   * @type {boolean}
   */
  this.hasError = false;

  /**
   * @type {function(number)}
   */
  this.onAspectRatio = onAspectRatio;

  /**
   * @type {function(string, string, string)}
   */
  this.onError = onError;

  /**
   * @type {function(string)}
   */
  this.onErrorRecovered = onErrorRecovered;

  // End of properties. Seal the object.
  Object.seal(this);
};

camera.Camera.Context.prototype = {
  __proto__: camera.View.Context.prototype
};

/**
 * Creates a stack of views.
 * @constructor
 */
camera.Camera.ViewsStack = function() {
  /**
   * Stack with the views as well as return callbacks.
   * @type {Array.<Object>}
   * @private
   */
  this.stack_ = [];

  // No more properties. Seal the object.
  Object.seal(this);
};

camera.Camera.ViewsStack.prototype = {
  get current() {
    return this.stack_.length ? this.stack_[this.stack_.length - 1].view : null;
  },
  get all() {
    return this.stack_.map(entry => entry.view);
  }
};

/**
 * Adds the view on the stack and hence makes it the current one. Optionally,
 * passes the arguments to the view.
 *
 * @param {camera.View} view View to be pushed on top of the stack.
 * @param {Object=} opt_arguments Optional arguments.
 * @param {function(Object=)} opt_callback Optional result callback to be called
 *     when the view is closed.
 */
camera.Camera.ViewsStack.prototype.push = function(
    view, opt_arguments, opt_callback) {
  if (this.current)
    this.current.inactivate();

  this.stack_.push({
    view: view,
    callback: opt_callback || function(result) {}
  });

  view.enter(opt_arguments);
  view.activate();
};

/**
 * Removes the current view from the stack and hence makes the previous one
 * the current one. Calls the callback passed to the previous view's navigate()
 * method with the result.
 *
 * @param {Object=} opt_result Optional result. If not passed, then undefined
 *     will be passed to the callback.
 */
camera.Camera.ViewsStack.prototype.pop = function(opt_result) {
  var entry = this.stack_.pop();
  entry.view.inactivate();
  entry.view.leave();

  if (this.current)
    this.current.activate();
  if (entry.callback)
    entry.callback(opt_result);
};

camera.Camera.prototype = {
  get cameraView() {
    return this.cameraView_;
  }
};

/**
 * Starts the app by initializing views and showing the camera view.
 */
camera.Camera.prototype.start = function() {
  var initAllViews = function() {
    // Initialize all views, and then start the app.
    Promise.all([
        new Promise(this.cameraView_.initialize.bind(this.cameraView_)),
        new Promise(this.albumView_.initialize.bind(this.albumView_)),
        new Promise(this.browserView_.initialize.bind(this.browserView_))])
      .then(() => {
        this.tooltipManager_.initialize();
        this.viewsStack_.push(this.cameraView_);
        camera.util.makeElementsUnfocusableByMouse();
        camera.util.setAriaAttributes();
      }).catch(error => {
        console.error('Failed to initialize the Camera app.', error);
        this.onError_('view-failure', 'App initialize failed.');
      });
  }.bind(this);

  camera.models.FileSystem.initialize(function() {
    var onFailure = function() {
      this.onError_('migration-failure', 'Migration failed.');
    }.bind(this);

    var promptMigrate = function() {
      this.router_.navigate(camera.Router.ViewIdentifier.DIALOG, {
        type: camera.views.Dialog.Type.ALERT,
        message: 'Start migration'
      }, function(result) {
        if (!result.isPositive) {
          chrome.app.window.current().close();
          return;
        }
        camera.models.FileSystem.migratePictures(initAllViews, onFailure);
      });
    }.bind(this);

    camera.models.FileSystem.needMigration(function() {
      // Show the prompt dialog for pictures migration.
      if (!camera.models.FileSystem.ackMigratePictures) {
        promptMigrate();
      } else {
        camera.models.FileSystem.migratePictures(initAllViews, onFailure);
      }
    }, function() {
      initAllViews();
    }, function(error) {
      this.onError_('filesystem-failure', error);
    }.bind(this));
  }.bind(this), function(error) {
    this.onError_('filesystem-failure', error);
  }.bind(this));
};

/**
 * Switches the view using a router's view identifier.
 *
 * @param {camera.Router.ViewIdentifier} viewIdentifier View identifier.
 * @param {Object=} opt_arguments Optional arguments for the view.
 * @param {function(Object=)} opt_callback Optional result callback to be called
 *     when the view is closed.
 * @private
 */
camera.Camera.prototype.navigateById_ = function(
    viewIdentifier, opt_arguments, opt_callback) {
  switch (viewIdentifier) {
    case camera.Router.ViewIdentifier.CAMERA:
      this.viewsStack_.push(this.cameraView_, opt_arguments, opt_callback);
      break;
    case camera.Router.ViewIdentifier.ALBUM:
      this.viewsStack_.push(this.albumView_, opt_arguments, opt_callback);
      break;
    case camera.Router.ViewIdentifier.BROWSER:
      this.viewsStack_.push(this.browserView_, opt_arguments, opt_callback);
      break;
    case camera.Router.ViewIdentifier.DIALOG:
      this.viewsStack_.push(this.dialogView_, opt_arguments, opt_callback);
      break;
  }
};

/**
 * Sets the window size to match the last known aspect ratio.
 * @private
 */
camera.Camera.prototype.updateWindowSize_ = function() {
  var appWindow = chrome.app.window.current();
  if (appWindow.isMaximized() || appWindow.isFullscreen()) {
    return;
  }

  // Keep the width fixed and calculate the height by the aspect ratio.
  var inner = appWindow.innerBounds;
  var innerW = inner.minWidth;
  var innerH = Math.round(innerW / appWindow.aspectRatio);

  // Limit window resizing capability by setting min-height. Don't limit
  // max-height here as it may disable maximize/fullscreen capabilities.
  // TODO(yuli): Revise if there is an alternative fix.
  inner.minHeight = innerH;

  inner.width = innerW;
  inner.height = innerH;
};

/**
 * Handles resizing of the window.
 * @private
 */
camera.Camera.prototype.onWindowResize_ = function() {
  if (this.resizeCompleteTimer_) {
    clearTimeout(this.resizeCompleteTimer_);
    this.resizeCompleteTimer_ = null;
  }
  this.resizeCompleteTimer_ = setTimeout(function() {
    this.resizeCompleteTimer_ = null;
    this.updateWindowSize_();
  }.bind(this), 500);

  // Resize all stacked views rather than just the current-view to avoid
  // camera-preview not being resized if a dialog or settings' menu is shown on
  // top of the camera-view.
  this.viewsStack_.all.forEach(function(view) {
    view.onResize();
  });
};

/**
 * Handles clicking on the error icon.
 * @private
 */
camera.Camera.prototype.onErrorIconClicked_ = function() {
  var icon = document.querySelector('#error .icon');
  icon.classList.remove('animate');
  setTimeout(function() {
    icon.classList.add('animate');
  }, 0);
};

/**
 * Handles pressed keys.
 * @param {Event} event Key press event.
 * @private
 */
camera.Camera.prototype.onKeyPressed_ = function(event) {
  if (this.context_.hasError)
    return;

  var currentView = this.viewsStack_.current;
  if (currentView)
    currentView.onKeyPressed(event);
};

/**
 * Updates the window apsect ratio.
 * @param {number} aspectRatio Aspect ratio of window's inner-bounds.
 * @private
 */
camera.Camera.prototype.onAspectRatio_ = function(aspectRatio) {
  chrome.app.window.current().aspectRatio = aspectRatio;
  this.updateWindowSize_();
};

/**
 * Shows an error message.
 * @param {string} identifier Identifier of the error.
 * @param {string} message Message for the error.
 * @param {string=} opt_hint Optional hint for the error message.
 * @private
 */
camera.Camera.prototype.onError_ = function(identifier, message, opt_hint) {
  document.body.classList.add('has-error');
  this.context_.hasError = true;
  document.querySelector('#error-msg').textContent = message;
  document.querySelector('#error-msg-hint').textContent = opt_hint || '';
};

/**
 * Removes the error message when an error goes away.
 * @param {string} identifier Identifier of the error.
 * @private
 */
camera.Camera.prototype.onErrorRecovered_ = function(identifier) {
  // TODO(mtomasz): Implement identifiers handling in case of multiple
  // error messages at once.
  this.context_.hasError = false;
  document.body.classList.remove('has-error');
};

/**
 * @type {camera.Camera} Singleton of the Camera object.
 * @private
 */
camera.Camera.instance_ = null;

/**
 * Returns the singleton instance of the Camera class.
 * @return {camera.Camera} Camera object.
 */
camera.Camera.getInstance = function() {
  if (!camera.Camera.instance_)
    camera.Camera.instance_ = new camera.Camera();
  return camera.Camera.instance_;
};

/**
 * Creates the Camera object and starts screen capturing.
 */
document.addEventListener('DOMContentLoaded', function() {
  camera.Camera.getInstance().start();
  chrome.app.window.current().show();
});

