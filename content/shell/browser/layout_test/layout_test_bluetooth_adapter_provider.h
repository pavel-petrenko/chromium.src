// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BLUETOOTH_ADAPTER_PROVIDER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BLUETOOTH_ADAPTER_PROVIDER_H_

#include "base/callback.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "device/bluetooth/test/mock_bluetooth_device.h"
#include "device/bluetooth/test/mock_bluetooth_discovery_session.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_characteristic.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_service.h"

namespace content {

// Implements fake adapters with named mock data set for use in tests as a
// result of layout tests calling testRunner.setBluetoothMockDataSet.

// We have a complete “GenericAccessAdapter”, meaning it has a device which has
// a Generic Access service with a Device Name characteristic with a descriptor.
// The other adapters are named based on their particular non-expected behavior.

class LayoutTestBluetoothAdapterProvider {
 public:
  // Returns a BluetoothAdapter. Its behavior depends on |fake_adapter_name|.
  static scoped_refptr<device::BluetoothAdapter> GetBluetoothAdapter(
      const std::string& fake_adapter_name);

 private:
  // Adapters

  // |BaseAdapter|
  // Devices Added:
  //  None.
  // Mock Functions:
  //  - GetDevices:
  //      Returns a list of devices added to the adapter.
  //  - GetDevice:
  //      Returns a device matching the address provided if the device was
  //      added to the adapter.
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetBaseAdapter();

  // |ScanFilterCheckingAdapter|
  // Inherits from |BaseAdapter|
  // BluetoothAdapter that asserts that its StartDiscoverySessionWithFilter()
  // method is called with a filter consisting of the standard battery, heart
  // rate, and glucose services.
  // Devices added:
  //  - BatteryDevice
  // Mock Functions:
  //  - StartDiscoverySessionWithFilter:
  //      - With correct arguments: Run success callback.
  //      - With incorrect arguments: Mock complains that function with
  //        correct arguments was never called and error callback is called.
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetScanFilterCheckingAdapter();

  // |FailStartDiscoveryAdapter|
  // Inherits from |BaseAdapter|
  // Devices added:
  //  None.
  // Mock Functions:
  //  - StartDiscoverySessionWithFilter:
  //      Run error callback.
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetFailStartDiscoveryAdapter();

  // |EmptyAdapter|
  // Inherits from |BaseAdapter|
  // Devices Added:
  //  None.
  // Mock Functions:
  //  - StartDiscoverySessionWithFilter:
  //      Run success callback with |DiscoverySession|.
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetEmptyAdapter();

  // |GlucoseHeartRateAdapter|
  // Inherits from |EmptyAdapter|
  // Devices added:
  //  - |GlucoseDevice|
  //  - |HeartRateDevice|
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetGlucoseHeartRateAdapter();

  // |MissingServiceGenericAccessAdapter|
  // Inherits from EmptyAdapter
  // Internal Structure:
  //   - GenericAccessDevice
  //       - Generic Access UUID (0x1800)
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetMissingServiceGenericAccessAdapter();

  // |MissingCharacteristicGenericAccessAdapter|
  // Inherits from EmptyAdapter
  // Internal Structure:
  //   - GenericAccessDevice
  //       - GenericAccess UUID (0x1800)
  //       - GenericAccessService
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetMissingCharacteristicGenericAccessAdapter();

  // |FailingConnectionsAdapter|
  // Inherits from BaseAdapter
  // FailingConnectionsAdapter holds a device for each type of connection error
  // that can occur. This way we don’t need to create an adapter for each type
  // of error. Each of the devices has a service with a different UUID so that
  // they can be accessed by using different filters.
  // See errorUUID() declaration below.
  // Internal Structure:
  //  - UnconnectableDevice(BluetoothDevice::ERROR_UNKNOWN)       errorUUID(0x0)
  //  - UnconnectableDevice(BluetoothDevice::ERROR_INPROGRESS)    errorUUID(0x1)
  //  - UnconnectableDevice(BluetoothDevice::ERROR_FAILED)        errorUUID(0x2)
  //  - UnconnectableDevice(BluetoothDevice::ERROR_AUTH_FAILED)   errorUUID(0x3)
  //  - UnconnectableDevice(BluetoothDevice::ERROR_AUTH_CANCELED) errorUUID(0x4)
  //  - UnconnectableDevice(BluetoothDevice::ERROR_AUTH_REJECTED) errorUUID(0x5)
  //  - UnconnectableDevice(BluetoothDevice::ERROR_AUTH_TIMEOUT)  errorUUID(0x6)
  //  - UnconnectableDevice(BluetoothDevice::ERROR_UNSUPPORTED_DEVICE)
  //    errorUUID(0x7)
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetFailingConnectionsAdapter();

