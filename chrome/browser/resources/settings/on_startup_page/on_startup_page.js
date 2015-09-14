// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'cr-settings-on-startup-page' is a settings page.
 *
 * Example:
 *
 *    <neon-animated-pages>
 *      <cr-settings-on-startup-page prefs="{{prefs}}">
 *      </cr-settings-on-startup-page>
 *      ... other pages ...
 *    </neon-animated-pages>
 *
 * @group Chrome Settings Elements
 * @element cr-settings-on-startup-page
 */
Polymer({
  is: 'cr-settings-on-startup-page',

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * The current active route.
     */
    currentRoute: {
      type: Object,
      notify: true,
    },

    prefValue: {
      type: Number,
    }
  },

  observers: [
    'prefValueChanged(prefValue)',
  ],

  prefValueChanged: function(prefValue) {
    this.set('prefs.session.restore_on_startup.value', parseInt(prefValue));
  },

  /** @private */
  onBackTap_: function() {
    this.$.pages.back();
  },

  /** @private */
  onSetPagesTap_: function() {
    this.$.pages.setSubpageChain(['startup-urls']);
  },
});
