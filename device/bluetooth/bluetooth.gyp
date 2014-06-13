# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'device_bluetooth',
      'type': 'static_library',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../net/net.gyp:net',
        '../../third_party/libxml/libxml.gyp:libxml',
        '../../ui/base/ui_base.gyp:ui_base',
        '../../ui/gfx/gfx.gyp:gfx',
        '../../ui/gfx/gfx.gyp:gfx_geometry',
        'bluetooth_strings.gyp:device_bluetooth_strings',
      ],
      'sources': [
        'bluetooth_adapter.cc',
        'bluetooth_adapter.h',
        'bluetooth_adapter_chromeos.cc',
        'bluetooth_adapter_chromeos.h',
        'bluetooth_adapter_factory.cc',
        'bluetooth_adapter_factory.h',
        'bluetooth_adapter_mac.h',
        'bluetooth_adapter_mac.mm',
        'bluetooth_adapter_win.cc',
        'bluetooth_adapter_win.h',
        'bluetooth_channel_mac.mm',
        'bluetooth_channel_mac.h',
        'bluetooth_device.cc',
        'bluetooth_device.h',
        'bluetooth_device_chromeos.cc',
        'bluetooth_device_chromeos.h',
        'bluetooth_device_mac.h',
        'bluetooth_device_mac.mm',
        'bluetooth_device_win.cc',
        'bluetooth_device_win.h',
        'bluetooth_discovery_session.cc',
        'bluetooth_discovery_session.h',
        'bluetooth_gatt_characteristic.cc',
        'bluetooth_gatt_characteristic.h',
        'bluetooth_gatt_descriptor.cc',
        'bluetooth_gatt_descriptor.h',
        'bluetooth_gatt_service.cc',
        'bluetooth_gatt_service.h',
        'bluetooth_init_win.cc',
        'bluetooth_init_win.h',
        'bluetooth_pairing_chromeos.cc',
        'bluetooth_pairing_chromeos.h',
        'bluetooth_remote_gatt_characteristic_chromeos.cc',
        'bluetooth_remote_gatt_characteristic_chromeos.h',
        'bluetooth_remote_gatt_descriptor_chromeos.cc',
        'bluetooth_remote_gatt_descriptor_chromeos.h',
        'bluetooth_remote_gatt_service_chromeos.cc',
        'bluetooth_remote_gatt_service_chromeos.h',
        'bluetooth_rfcomm_channel_mac.mm',
        'bluetooth_rfcomm_channel_mac.h',
        'bluetooth_service_record_win.cc',
        'bluetooth_service_record_win.h',
        'bluetooth_socket.cc',
        'bluetooth_socket.h',
        'bluetooth_socket_chromeos.cc',
        'bluetooth_socket_chromeos.h',
        'bluetooth_socket_mac.h',
        'bluetooth_socket_mac.mm',
        'bluetooth_socket_net.cc',
        'bluetooth_socket_net.h',
        'bluetooth_socket_thread.cc',
        'bluetooth_socket_thread.h',
        'bluetooth_socket_win.cc',
        'bluetooth_socket_win.h',
        'bluetooth_task_manager_win.cc',
        'bluetooth_task_manager_win.h',
        'bluetooth_uuid.cc',
        'bluetooth_uuid.h',
      ],
      'conditions': [
        ['chromeos==1', {
          'dependencies': [
            '../../build/linux/system.gyp:dbus',
            '../../chromeos/chromeos.gyp:chromeos',
            '../../dbus/dbus.gyp:dbus',
          ]
        }],
        ['OS=="win"', {
          'all_dependent_settings': {
            'msvs_settings': {
              'VCLinkerTool': {
                'DelayLoadDLLs': [
                  # Despite MSDN stating that Bthprops.dll contains the
                  # symbols declared by bthprops.lib, they actually reside here:
                  'Bthprops.cpl',
                ],
              },
            },
          },
        }],
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/IOBluetooth.framework',
            ],
          },
        }],
      ],
    },
    {
      'target_name': 'device_bluetooth_mocks',
      'type': 'static_library',
      'dependencies': [
        '../../testing/gmock.gyp:gmock',
        'device_bluetooth',
      ],
      'include_dirs': [
        '../../',
      ],
      'sources': [
        'test/mock_bluetooth_adapter.cc',
        'test/mock_bluetooth_adapter.h',
        'test/mock_bluetooth_device.cc',
        'test/mock_bluetooth_device.h',
        'test/mock_bluetooth_discovery_session.cc',
        'test/mock_bluetooth_discovery_session.h',
        'test/mock_bluetooth_gatt_characteristic.cc',
        'test/mock_bluetooth_gatt_characteristic.h',
        'test/mock_bluetooth_gatt_descriptor.cc',
        'test/mock_bluetooth_gatt_descriptor.h',
        'test/mock_bluetooth_gatt_service.cc',
        'test/mock_bluetooth_gatt_service.h',
        'test/mock_bluetooth_socket.cc',
        'test/mock_bluetooth_socket.h',
      ],
    },
  ],
}