  // Discovery Sessions

  // |DiscoverySession|
  // Mock Functions:
  //  - Stop:
  //      Run success callback.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDiscoverySession>>
  GetDiscoverySession();

  // Devices

  // |BaseDevice|
  // Adv UUIDs added:
  // None.
  // Services added:
  // None.
  // MockFunctions:
  //  - GetUUIDs:
  //      Returns uuids
  //  - GetGattServices:
  //      Returns a list of all services added to the device.
  //  - GetGattService:
  //      Return a service matching the identifier provided if the service was
  //      added to the mock.
  //  - GetAddress:
  //      Returns: address
  //  - GetName:
  //      Returns: device_name.
  //  - GetBluetoothClass:
  //      Returns: 0x1F00. “Unspecified Device Class” see
  //      bluetooth.org/en-us/specification/assigned-numbers/baseband
  //  - GetVendorIDSource:
  //      Returns: BluetoothDevice::VENDOR_ID_BLUETOOTH.
  //  - GetVendorID:
  //      Returns: 0xFFFF.
  //  - GetProductID:
  //      Returns: 1.
  //  - GetDeviceID:
  //      Returns: 2.
  //  - IsPaired:
  //      Returns true.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetBaseDevice(device::MockBluetoothAdapter* adapter,
                const std::string& device_name = "Base Device",
                device::BluetoothDevice::UUIDList uuids =
                    device::BluetoothDevice::UUIDList(),
                const std::string& address = "00:00:00:00:00");

  // |BatteryDevice|
  // Inherits from BaseDevice(adapter, "Battery Device", uuids,
  //                          "00:00:00:00:01")
  // Adv UUIDs added:
  //   - Generic Access (0x1800)
  //   - Battery Service UUID (0x180F)
  // Services added:
  // None.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetBatteryDevice(device::MockBluetoothAdapter* adapter);

  // |GlucoseDevice|
  // Inherits from BaseDevice(adapter, "Glucose Device", uuids,
  //                          "00:00:00:00:02")
  // Adv UUIDs added:
  //   - Generic Access (0x1800)
  //   - Glucose UUID (0x1808)
  // Services added:
  // None.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetGlucoseDevice(device::MockBluetoothAdapter* adapter);

  // |HeartRateDevice|
  // Inherits from BaseDevice(adapter, "Heart Rate Device", uuids,
  //                          "00:00:00:00:03")
  // Adv UUIDs added:
  //   - Generic Access (0x1800)
  //   - Heart Rate UUID (0x180D)
  // Services added:
  // None.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetHeartRateDevice(device::MockBluetoothAdapter* adapter);

  // |ConnectableDevice|
  // Inherits from BaseDevice(adapter, device_name)
  // Adv UUIDs Added:
  // None.
  // Services Added:
  // None.
  // Mock Functions:
  //   - CreateGattConnection:
  //       - Run success callback with BaseGATTConnection
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetConnectableDeviceNew(
      device::MockBluetoothAdapter* adapter,
      const std::string& device_name = "Connectable Device",
      device::BluetoothDevice::UUIDList = device::BluetoothDevice::UUIDList());

  // |UnconnectableDevice|
  // Inherits from BaseDevice(adapter, device_name)
  // Adv UUIDs Added:
  //  - errorUUID(error_code)
  // Services Added:
  // None.
  // Mock Functions:
  //  - CreateGATTConnection:
  //      - Run error callback with error_type
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetUnconnectableDevice(
      device::MockBluetoothAdapter* adapter,
      device::BluetoothDevice::ConnectErrorCode error_code,
      const std::string& device_name = "Unconnectable Device");

