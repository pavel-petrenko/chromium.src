// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_BRIDGED_INVALIDATOR_H_
#define CHROME_BROWSER_SYNC_GLUE_BRIDGED_INVALIDATOR_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "sync/notifier/invalidator.h"

namespace browser_sync {

class ChromeSyncNotificationBridge;

// An Invalidator implementation that wraps a ChromeSyncNotificationBridge
// and optionally an Invalidator delegate (which it takes ownership of).
// All Invalidator calls are passed straight through to the delegate,
// with the exception of RegisterHandler, UpdateRegisteredIds, and
// UnregisterHandler, which result in the handler being registered, updated,
// and unregistered with the ChromeSyncNotificationBridge, respectively.
class BridgedInvalidator : public syncer::Invalidator {
 public:
  // Does not take ownership of |bridge|. Takes ownership of |delegate|.
  // |delegate| may be NULL.
  BridgedInvalidator(ChromeSyncNotificationBridge* bridge,
                     syncer::Invalidator* delegate);
  virtual ~BridgedInvalidator();

  // Invalidator implementation. Passes through all calls to the delegate.
  // RegisterHandler, UpdateRegisteredIds, and UnregisterHandler calls will
  // also be forwarded to the bridge.
  virtual void RegisterHandler(syncer::InvalidationHandler* handler) OVERRIDE;
  virtual void UpdateRegisteredIds(syncer::InvalidationHandler * handler,
                                   const syncer::ObjectIdSet& ids) OVERRIDE;
  virtual void UnregisterHandler(syncer::InvalidationHandler* handler) OVERRIDE;
  virtual void SetUniqueId(const std::string& unique_id) OVERRIDE;
  virtual void SetStateDeprecated(const std::string& state) OVERRIDE;
  virtual void UpdateCredentials(
      const std::string& email, const std::string& token) OVERRIDE;
  virtual void SendNotification(
      syncer::ModelTypeSet changed_types) OVERRIDE;

 private:
  // The notification bridge that we register the observers with.
  ChromeSyncNotificationBridge* bridge_;

  // The delegate we are wrapping.
  scoped_ptr<syncer::Invalidator> delegate_;
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_BRIDGED_INVALIDATOR_H_
