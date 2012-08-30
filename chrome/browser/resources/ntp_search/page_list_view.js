// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview PageListView implementation.
 * PageListView manages page list, dot list, switcher buttons and handles apps
 * pages callbacks from backend.
 *
 * Note that you need to have AppLauncherHandler in your WebUI to use this code.
 */

cr.define('ntp', function() {
  'use strict';

  /**
   * Creates a PageListView object.
   * @constructor
   * @extends {Object}
   */
  function PageListView() {
  }

  PageListView.prototype = {
    /**
     * The CardSlider object to use for changing app pages.
     * @type {CardSlider|undefined}
     */
    cardSlider: undefined,

    /**
     * The frame div for this.cardSlider.
     * @type {!Element|undefined}
     */
    sliderFrame: undefined,

    /**
     * The 'page-list' element.
     * @type {!Element|undefined}
     */
    pageList: undefined,

    /**
     * A list of all 'tile-page' elements.
     * @type {!NodeList|undefined}
     */
    tilePages: undefined,

    /**
     * A list of all 'apps-page' elements.
     * @type {!NodeList|undefined}
     */
    appsPages: undefined,

    /**
     * The Suggestions page.
     * @type {!Element|undefined}
     */
    suggestionsPage: undefined,

    /**
     * The Most Visited page.
     * @type {!Element|undefined}
     */
    mostVisitedPage: undefined,

    /**
     * The Recently Closed page.
     * @type {!Element|undefined}
     */
    recentlyClosedPage: undefined,

    /**
     * The 'dots-list' element.
     * @type {!Element|undefined}
     */
    dotList: undefined,

    /**
     * The left and right paging buttons.
     * @type {!Element|undefined}
     */
    pageSwitcherStart: undefined,
    pageSwitcherEnd: undefined,

    /**
     * The 'trash' element.  Note that technically this is unnecessary,
     * JavaScript creates the object for us based on the id.  But I don't want
     * to rely on the ID being the same, and JSCompiler doesn't know about it.
     * @type {!Element|undefined}
     */
    trash: undefined,

    /**
     * The type of page that is currently shown. The value is a numerical ID.
     * @type {number}
     */
    shownPage: 0,

    /**
     * The index of the page that is currently shown, within the page type.
     * For example if the third Apps page is showing, this will be 2.
     * @type {number}
     */
    shownPageIndex: 0,

    /**
     * EventTracker for managing event listeners for page events.
     * @type {!EventTracker}
     */
    eventTracker: new EventTracker,

    /**
     * If non-null, this is the ID of the app to highlight to the user the next
     * time getAppsCallback runs. "Highlight" in this case means to switch to
     * the page and run the new tile animation.
     * @type {?string}
     */
    highlightAppId: null,

    /**
     * Initializes page list view.
     * @param {!Element} pageList A DIV element to host all pages.
     * @param {!Element} dotList An UL element to host nav dots. Each dot
     *     represents a page.
     * @param {!Element} cardSliderFrame The card slider frame that hosts
     *     pageList and switcher buttons.
     * @param {!Element|undefined} opt_trash Optional trash element.
     * @param {!Element|undefined} opt_pageSwitcherStart Optional start page
     *     switcher button.
     * @param {!Element|undefined} opt_pageSwitcherEnd Optional end page
     *     switcher button.
     */
    initialize: function(pageList, dotList, cardSliderFrame, opt_trash,
                         opt_pageSwitcherStart, opt_pageSwitcherEnd) {
      this.pageList = pageList;

      this.dotList = dotList;
      cr.ui.decorate(this.dotList, ntp.DotList);

      this.trash = opt_trash;
      if (this.trash)
        new ntp.Trash(this.trash);

      this.pageSwitcherStart = opt_pageSwitcherStart;
      if (this.pageSwitcherStart)
        ntp.initializePageSwitcher(this.pageSwitcherStart);

      this.pageSwitcherEnd = opt_pageSwitcherEnd;
      if (this.pageSwitcherEnd)
        ntp.initializePageSwitcher(this.pageSwitcherEnd);

      this.shownPage = loadTimeData.getInteger('shown_page_type');
      this.shownPageIndex = loadTimeData.getInteger('shown_page_index');

      if (loadTimeData.getBoolean('showApps')) {
        // Request data on the apps so we can fill them in.
        // Note that this is kicked off asynchronously.  'getAppsCallback' will
        // be invoked at some point after this function returns.
        chrome.send('getApps');
      } else if (this.shownPage == loadTimeData.getInteger('apps_page_id')) {
        // No apps page.
        this.setShownPage_(
            loadTimeData.getInteger('most_visited_page_id'), 0);
      }

      document.addEventListener('keydown', this.onDocKeyDown_.bind(this));
      // Prevent touch events from triggering any sort of native scrolling.
      document.addEventListener('touchmove', function(e) {
        e.preventDefault();
      }, true);

      this.tilePages = this.pageList.getElementsByClassName('tile-page');
      this.appsPages = this.pageList.getElementsByClassName('apps-page');

      // Initialize the cardSlider without any cards at the moment.
      this.sliderFrame = cardSliderFrame;
      this.cardSlider = new cr.ui.CardSlider(this.sliderFrame, this.pageList,
          this.sliderFrame.offsetWidth);

      // Handle mousewheel events anywhere in the card slider, so that wheel
      // events on the page switchers will still scroll the page.
      // This listener must be added before the card slider is initialized,
      // because it needs to be called before the card slider's handler.
      var cardSlider = this.cardSlider;
      cardSliderFrame.addEventListener('mousewheel', function(e) {
        if (cardSlider.currentCardValue.handleMouseWheel(e)) {
          e.preventDefault();  // Prevent default scroll behavior.
          e.stopImmediatePropagation();  // Prevent horizontal card flipping.
        }
      });

      this.cardSlider.initialize(
          loadTimeData.getBoolean('isSwipeTrackingFromScrollEventsEnabled'));

      // Handle events from the card slider.
      this.pageList.addEventListener('cardSlider:card_changed',
                                     this.onCardChanged_.bind(this));
      this.pageList.addEventListener('cardSlider:card_added',
                                     this.onCardAdded_.bind(this));
      this.pageList.addEventListener('cardSlider:card_removed',
                                     this.onCardRemoved_.bind(this));

      // Ensure the slider is resized appropriately with the window.
      window.addEventListener('resize', this.onWindowResize_.bind(this));

      // Update apps when online state changes.
      window.addEventListener('online',
          this.updateOfflineEnabledApps_.bind(this));
      window.addEventListener('offline',
          this.updateOfflineEnabledApps_.bind(this));
    },

    /**
     * Appends a tile page.
     *
     * @param {TilePage} page The page element.
     * @param {string} title The title of the tile page.
     * @param {TilePage} opt_refNode Optional reference node to insert in front
     *     of.
     * When opt_refNode is falsey, |page| will just be appended to the end of
     * the page list.
     */
    appendTilePage: function(page, title, opt_refNode) {
      if (opt_refNode) {
        var refIndex = this.getTilePageIndex(opt_refNode);
        this.cardSlider.addCardAtIndex(page, refIndex);
      } else {
        this.cardSlider.appendCard(page);
      }

      // Remember special MostVisitedPage.
      if (typeof ntp.MostVisitedPage != 'undefined' &&
          page instanceof ntp.MostVisitedPage) {
        assert(this.tilePages.length == 1,
               'MostVisitedPage should be added as first tile page');
        this.mostVisitedPage = page;
      }

      if (typeof ntp.RecentlyClosedPage != 'undefined' &&
          page instanceof ntp.RecentlyClosedPage) {
        this.recentlyClosedPage = page;
      }

      if (typeof ntp.SuggestionsPage != 'undefined' &&
          page instanceof ntp.SuggestionsPage) {
        this.suggestionsPage = page;
      }

      // Make a deep copy of the dot template to add a new one.
      var newDot = new ntp.NavDot(page, title);
      page.navigationDot = newDot;
      this.dotList.insertBefore(newDot,
                                opt_refNode ? opt_refNode.navigationDot : null);
      // Set a tab index on the first dot.
      if (this.dotList.dots.length == 1)
        newDot.tabIndex = 3;
    },

    /**
     * Called by chrome when an app has changed positions.
     * @param {Object} appData The data for the app. This contains page and
     *     position indices.
     */
    appMoved: function(appData) {
      assert(loadTimeData.getBoolean('showApps'));

      var app = $(appData.id);
      assert(app, 'trying to move an app that doesn\'t exist');
      app.remove(false);

      this.appsPages[appData.page_index].insertApp(appData, false);
    },

    /**
     * Called by chrome when an existing app has been disabled or
     * removed/uninstalled from chrome.
     * @param {Object} appData A data structure full of relevant information for
     *     the app.
     * @param {boolean} isUninstall True if the app is being uninstalled;
     *     false if the app is being disabled.
     * @param {boolean} fromPage True if the removal was from the current page.
     */
    appRemoved: function(appData, isUninstall, fromPage) {
      assert(loadTimeData.getBoolean('showApps'));

      var app = $(appData.id);
      assert(app, 'trying to remove an app that doesn\'t exist');

      if (!isUninstall)
        app.replaceAppData(appData);
      else
        app.remove(!!fromPage);
    },

    /**
     * @return {boolean} If the page is still starting up.
     * @private
     */
    isStartingUp_: function() {
      return document.documentElement.classList.contains('starting-up');
    },

    /**
     * Tracks whether apps have been loaded at least once.
     * @type {boolean}
     * @private
     */
    appsLoaded_: false,

    /**
     * Callback invoked by chrome with the apps available.
     *
     * Note that calls to this function can occur at any time, not just in
     * response to a getApps request. For example, when a user
     * installs/uninstalls an app on another synchronized devices.
     * @param {Object} data An object with all the data on available
     *        applications.
     */
    getAppsCallback: function(data) {
      assert(loadTimeData.getBoolean('showApps'));

      var startTime = Date.now();

      // Remember this to select the correct card when done rebuilding.
      var prevCurrentCard = this.cardSlider.currentCard;

      // Make removal of pages and dots as quick as possible with less DOM
      // operations, reflows, or repaints. We set currentCard = 0 and remove
      // from the end to not encounter any auto-magic card selections in the
      // process and we hide the card slider throughout.
      this.cardSlider.currentCard = 0;

      // Clear any existing apps pages and dots.
      // TODO(rbyers): It might be nice to preserve animation of dots after an
      // uninstall. Could we re-use the existing page and dot elements?  It
      // seems unfortunate to have Chrome send us the entire apps list after an
      // uninstall.
      while (this.appsPages.length > 0)
        this.removeTilePageAndDot_(this.appsPages[this.appsPages.length - 1]);

      // Get the array of apps and add any special synthesized entries
      var apps = data.apps;

      // Get a list of page names
      var pageNames = data.appPageNames;

      function stringListIsEmpty(list) {
        for (var i = 0; i < list.length; i++) {
          if (list[i])
            return false;
        }
        return true;
      }

      // Sort by launch ordinal
      apps.sort(function(a, b) {
        return a.app_launch_ordinal > b.app_launch_ordinal ? 1 :
          a.app_launch_ordinal < b.app_launch_ordinal ? -1 : 0;
      });

      // An app to animate (in case it was just installed).
      var highlightApp;

      // If there are any pages after the apps, add new pages before them.
      var lastAppsPage = (this.appsPages.length > 0) ?
          this.appsPages[this.appsPages.length - 1] : null;
      var lastAppsPageIndex = (lastAppsPage != null) ?
          Array.prototype.indexOf.call(this.tilePages, lastAppsPage) : -1;
      var nextPageAfterApps = lastAppsPageIndex != -1 ?
          this.tilePages[lastAppsPageIndex + 1] : null;

      // Add the apps, creating pages as necessary
      for (var i = 0; i < apps.length; i++) {
        var app = apps[i];
        var pageIndex = app.page_index || 0;
        while (pageIndex >= this.appsPages.length) {
          var pageName = loadTimeData.getString('appDefaultPageName');
          if (this.appsPages.length < pageNames.length)
            pageName = pageNames[this.appsPages.length];

          var origPageCount = this.appsPages.length;
          this.appendTilePage(new ntp.AppsPage(), pageName, nextPageAfterApps);
          // Confirm that appsPages is a live object, updated when a new page is
          // added (otherwise we'd have an infinite loop)
          assert(this.appsPages.length == origPageCount + 1,
                 'expected new page');
        }

        if (app.id == this.highlightAppId)
          highlightApp = app;
        else
          this.appsPages[pageIndex].insertApp(app, false);
      }

      this.cardSlider.currentCard = prevCurrentCard;

      if (highlightApp)
        this.appAdded(highlightApp, true);

      logEvent('apps.layout: ' + (Date.now() - startTime));

      // Tell the slider about the pages and mark the current page.
      this.updateSliderCards();
      this.cardSlider.currentCardValue.navigationDot.classList.add('selected');

      if (!this.appsLoaded_) {
        this.appsLoaded_ = true;
        cr.dispatchSimpleEvent(document, 'sectionready', true, true);
      }
    },

    /**
     * Called by chrome when a new app has been added to chrome or has been
     * enabled if previously disabled.
     * @param {Object} appData A data structure full of relevant information for
     *     the app.
     * @param {boolean=} opt_highlight Whether the app about to be added should
     *     be highlighted.
     */
    appAdded: function(appData, opt_highlight) {
      assert(loadTimeData.getBoolean('showApps'));

      if (appData.id == this.highlightAppId) {
        opt_highlight = true;
        this.highlightAppId = null;
      }

      var pageIndex = appData.page_index || 0;

      if (pageIndex >= this.appsPages.length) {
        while (pageIndex >= this.appsPages.length) {
          this.appendTilePage(new ntp.AppsPage(),
                              loadTimeData.getString('appDefaultPageName'));
        }
        this.updateSliderCards();
      }

      var page = this.appsPages[pageIndex];
      var app = $(appData.id);
      if (app) {
        app.replaceAppData(appData);
      } else if (opt_highlight) {
        page.insertAndHighlightApp(appData);
        this.setShownPage_(loadTimeData.getInteger('apps_page_id'),
                           appData.page_index);
      } else {
        page.insertApp(appData, false);
      }
    },

    /**
     * Callback invoked by chrome whenever an app preference changes.
     * @param {Object} data An object with all the data on available
     *     applications.
     */
    appsPrefChangedCallback: function(data) {
      assert(loadTimeData.getBoolean('showApps'));

      for (var i = 0; i < data.apps.length; ++i) {
        $(data.apps[i].id).appData = data.apps[i];
      }
    },

    /**
     * Invoked whenever the pages in apps-page-list have changed so that
     * the Slider knows about the new elements.
     */
    updateSliderCards: function() {
      var pageNo = Math.max(0, Math.min(this.cardSlider.currentCard,
                                        this.tilePages.length - 1));
      this.cardSlider.setCards(Array.prototype.slice.call(this.tilePages),
                               pageNo);
      switch (this.shownPage) {
        case loadTimeData.getInteger('apps_page_id'):
          this.cardSlider.selectCardByValue(
              this.appsPages[Math.min(this.shownPageIndex,
                                      this.appsPages.length - 1)]);
          break;
        case loadTimeData.getInteger('most_visited_page_id'):
          if (this.mostVisitedPage)
            this.cardSlider.selectCardByValue(this.mostVisitedPage);
          break;
        case loadTimeData.getInteger('recently_closed_page_id'):
          if (this.recentlyClosedPage)
            this.cardSlider.selectCardByValue(this.recentlyClosedPage);
          break;
        case loadTimeData.getInteger('suggestions_page_id'):
          if (this.suggestionsPage)
            this.cardSlider.selectCardByValue(this.suggestionsPage);
          break;
      }
    },

    /**
     * Adjusts the size and position of the page switchers according to the
     * layout of the current card. TODO(pedrosimonetti): Delete.
     */
    updatePageSwitchers: function() {
    },

    /**
     * Returns the index of the given apps page.
     * @param {AppsPage} page The AppsPage we wish to find.
     * @return {number} The index of |page| or -1 if it is not in the
     *    collection.
     */
    getAppsPageIndex: function(page) {
      return Array.prototype.indexOf.call(this.appsPages, page);
    },

    /**
     * Handler for cardSlider:card_changed events from this.cardSlider.
     * @param {Event} e The cardSlider:card_changed event.
     * @private
     */
    onCardChanged_: function(e) {
      var page = e.cardSlider.currentCardValue;

      // Don't change shownPage until startup is done (and page changes actually
      // reflect user actions).
      if (!this.isStartingUp_()) {
        if (page.classList.contains('apps-page')) {
          this.setShownPage_(loadTimeData.getInteger('apps_page_id'),
                             this.getAppsPageIndex(page));
        } else if (page.classList.contains('most-visited-page')) {
          this.setShownPage_(
              loadTimeData.getInteger('most_visited_page_id'), 0);
        } else if (page.classList.contains('recently-closed-page')) {
          this.setShownPage_(
              loadTimeData.getInteger('recently_closed_page_id'), 0);
        } else if (page.classList.contains('suggestions-page')) {
          this.setShownPage_(loadTimeData.getInteger('suggestions_page_id'), 0);
        } else {
          console.error('unknown page selected');
        }
      }

      // Update the active dot
      var curDot = this.dotList.getElementsByClassName('selected')[0];
      if (curDot)
        curDot.classList.remove('selected');
      page.navigationDot.classList.add('selected');
      this.updatePageSwitchers();
    },

    /**
     * Saves/updates the newly selected page to open when first loading the NTP.
     * @type {number} shownPage The new shown page type.
     * @type {number} shownPageIndex The new shown page index.
     * @private
     */
    setShownPage_: function(shownPage, shownPageIndex) {
      assert(shownPageIndex >= 0);
      this.shownPage = shownPage;
      this.shownPageIndex = shownPageIndex;
      chrome.send('pageSelected', [this.shownPage, this.shownPageIndex]);
    },

    /**
     * Listen for card additions to update the page switchers or the current
     * card accordingly.
     * @param {Event} e A card removed or added event.
     */
    onCardAdded_: function(e) {
      // When the second arg passed to insertBefore is falsey, it acts just like
      // appendChild.
      this.pageList.insertBefore(e.addedCard, this.tilePages[e.addedIndex]);
      this.onCardAddedOrRemoved_();
    },

    /**
     * Listen for card removals to update the page switchers or the current card
     * accordingly.
     * @param {Event} e A card removed or added event.
     */
    onCardRemoved_: function(e) {
      e.removedCard.parentNode.removeChild(e.removedCard);
      this.onCardAddedOrRemoved_();
    },

    /**
     * Called when a card is removed or added.
     * @private
     */
    onCardAddedOrRemoved_: function() {
      if (this.isStartingUp_())
        return;

      // Without repositioning there were issues - http://crbug.com/133457.
      this.cardSlider.repositionFrame();
      this.updatePageSwitchers();
    },

    /**
     * Window resize handler.
     * @private
     */
    onWindowResize_: function(e) {
      this.cardSlider.resize(this.sliderFrame.offsetWidth);
      this.updatePageSwitchers();
    },

    /**
     * Listener for offline status change events. Updates apps that are
     * not offline-enabled to be grayscale if the browser is offline.
     * @private
     */
    updateOfflineEnabledApps_: function() {
      var apps = document.querySelectorAll('.app');
      for (var i = 0; i < apps.length; ++i) {
        if (apps[i].appData.enabled && !apps[i].appData.offline_enabled) {
          apps[i].setIcon();
          apps[i].loadIcon();
        }
      }
    },

    /**
     * Handler for key events on the page. Ctrl-Arrow will switch the visible
     * page.
     * @param {Event} e The KeyboardEvent.
     * @private
     */
    onDocKeyDown_: function(e) {
      if (!e.ctrlKey || e.altKey || e.metaKey || e.shiftKey)
        return;

      var direction = 0;
      if (e.keyIdentifier == 'Left')
        direction = -1;
      else if (e.keyIdentifier == 'Right')
        direction = 1;
      else
        return;

      var cardIndex =
          (this.cardSlider.currentCard + direction +
           this.cardSlider.cardCount) % this.cardSlider.cardCount;
      this.cardSlider.selectCard(cardIndex, true);

      e.stopPropagation();
    },

    /**
     * Returns the index of a given tile page.
     * @param {TilePage} page The TilePage we wish to find.
     * @return {number} The index of |page| or -1 if it is not in the
     *    collection.
     */
    getTilePageIndex: function(page) {
      return Array.prototype.indexOf.call(this.tilePages, page);
    },

    /**
     * Removes a page and navigation dot (if the navdot exists).
     * @param {TilePage} page The page to be removed.
     */
    removeTilePageAndDot_: function(page) {
      if (page.navigationDot)
        page.navigationDot.remove();
      this.cardSlider.removeCard(page);
    }
  };

  return {
    PageListView: PageListView
  };
});