  // |GenericAccessDevice|
  // Inherits from ConnectableDevice(adapter, device_name)
  // Adv UUIDs Added:
  //   - Generic Access UUID (0x1800)
  // Services Added:
  // None. Each user of the GenericAccessDevice is in charge of adding the
  // relevant services, characteristics, and descriptors.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetGenericAccessDevice(
      device::MockBluetoothAdapter* adapter,
      const std::string& device_name = "Generic Access Device");

  // Services

  // |BaseGATTService|
  // Characteristics Added:
  // None.
  // Mock Functions:
  //   - GetCharacteristics:
  //       Returns a list with all the characteristics added to the service
  //   - GetCharacteristic:
  //       Returns a characteristic matching the identifier provided if the
  //       characteristic was added to the mock.
  //   - GetIdentifier:
  //       Returns: uuid + “ Identifier”
  //   - GetUUID:
  //       Returns: uuid
  //   - IsLocal:
  //       Returns: false
  //   - IsPrimary:
  //       Returns: true
  //   - GetDevice:
  //       Returns: device
  static scoped_ptr<testing::NiceMock<device::MockBluetoothGattService>>
  GetBaseGATTService(device::MockBluetoothDevice* device,
                     const std::string& uuid);

  // Helper functions:

  // errorUUID(alias) returns a UUID with the top 32 bits of
  // "00000000-97e5-4cd7-b9f1-f5a427670c59" replaced with the bits of |alias|.
  // For example, errorUUID(0xDEADBEEF) returns
  // "deadbeef-97e5-4cd7-b9f1-f5a427670c59". The bottom 96 bits of error UUIDs
  // were generated as a type 4 (random) UUID.
  static std::string errorUUID(uint32_t alias);

  // Function to turn an integer into an MAC address of the form
  // XX:XX:XX:XX:XX:XX. For example makeMACAddress(0xdeadbeef)
  // returns "00:00:DE:AD:BE:EF".
  static std::string makeMACAddress(uint64_t addr);

  // The functions after this haven't been updated to the new design yet.

  // Returns "SingleEmptyDeviceAdapter" fake BluetoothAdapter with the following
  // characteristics:
  //  - |StartDiscoverySessionWithFilter| runs the success callback with
  //  |DiscoverySession|
  //    as argument.
  //  - |GetDevices| returns a list with an |EmptyDevice|.
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetSingleEmptyDeviceAdapter();

  // Returns "ConnectableDeviceAdapter" fake BluetoothAdapter with the
  // following characteristics:
  //  - |StartDiscoverySessionWithFilter| runs the success callback with
  //  |DiscoverySession|
  //    as argument.
  //  - |GetDevices| returns a list with a |ConnectableDevice|.
  static scoped_refptr<testing::NiceMock<device::MockBluetoothAdapter>>
  GetConnectableDeviceAdapter();

  // Returns an |EmptyDevice| with the following characeteristics:
  //  - |GetAddress| returns "Empty Mock Device instanceID".
  //  - |GetName| returns "Empty Mock Device name".
  //  - |GetBluetoothClass| returns 0x1F00.  "Unspecified Device Class": see
  //    bluetooth.org/en-us/specification/assigned-numbers/baseband
  //  - |GetVendorIDSource| returns |BluetoothDevice::VENDOR_ID_BLUETOOTH|.
  //  - |GetVendorID| returns 0xFFFF.
  //  - |GetProductID| returns 1.
  //  - |GetDeviceID| returns 2.
  //  - |IsPaired| returns true.
  //  - |GetUUIDs| returns a list with two UUIDs: "1800" and "1801".
  //  - |GetGattServices| returns a list with one service "Generic Access".
  //    "Generic Access" has a "Device Name" characteristic, with a value of
  //    "Empty Mock Device Name" that can be read but not written, and a
  //    "Reconnection Address" characteristic which can't be read, but can be
  //    written.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetEmptyDevice(device::MockBluetoothAdapter* adapter,
                 const std::string& device_name = "Empty Mock Device");

  // Returns a fake |ConnectableDevice| with the same characteristics as
  // |EmptyDevice| except:
  //  - |CreateGattConnection| runs success callback with a
  //    fake BluetoothGattConnection as argument.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothDevice>>
  GetConnectableDevice(device::MockBluetoothAdapter* adapter);

  // Returns a fake BluetoothGattCharacteristic with the following
  // characteristics:
  // - |GetIdentifier| returns |uuid|.
  // - |GetUUID| returns BluetoothUUID(|uuid|).
  // - |IsLocal| returns false.
  // - |GetService| returns |service|.
  // - |IsNotifying| returns false.
  static scoped_ptr<testing::NiceMock<device::MockBluetoothGattCharacteristic>>
  GetGattCharacteristic(device::MockBluetoothGattService* service,
                        const std::string& uuid);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BLUETOOTH_ADAPTER_PROVIDER_H_
