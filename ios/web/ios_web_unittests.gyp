# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'ios_web_unittests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
        '../../net/net.gyp:net_test_support',
        '../../testing/gmock.gyp:gmock',
        '../../testing/gtest.gyp:gtest',
        '../../third_party/ocmock/ocmock.gyp:ocmock',
        '../../ui/base/ui_base.gyp:ui_base_test_support',
        '../testing/ios_testing.gyp:ocmock_support',
        'ios_web.gyp:ios_web',
        'ios_web.gyp:test_support_ios_web',
      ],
      'sources': [
        'active_state_manager_impl_unittest.mm',
        'alloc_with_zone_interceptor_unittest.mm',
        'browser_state_unittest.cc',
        'browsing_data_partition_impl_unittest.mm',
        'crw_browsing_data_store_unittest.mm',
        'crw_network_activity_indicator_manager_unittest.mm',
        'history_state_util_unittest.mm',
        'navigation/crw_session_controller_unittest.mm',
        'navigation/crw_session_entry_unittest.mm',
        'navigation/navigation_item_impl_unittest.mm',
        'navigation/navigation_manager_impl_unittest.mm',
        'navigation/nscoder_util_unittest.mm',
        'net/cert_policy_unittest.cc',
        'net/cert_verifier_block_adapter_unittest.cc',
        'net/clients/crw_csp_network_client_unittest.mm',
        'net/clients/crw_js_injection_network_client_unittest.mm',
        'net/clients/crw_passkit_network_client_unittest.mm',
        'net/crw_cert_policy_cache_unittest.mm',
        'net/crw_url_verifying_protocol_handler_unittest.mm',
        'net/request_group_util_unittest.mm',
        'net/request_tracker_impl_unittest.mm',
        'net/web_http_protocol_handler_delegate_unittest.mm',
        'public/referrer_util_unittest.cc',
        'public/test/http_server_unittest.mm',
        'string_util_unittest.cc',
        'test/crw_fake_web_controller_observer_unittest.mm',
        'test/run_all_unittests.cc',
        'ui_web_view_util_unittest.mm',
        'url_scheme_util_unittest.mm',
        'url_util_unittest.cc',
        'weak_nsobject_counter_unittest.mm',
        'web_state/crw_web_view_scroll_view_proxy_unittest.mm',
        'web_state/js/common_js_unittest.mm',
        'web_state/js/core_js_unittest.mm',
        'web_state/js/credential_util_unittest.mm',
        'web_state/js/crw_js_early_script_manager_unittest.mm',
        'web_state/js/crw_js_injection_manager_unittest.mm',
        'web_state/js/crw_js_invoke_parameter_queue_unittest.mm',
        'web_state/js/crw_js_window_id_manager_unittest.mm',
        'web_state/js/page_script_util_unittest.mm',
        'web_state/ui/crw_static_file_web_view_unittest.mm',
        'web_state/ui/crw_ui_simple_web_view_controller_unittest.mm',
        'web_state/ui/crw_web_controller_container_view_unittest.mm',
        'web_state/ui/crw_web_controller_observer_unittest.mm',
        'web_state/ui/crw_web_controller_unittest.mm',
        'web_state/ui/crw_wk_simple_web_view_controller_unittest.mm',
        'web_state/ui/crw_wk_web_view_crash_detector_unittest.mm',
        'web_state/ui/web_view_js_utils_unittest.mm',
        'web_state/ui/wk_back_forward_list_item_holder_unittest.mm',
        'web_state/ui/wk_web_view_configuration_provider_unittest.mm',
        'web_state/web_state_impl_unittest.mm',
        'web_state/web_view_internal_creation_util_unittest.mm',
        'web_state/wk_web_view_security_util_unittest.mm',
        'web_view_counter_impl_unittest.mm',
        'webui/crw_web_ui_manager_unittest.mm',
        'webui/crw_web_ui_page_builder_unittest.mm',
        'webui/url_fetcher_block_adapter_unittest.mm',
      ],
      'actions': [
        {
          'action_name': 'copy_test_data',
          'variables': {
            'test_data_files': [
              'test/data/chrome.html',
              'test/data/testbadpass.pkpass',
              'test/data/testfavicon.png',
              'test/data/testpass.pkpass',
            ],
            'test_data_prefix': 'ios/web',
          },
          'includes': [ '../../build/copy_test_data_ios.gypi' ],
        },
      ],
    },
  ],
}
