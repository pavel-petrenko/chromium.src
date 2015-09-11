// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The "onsync" event currently understands commands (passed as
// registration tags) coming from the test. Any other tag is
// passed through to the document unchanged.
//
// "delay" - Delays finishing the sync event with event.waitUntil.
//           Send a postMessage of "completeDelayedOneShot" to finish the
//           event.
// "unregister" - Unregisters the sync registration from within the sync event.

'use strict';

var resolveCallback = null;
var rejectCallback = null;

this.onmessage = function(event) {
  if (event.data === 'completeDelayedOneShot') {
    if (resolveCallback === null) {
      sendMessageToClients('sync', 'error - resolveCallback is null');
      return;
    }

    resolveCallback();
    sendMessageToClients('sync', 'ok - delay completed');
    return;
  }

  if (event.data === 'rejectDelayedOneShot') {
    if (rejectCallback === null) {
      sendMessageToClients('sync', 'error - rejectCallback is null');
      return;
    }

    rejectCallback();
    sendMessageToClients('sync', 'ok - delay rejected');
  }
}

this.onsync = function(event) {
  var eventProperties = [
    // Extract name from toString result: "[object <Class>]"
    Object.prototype.toString.call(event).match(/\s([a-zA-Z]+)/)[1],
    (typeof event.waitUntil)
  ];

  if (eventProperties[0] != 'SyncEvent') {
    sendMessageToClients('sync', 'error - wrong event type');
    return;
  }

  if (eventProperties[1] != 'function') {
    sendMessageToClients('sync', 'error - wrong wait until type');
  }

  if (event.registration === undefined) {
    sendMessageToClients('sync', 'error - event missing registration');
    return;
  }

  if (event.registration.tag === undefined) {
    sendMessageToClients('sync', 'error - registration missing tag');
    return;
  }

  var tag = event.registration.tag;

  if (tag === 'delay') {
    var syncPromise = new Promise(function(resolve, reject) {
      resolveCallback = resolve;
      rejectCallback = reject;
    });
    event.waitUntil(syncPromise);
    return;
  }

  if (tag === 'unregister') {
    event.waitUntil(event.registration.unregister()
      .then(function() {
        sendMessageToClients('sync', 'ok - unregister completed');
      }));
    return;
  }

  sendMessageToClients('sync', tag + ' fired');
};

function sendMessageToClients(type, data) {
  clients.matchAll().then(function(clients) {
    clients.forEach(function(client) {
      client.postMessage({type, data});
    });
  }, function(error) {
    console.log(error);
  });
}
