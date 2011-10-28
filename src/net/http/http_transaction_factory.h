// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_TRANSACTION_FACTORY_H__
#define NET_HTTP_HTTP_TRANSACTION_FACTORY_H__
#pragma once

#include "base/memory/scoped_ptr.h"
#include "net/base/net_export.h"

namespace net {

class HttpCache;
class HttpNetworkSession;
class HttpTransaction;


// An interface to a class that can create HttpTransaction objects.
class NET_EXPORT HttpTransactionFactory {
 public:
  virtual ~HttpTransactionFactory() {}

  // Creates a HttpTransaction object. On success, saves the new
  // transaction to |*trans| and returns OK.
  virtual int CreateTransaction(scoped_ptr<HttpTransaction>* trans) = 0;

  // Returns the associated cache if any (may be NULL).
  virtual HttpCache* GetCache() = 0;

  // Returns the associated HttpNetworkSession used by new transactions.
  virtual HttpNetworkSession* GetSession() = 0;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_TRANSACTION_FACTORY_H__
