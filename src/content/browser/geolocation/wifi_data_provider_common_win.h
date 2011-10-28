// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GEOLOCATION_WIFI_DATA_PROVIDER_COMMON_WIN_H_
#define CONTENT_BROWSER_GEOLOCATION_WIFI_DATA_PROVIDER_COMMON_WIN_H_
#pragma once

#include <windows.h>
#include <ntddndis.h>

#include "content/browser/geolocation/device_data_provider.h"

// Extracts access point data from the NDIS_802_11_BSSID_LIST structure and
// appends it to the data vector. Returns the number of access points for which
// data was extracted.
int GetDataFromBssIdList(const NDIS_802_11_BSSID_LIST& bss_id_list,
                         int list_size,
                         WifiData::AccessPointDataSet* data);

#endif  // CONTENT_BROWSER_GEOLOCATION_WIFI_DATA_PROVIDER_COMMON_WIN_H_
