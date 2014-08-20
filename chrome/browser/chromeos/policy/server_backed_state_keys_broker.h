// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_SERVER_BACKED_STATE_KEYS_BROKER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_SERVER_BACKED_STATE_KEYS_BROKER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace base {
class TaskRunner;
}

namespace chromeos {
class SessionManagerClient;
}

namespace policy {

// Brokers server-backed state keys for the device. Retrieves them from session
// manager via DBus and refreshes them periodically. Consumers can register
// callbacks to invoke when the state keys change.
class ServerBackedStateKeysBroker {
 public:
  typedef scoped_ptr<base::CallbackList<void()>::Subscription> Subscription;
  typedef base::Callback<void(const std::vector<std::string>&, bool)>
      StateKeysCallback;

  ServerBackedStateKeysBroker(
      chromeos::SessionManagerClient* session_manager_client,
      scoped_refptr<base::TaskRunner> delayed_task_runner);
  ~ServerBackedStateKeysBroker();

  // Registers a callback to be invoked whenever the state keys get updated.
  // Note that consuming code needs to hold on to the returned Subscription as
  // long as it wants to receive the callback. If the state keys haven't been
  // requested yet, calling this will also trigger their initial fetch.
  Subscription RegisterUpdateCallback(const base::Closure& callback);

  // Requests state keys asynchronously. Invokes the passed callback exactly
  // once (unless |this| gets destroyed before the callback happens), with the
  // current state keys passed as a parameter to the callback. If there's a
  // problem determining the state keys, the passed vector will be empty.
  void RequestStateKeys(const StateKeysCallback& callback);

  // Get the set of current state keys. Empty if state keys are unavailable
  // or pending retrieval.
  const std::vector<std::string>& state_keys() const { return state_keys_; }

  // Returns the state key for the current point in time. Returns an empty
  // string if state keys are unavailable or pending retrieval.
  std::string current_state_key() const {
    return state_keys_.empty() ? std::string() : state_keys_.front();
  }

  // Whether state key retrieval is pending.
  bool pending() const { return !initial_retrieval_completed_; }

  // Whether state keys are available.
  bool available() const { return !state_keys_.empty(); }

 private:
  // Asks |session_manager_client_| to provide current state keys..
  void FetchStateKeys();

  // Stores newly-received state keys and notifies consumers.
  void StoreStateKeys(const std::vector<std::string>& state_keys,
                      bool first_boot);

  chromeos::SessionManagerClient* session_manager_client_;

  scoped_refptr<base::TaskRunner> delayed_task_runner_;

  // The current set of state keys.
  std::vector<std::string> state_keys_;

  // Set to true on first run after factory reset.
  bool first_boot_;

  // Whether a request for state keys is pending.
  bool requested_;

  // Whether the initial retrieval operation completed.
  bool initial_retrieval_completed_;

  // List of callbacks to receive update notifications.
  base::CallbackList<void()> update_callbacks_;

  // List of pending one-shot state key request callbacks.
  std::vector<StateKeysCallback> request_callbacks_;

  base::WeakPtrFactory<ServerBackedStateKeysBroker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServerBackedStateKeysBroker);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_SERVER_BACKED_STATE_KEYS_BROKER_H_
