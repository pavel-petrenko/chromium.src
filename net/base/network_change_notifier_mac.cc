// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/network_change_notifier_mac.h"

#include <netinet/in.h>
#include <resolv.h>

#include "base/basictypes.h"
#include "base/threading/thread.h"
#include "net/dns/dns_config_watcher.h"

#ifndef _PATH_RESCONF  // Normally defined in <resolv.h>
#define _PATH_RESCONF "/etc/resolv.conf"
#endif

namespace net {

static bool CalculateReachability(SCNetworkConnectionFlags flags) {
  bool reachable = flags & kSCNetworkFlagsReachable;
  bool connection_required = flags & kSCNetworkFlagsConnectionRequired;
  return reachable && !connection_required;
}

NetworkChangeNotifier::ConnectionType CalculateConnectionType(
    SCNetworkConnectionFlags flags) {
  bool reachable = CalculateReachability(flags);
  if (reachable) {
#if defined(OS_IOS)
    return (flags & kSCNetworkReachabilityFlagsIsWWAN) ?
        NetworkChangeNotifier::CONNECTION_3G :
        NetworkChangeNotifier::CONNECTION_WIFI;
#else
    // TODO(droger): Get something more detailed than CONNECTION_UNKNOWN.
    // http://crbug.com/112937
    return NetworkChangeNotifier::CONNECTION_UNKNOWN;
#endif  // defined(OS_IOS)
  } else {
    return NetworkChangeNotifier::CONNECTION_NONE;
  }
}

class NetworkChangeNotifierMac::DnsWatcherThread : public base::Thread {
 public:
  DnsWatcherThread() : base::Thread("DnsWatcher") {}

  virtual ~DnsWatcherThread() {
    Stop();
  }

  virtual void Init() OVERRIDE {
    watcher_.Init();
  }

  virtual void CleanUp() OVERRIDE {
    watcher_.CleanUp();
  }

 private:
  internal::DnsConfigWatcher watcher_;

  DISALLOW_COPY_AND_ASSIGN(DnsWatcherThread);
};

NetworkChangeNotifierMac::NetworkChangeNotifierMac()
    : connection_type_(CONNECTION_UNKNOWN),
      connection_type_initialized_(false),
      initial_connection_type_cv_(&connection_type_lock_),
      forwarder_(this),
      dns_watcher_thread_(new DnsWatcherThread()) {
  // Must be initialized after the rest of this object, as it may call back into
  // SetInitialConnectionType().
  config_watcher_.reset(new NetworkConfigWatcherMac(&forwarder_));
  dns_watcher_thread_->StartWithOptions(
      base::Thread::Options(MessageLoop::TYPE_IO, 0));
}

NetworkChangeNotifierMac::~NetworkChangeNotifierMac() {
  // Delete the ConfigWatcher to join the notifier thread, ensuring that
  // StartReachabilityNotifications() has an opportunity to run to completion.
  config_watcher_.reset();

  // Now that StartReachabilityNotifications() has either run to completion or
  // never run at all, unschedule reachability_ if it was previously scheduled.
  if (reachability_.get() && run_loop_.get()) {
    SCNetworkReachabilityUnscheduleFromRunLoop(reachability_.get(),
                                               run_loop_.get(),
                                               kCFRunLoopCommonModes);
  }
}

NetworkChangeNotifier::ConnectionType
NetworkChangeNotifierMac::GetCurrentConnectionType() const {
  base::AutoLock lock(connection_type_lock_);
  // Make sure the initial connection type is set before returning.
  while (!connection_type_initialized_) {
    initial_connection_type_cv_.Wait();
  }
  return connection_type_;
}

void NetworkChangeNotifierMac::Forwarder::Init()  {
  net_config_watcher_->SetInitialConnectionType();
}

void NetworkChangeNotifierMac::Forwarder::StartReachabilityNotifications() {
  net_config_watcher_->StartReachabilityNotifications();
}

void NetworkChangeNotifierMac::Forwarder::SetDynamicStoreNotificationKeys(
    SCDynamicStoreRef store)  {
  net_config_watcher_->SetDynamicStoreNotificationKeys(store);
}

void NetworkChangeNotifierMac::Forwarder::OnNetworkConfigChange(
    CFArrayRef changed_keys)  {
  net_config_watcher_->OnNetworkConfigChange(changed_keys);
}

void NetworkChangeNotifierMac::SetInitialConnectionType() {
  // Called on notifier thread.

  // Try to reach 0.0.0.0. This is the approach taken by Firefox:
  //
  // http://mxr.mozilla.org/mozilla2.0/source/netwerk/system/mac/nsNetworkLinkService.mm
  //
  // From my (adamk) testing on Snow Leopard, 0.0.0.0
  // seems to be reachable if any network connection is available.
  struct sockaddr_in addr = {0};
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  reachability_.reset(SCNetworkReachabilityCreateWithAddress(
      kCFAllocatorDefault, reinterpret_cast<struct sockaddr*>(&addr)));

  SCNetworkConnectionFlags flags;
  ConnectionType connection_type = CONNECTION_UNKNOWN;
  if (SCNetworkReachabilityGetFlags(reachability_, &flags)) {
    connection_type = CalculateConnectionType(flags);
  } else {
    LOG(ERROR) << "Could not get initial network connection type,"
               << "assuming online.";
  }
  {
    base::AutoLock lock(connection_type_lock_);
    connection_type_ = connection_type;
    connection_type_initialized_ = true;
    initial_connection_type_cv_.Signal();
  }
}

void NetworkChangeNotifierMac::StartReachabilityNotifications() {
  // Called on notifier thread.
  run_loop_.reset(CFRunLoopGetCurrent());
  CFRetain(run_loop_.get());

  DCHECK(reachability_);
  SCNetworkReachabilityContext reachability_context = {
    0,     // version
    this,  // user data
    NULL,  // retain
    NULL,  // release
    NULL   // description
  };
  if (!SCNetworkReachabilitySetCallback(
          reachability_,
          &NetworkChangeNotifierMac::ReachabilityCallback,
          &reachability_context)) {
    LOG(DFATAL) << "Could not set network reachability callback";
    reachability_.reset();
  } else if (!SCNetworkReachabilityScheduleWithRunLoop(reachability_,
                                                       run_loop_,
                                                       kCFRunLoopCommonModes)) {
    LOG(DFATAL) << "Could not schedule network reachability on run loop";
    reachability_.reset();
  }
}

void NetworkChangeNotifierMac::SetDynamicStoreNotificationKeys(
    SCDynamicStoreRef store) {
#if defined(OS_IOS)
  // SCDynamicStore API does not exist on iOS.
  NOTREACHED();
#else
  base::mac::ScopedCFTypeRef<CFMutableArrayRef> notification_keys(
      CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks));
  base::mac::ScopedCFTypeRef<CFStringRef> key(
      SCDynamicStoreKeyCreateNetworkGlobalEntity(
          NULL, kSCDynamicStoreDomainState, kSCEntNetInterface));
  CFArrayAppendValue(notification_keys.get(), key.get());
  key.reset(SCDynamicStoreKeyCreateNetworkGlobalEntity(
      NULL, kSCDynamicStoreDomainState, kSCEntNetIPv4));
  CFArrayAppendValue(notification_keys.get(), key.get());
  key.reset(SCDynamicStoreKeyCreateNetworkGlobalEntity(
      NULL, kSCDynamicStoreDomainState, kSCEntNetIPv6));
  CFArrayAppendValue(notification_keys.get(), key.get());

