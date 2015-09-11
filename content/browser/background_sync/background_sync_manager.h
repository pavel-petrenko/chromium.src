// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_MANAGER_H_
#define CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_MANAGER_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/background_sync/background_sync.pb.h"
#include "content/browser/background_sync/background_sync_registration.h"
#include "content/browser/background_sync/background_sync_registration_handle.h"
#include "content/browser/background_sync/background_sync_status.h"
#include "content/browser/cache_storage/cache_storage_scheduler.h"
#include "content/browser/service_worker/service_worker_context_observer.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "url/gurl.h"

namespace content {

class BackgroundSyncNetworkObserver;
class BackgroundSyncPowerObserver;
class ServiceWorkerContextWrapper;

// BackgroundSyncManager manages and stores the set of background sync
// registrations across all registered service workers for a profile.
// Registrations are stored along with their associated Service Worker
// registration in ServiceWorkerStorage. If the ServiceWorker is unregistered,
// the sync registrations are removed. This class expects to be run on the IO
// thread. The asynchronous methods are executed sequentially.

// TODO(jkarlin): Check permissions when registering, scheduling, and firing
// background sync. In the meantime, --enable-service-worker-sync is required to
// fire a sync event.
// TODO(jkarlin): Unregister syncs when permission is revoked.
// TODO(jkarlin): Create a background sync scheduler to actually run the
// registered events.
// TODO(jkarlin): Keep the browser alive if "Let Google Chrome Run in the
// Background" is true and a sync is registered.
class CONTENT_EXPORT BackgroundSyncManager
    : NON_EXPORTED_BASE(public ServiceWorkerContextObserver) {
 public:
  using StatusCallback = base::Callback<void(BackgroundSyncStatus)>;
  using StatusAndRegistrationCallback =
      base::Callback<void(BackgroundSyncStatus,
                          scoped_ptr<BackgroundSyncRegistrationHandle>)>;
  using StatusAndRegistrationsCallback = base::Callback<void(
      BackgroundSyncStatus,
      scoped_ptr<ScopedVector<BackgroundSyncRegistrationHandle>>)>;

  static scoped_ptr<BackgroundSyncManager> Create(
      const scoped_refptr<ServiceWorkerContextWrapper>& service_worker_context);
  ~BackgroundSyncManager() override;

  // Stores the given background sync registration and adds it to the scheduling
  // queue. It will overwrite an existing registration with the same tag and
  // periodicity unless they're identical (save for the id). Calls |callback|
  // with BACKGROUND_SYNC_STATUS_OK and the accepted registration on success.
  // The accepted registration will have a unique id. It may also have altered
  // parameters if the user or UA chose different parameters than those
  // supplied.
  void Register(int64 sw_registration_id,
                const BackgroundSyncRegistrationOptions& options,
                bool requested_from_service_worker,
                const StatusAndRegistrationCallback& callback);

  // Finds the background sync registration associated with
  // |sw_registration_id| with periodicity |periodicity|. Calls
  // |callback| with BACKGROUND_SYNC_STATUS_NOT_FOUND if it doesn't exist. Calls
  // |callback| with BACKGROUND_SYNC_STATUS_OK on success.
  void GetRegistration(int64 sw_registration_id,
                       const std::string& sync_registration_tag,
                       SyncPeriodicity periodicity,
                       const StatusAndRegistrationCallback& callback);

  void GetRegistrations(int64 sw_registration_id,
                        SyncPeriodicity periodicity,
                        const StatusAndRegistrationsCallback& callback);

  // Given a HandleId |handle_id|, return a new handle for the same
  // registration.
  scoped_ptr<BackgroundSyncRegistrationHandle> DuplicateRegistrationHandle(
      BackgroundSyncRegistrationHandle::HandleId handle_id);

  // ServiceWorkerContextObserver overrides.
  void OnRegistrationDeleted(int64 registration_id,
                             const GURL& pattern) override;
  void OnStorageWiped() override;

 protected:
  // A registration might be referenced by the client longer than
  // the BackgroundSyncManager needs to keep track of it (e.g., the event has
  // finished firing). The BackgroundSyncManager reference counts its
  // registrations internally and every BackgroundSyncRegistrationHandle has a
  // unique handle id which maps to a locally maintained (in
  // client_registration_ids_) scoped_refptr.
  class RefCountedRegistration;

  explicit BackgroundSyncManager(
      const scoped_refptr<ServiceWorkerContextWrapper>& context);

  // Init must be called before any public member function. Only call it once.
  void Init();

  // The following methods are virtual for testing.
  virtual void StoreDataInBackend(
      int64 sw_registration_id,
      const GURL& origin,
      const std::string& backend_key,
      const std::string& data,
      const ServiceWorkerStorage::StatusCallback& callback);
  virtual void GetDataFromBackend(
      const std::string& backend_key,
      const ServiceWorkerStorage::GetUserDataForAllRegistrationsCallback&
          callback);
  virtual void FireOneShotSync(
      BackgroundSyncRegistrationHandle::HandleId handle_id,
      const scoped_refptr<ServiceWorkerVersion>& active_version,
      const ServiceWorkerVersion::StatusCallback& callback);

 private:
  friend class BackgroundSyncRegistrationHandle;

  class RegistrationKey {
   public:
    explicit RegistrationKey(const BackgroundSyncRegistration& registration);
    explicit RegistrationKey(const BackgroundSyncRegistrationOptions& options);
    RegistrationKey(const std::string& tag, SyncPeriodicity periodicity);
    RegistrationKey(const RegistrationKey& other) = default;
    RegistrationKey& operator=(const RegistrationKey& other) = default;

    bool operator<(const RegistrationKey& rhs) const {
      return value_ < rhs.value_;
    }

   private:
    std::string value_;
  };

  struct BackgroundSyncRegistrations {
    using RegistrationMap =
        std::map<RegistrationKey, scoped_refptr<RefCountedRegistration>>;

    BackgroundSyncRegistrations();
    ~BackgroundSyncRegistrations();

    RegistrationMap registration_map;
    BackgroundSyncRegistration::RegistrationId next_id;
    GURL origin;
  };

  using PermissionStatusCallback = base::Callback<void(bool)>;
  using SWIdToRegistrationsMap = std::map<int64, BackgroundSyncRegistrations>;

  scoped_ptr<BackgroundSyncRegistrationHandle> CreateRegistrationHandle(
      const scoped_refptr<RefCountedRegistration>& registration);

  // Returns the BackgroundSyncRegistration corresponding to |handle_id|.
  // Returns nullptr if the registration is not found.
  BackgroundSyncRegistration* GetRegistrationForHandle(
      BackgroundSyncRegistrationHandle::HandleId handle_id) const;

  // The BackgroundSyncManager holds references to registrations that have
  // active Handles. The handles must call this on destruction.
  void ReleaseRegistrationHandle(
      BackgroundSyncRegistrationHandle::HandleId handle_id);

  // Disable the manager. Already queued operations will abort once they start
  // to run (in their impl methods). Future operations will not queue. Any
  // registrations are cleared from memory and the backend (if it's still
  // functioning). The manager will reenable itself once it receives the
  // OnStorageWiped message or on browser restart.
  void DisableAndClearManager(const base::Closure& callback);
  void DisableAndClearDidGetRegistrations(
      const base::Closure& callback,
      const std::vector<std::pair<int64, std::string>>& user_data,
      ServiceWorkerStatusCode status);
  void DisableAndClearManagerClearedOne(const base::Closure& barrier_closure,
                                        ServiceWorkerStatusCode status);

  // Returns the existing registration or nullptr if it cannot be found.
  RefCountedRegistration* LookupActiveRegistration(
      int64 sw_registration_id,
      const RegistrationKey& registration_key);

  // Write all registrations for a given |sw_registration_id| to persistent
  // storage.
  void StoreRegistrations(int64 sw_registration_id,
                          const ServiceWorkerStorage::StatusCallback& callback);

  // Removes the active registration if it is in the map.
  void RemoveActiveRegistration(int64 sw_registration_id,
                                const RegistrationKey& registration_key);

  void AddActiveRegistration(
      int64 sw_registration_id,
      const GURL& origin,
      const scoped_refptr<RefCountedRegistration>& sync_registration);

  void InitImpl(const base::Closure& callback);
  void InitDidGetDataFromBackend(
      const base::Closure& callback,
      const std::vector<std::pair<int64, std::string>>& user_data,
      ServiceWorkerStatusCode status);

  // Register callbacks
  void RegisterImpl(int64 sw_registration_id,
                    const BackgroundSyncRegistrationOptions& options,
                    bool requested_from_service_worker,
                    const StatusAndRegistrationCallback& callback);
  void RegisterDidStore(
      int64 sw_registration_id,
      const scoped_refptr<RefCountedRegistration>& new_registration_ref,
      const StatusAndRegistrationCallback& callback,
      ServiceWorkerStatusCode status);

  // Removes the background sync with periodicity |periodicity| and id
  // |sync_registration_id|. Calls |callback| with
  // BACKGROUND_SYNC_STATUS_NOT_FOUND if no match is found. Calls |callback|
  // with BACKGROUND_SYNC_STATUS_OK on success.
  void Unregister(int64 sw_registration_id,
                  SyncPeriodicity periodicity,
                  BackgroundSyncRegistrationHandle::HandleId handle_id,
                  const StatusCallback& callback);
  void UnregisterImpl(
      int64 sw_registration_id,
      const RegistrationKey& key,
      BackgroundSyncRegistration::RegistrationId sync_registration_id,
      SyncPeriodicity periodicity,
      const StatusCallback& callback);
  void UnregisterDidStore(int64 sw_registration_id,
                          SyncPeriodicity periodicity,
                          const StatusCallback& callback,
                          ServiceWorkerStatusCode status);

  // GetRegistration callbacks
  void GetRegistrationImpl(int64 sw_registration_id,
                           const RegistrationKey& registration_key,
                           const StatusAndRegistrationCallback& callback);

  // GetRegistrations callbacks
  void GetRegistrationsImpl(int64 sw_registration_id,
                            SyncPeriodicity periodicity,
                            const StatusAndRegistrationsCallback& callback);

  bool AreOptionConditionsMet(const BackgroundSyncRegistrationOptions& options);
  bool IsRegistrationReadyToFire(
      const BackgroundSyncRegistration& registration);

  // Schedules pending registrations to run in the future. For one-shots this
  // means keeping the browser alive so that network connectivity events can be
  // seen (on Android the browser is instead woken up the next time it goes
  // online). For periodic syncs this means creating an alarm.
  void SchedulePendingRegistrations();

  // FireReadyEvents and callbacks
  void FireReadyEvents();
  void FireReadyEventsImpl(const base::Closure& callback);
  void FireReadyEventsDidFindRegistration(
      const RegistrationKey& registration_key,
      BackgroundSyncRegistration::RegistrationId registration_id,
      const base::Closure& event_fired_callback,
      const base::Closure& event_completed_callback,
      ServiceWorkerStatusCode service_worker_status,
      const scoped_refptr<ServiceWorkerRegistration>&
          service_worker_registration);

  // Called when a sync event has completed.
  void EventComplete(
      const scoped_refptr<ServiceWorkerRegistration>&
          service_worker_registration,
      int64 service_worker_id,
      scoped_ptr<BackgroundSyncRegistrationHandle> registration_handle,
      const base::Closure& callback,
      ServiceWorkerStatusCode status_code);
  void EventCompleteImpl(
      int64 service_worker_id,
      scoped_ptr<BackgroundSyncRegistrationHandle> registration_handle,
      ServiceWorkerStatusCode status_code,
      const base::Closure& callback);
  void EventCompleteDidStore(int64 service_worker_id,
                             const base::Closure& callback,
                             ServiceWorkerStatusCode status_code);

  // Called when all sync events have completed.
  static void OnAllSyncEventsCompleted(const base::TimeTicks& start_time,
                                       int number_of_batched_sync_events);

  // OnRegistrationDeleted callbacks
  void OnRegistrationDeletedImpl(int64 registration_id,
                                 const base::Closure& callback);

  // OnStorageWiped callbacks
  void OnStorageWipedImpl(const base::Closure& callback);

  void OnNetworkChanged();
  void OnPowerChanged();

  // Operation Scheduling callback and convenience functions.
  template <typename CallbackT, typename... Params>
  void CompleteOperationCallback(const CallbackT& callback,
                                 Params... parameters);
  void CompleteStatusAndRegistrationCallback(
      StatusAndRegistrationCallback callback,
      BackgroundSyncStatus status,
      scoped_ptr<BackgroundSyncRegistrationHandle> result);
  void CompleteStatusAndRegistrationsCallback(
      StatusAndRegistrationsCallback callback,
      BackgroundSyncStatus status,
      scoped_ptr<ScopedVector<BackgroundSyncRegistrationHandle>> results);
  base::Closure MakeEmptyCompletion();
  base::Closure MakeClosureCompletion(const base::Closure& callback);
  StatusAndRegistrationCallback MakeStatusAndRegistrationCompletion(
      const StatusAndRegistrationCallback& callback);
  StatusAndRegistrationsCallback MakeStatusAndRegistrationsCompletion(
      const StatusAndRegistrationsCallback& callback);
  BackgroundSyncManager::StatusCallback MakeStatusCompletion(
      const StatusCallback& callback);

  SWIdToRegistrationsMap active_registrations_;
  CacheStorageScheduler op_scheduler_;
  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;
  bool disabled_;

  scoped_ptr<BackgroundSyncNetworkObserver> network_observer_;
  scoped_ptr<BackgroundSyncPowerObserver> power_observer_;

  // The registrations that clients have handles to.
  IDMap<scoped_refptr<RefCountedRegistration>, IDMapOwnPointer>
      registration_handle_ids_;

  base::WeakPtrFactory<BackgroundSyncManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_MANAGER_H_