  // Set the notification keys.  This starts us receiving notifications.
  bool ret = SCDynamicStoreSetNotificationKeys(
      store, notification_keys.get(), NULL);
  // TODO(willchan): Figure out a proper way to handle this rather than crash.
  CHECK(ret);
#endif  // defined(OS_IOS)
}

void NetworkChangeNotifierMac::OnNetworkConfigChange(CFArrayRef changed_keys) {
#if defined(OS_IOS)
  // SCDynamicStore API does not exist on iOS.
  NOTREACHED();
#else
  DCHECK_EQ(run_loop_.get(), CFRunLoopGetCurrent());

  for (CFIndex i = 0; i < CFArrayGetCount(changed_keys); ++i) {
    CFStringRef key = static_cast<CFStringRef>(
        CFArrayGetValueAtIndex(changed_keys, i));
    if (CFStringHasSuffix(key, kSCEntNetIPv4) ||
        CFStringHasSuffix(key, kSCEntNetIPv6)) {
      NotifyObserversOfIPAddressChange();
      return;
    }
    if (CFStringHasSuffix(key, kSCEntNetInterface)) {
      // TODO(willchan): Does not appear to be working.  Look into this.
      // Perhaps this isn't needed anyway.
    } else {
      NOTREACHED();
    }
  }
#endif  // defined(OS_IOS)
}

// static
void NetworkChangeNotifierMac::ReachabilityCallback(
    SCNetworkReachabilityRef target,
    SCNetworkConnectionFlags flags,
    void* notifier) {
  NetworkChangeNotifierMac* notifier_mac =
      static_cast<NetworkChangeNotifierMac*>(notifier);

  DCHECK_EQ(notifier_mac->run_loop_.get(), CFRunLoopGetCurrent());

  ConnectionType new_type = CalculateConnectionType(flags);
  ConnectionType old_type;
  {
    base::AutoLock lock(notifier_mac->connection_type_lock_);
    old_type = notifier_mac->connection_type_;
    notifier_mac->connection_type_ = new_type;
  }
  if (old_type != new_type)
    NotifyObserversOfConnectionTypeChange();

#if defined(OS_IOS)
  // On iOS, the SCDynamicStore API does not exist, and we use the reachability
  // API to detect IP address changes instead.
  if (new_type != CONNECTION_NONE)
    NotifyObserversOfIPAddressChange();
#endif  // defined(OS_IOS)
}

}  // namespace net
