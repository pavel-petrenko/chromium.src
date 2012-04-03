# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'dependencies': [
    'browser/debugger/devtools_resources.gyp:devtools_resources',
    '../base/base.gyp:base_static',
    '../crypto/crypto.gyp:crypto',
    '../net/net.gyp:http_server',
    '../net/net.gyp:net',
    '../ppapi/ppapi_internal.gyp:ppapi_proxy',
    '../skia/skia.gyp:skia',
    '../third_party/flac/flac.gyp:libflac',
    '../third_party/speex/speex.gyp:libspeex',
    '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
    '../third_party/zlib/zlib.gyp:zlib',
    '../ui/gfx/surface/surface.gyp:surface',
    '../ui/ui.gyp:ui',
    '../ui/ui.gyp:ui_resources',
    '../webkit/support/webkit_support.gyp:webkit_resources',
    '../webkit/support/webkit_support.gyp:webkit_strings',
  ],
  'include_dirs': [
    '..',
    '<(INTERMEDIATE_DIR)',
  ],
  'sources': [
    'public/browser/access_token_store.h',
    'public/browser/browser_child_process_host.h',
    'public/browser/browser_child_process_host_delegate.cc',
    'public/browser/browser_child_process_host_delegate.h',
    'public/browser/browser_child_process_host_iterator.cc',
    'public/browser/browser_child_process_host_iterator.h',
    'public/browser/browser_context.h',
    'public/browser/browser_main_parts.h',
    'public/browser/browser_message_filter.cc',
    'public/browser/browser_message_filter.h',
    'public/browser/browser_shutdown.h',
    'public/browser/browser_thread.h',
    'public/browser/browser_thread_delegate.h',
    'public/browser/child_process_data.h',
    'public/browser/content_browser_client.h',
    'public/browser/content_ipc_logging.h',
    'public/browser/devtools_agent_host_registry.h',
    'public/browser/devtools_client_host.h',
    'public/browser/download_danger_type.h',
    'public/browser/download_id.h',
    'public/browser/devtools_frontend_host_delegate.h',
    'public/browser/devtools_http_handler.h',
    'public/browser/devtools_http_handler_delegate.h',
    'public/browser/devtools_manager.h',
    'public/browser/download_item.h',
    'public/browser/download_manager.h',
    'public/browser/download_query.cc',
    'public/browser/download_query.h',
    'public/browser/download_manager_delegate.cc',
    'public/browser/download_manager_delegate.h',
    'public/browser/favicon_status.cc',
    'public/browser/favicon_status.h',
    'public/browser/geolocation_permission_context.h',
    'public/browser/global_request_id.h',
    'public/browser/host_zoom_map.h',
    'public/browser/invalidate_type.h',
    'public/browser/javascript_dialogs.h',
    'public/browser/native_web_keyboard_event.h',
    'public/browser/navigation_controller.h',
    'public/browser/navigation_details.cc',
    'public/browser/navigation_details.h',
    'public/browser/navigation_entry.h',
    'public/browser/navigation_type.h',
    'public/browser/notification_details.h',
    'public/browser/notification_observer.h',
    'public/browser/notification_registrar.cc',
    'public/browser/notification_registrar.h',
    'public/browser/notification_service.h',
    'public/browser/notification_source.h',
    'public/browser/notification_types.h',
    'public/browser/page_navigator.cc',
    'public/browser/page_navigator.h',
    'public/browser/plugin_data_remover.h',
    'public/browser/plugin_service.h',
    'public/browser/profiler_controller.h',
    'public/browser/profiler_subscriber.h',
    'public/browser/render_process_host.h',
    'public/browser/render_process_host_factory.h',
    'public/browser/render_view_host_delegate.cc',
    'public/browser/render_view_host_delegate.h',
    'public/browser/render_view_host_observer.cc',
    'public/browser/render_view_host_observer.h',
    'public/browser/resource_dispatcher_host_delegate.cc',
    'public/browser/resource_dispatcher_host_delegate.h',
    'public/browser/resource_dispatcher_host_login_delegate.h',
    'public/browser/save_page_type.h',
    'public/browser/sensors.h',
    'public/browser/sensors_listener.h',
    'public/browser/site_instance.h',
    'public/browser/ssl_status.cc',
    'public/browser/ssl_status.h',
    'public/browser/user_metrics.h',
    'public/browser/web_contents.h',
    'public/browser/web_contents_delegate.cc',
    'public/browser/web_contents_delegate.h',
    'public/browser/web_contents_observer.cc',
    'public/browser/web_contents_observer.h',
    'public/browser/web_contents_view.h',
    'public/browser/web_intents_dispatcher.h',
    'public/browser/web_ui.h',
    'public/browser/web_ui_controller.cc',
    'public/browser/web_ui_controller.h',
    'public/browser/web_ui_controller_factory.h',
    'public/browser/web_ui_message_handler.h',
    'public/browser/worker_service.h',
    'public/browser/worker_service_observer.h',
    'browser/accessibility/browser_accessibility.cc',
    'browser/accessibility/browser_accessibility.h',
    'browser/accessibility/browser_accessibility_cocoa.h',
    'browser/accessibility/browser_accessibility_cocoa.mm',
    'browser/accessibility/browser_accessibility_delegate_mac.h',
    'browser/accessibility/browser_accessibility_mac.h',
    'browser/accessibility/browser_accessibility_mac.mm',
    'browser/accessibility/browser_accessibility_manager.cc',
    'browser/accessibility/browser_accessibility_manager.h',
    'browser/accessibility/browser_accessibility_manager_mac.h',
    'browser/accessibility/browser_accessibility_manager_mac.mm',
    'browser/accessibility/browser_accessibility_manager_win.cc',
    'browser/accessibility/browser_accessibility_manager_win.h',
    'browser/accessibility/browser_accessibility_state.cc',
    'browser/accessibility/browser_accessibility_state.h',
    'browser/accessibility/browser_accessibility_win.cc',
    'browser/accessibility/browser_accessibility_win.h',
    'browser/appcache/appcache_dispatcher_host.cc',
    'browser/appcache/appcache_dispatcher_host.h',
    'browser/appcache/appcache_frontend_proxy.cc',
    'browser/appcache/appcache_frontend_proxy.h',
    'browser/appcache/chrome_appcache_service.cc',
    'browser/appcache/chrome_appcache_service.h',
    'browser/browser_child_process_host_impl.cc',
    'browser/browser_child_process_host_impl.h',
    'browser/browser_main.cc',
    'browser/browser_main.h',
    'browser/browser_main_loop.cc',
    'browser/browser_main_loop.h',
    'browser/browser_process_sub_thread.cc',
    'browser/browser_process_sub_thread.h',
    'browser/browser_thread_impl.cc',
    'browser/browser_thread_impl.h',
    'browser/browser_url_handler.cc',
    'browser/browser_url_handler.h',
    'browser/browsing_instance.cc',
    'browser/browsing_instance.h',
    'browser/cert_store.cc',
    'browser/cert_store.h',
    'browser/child_process_launcher.cc',
    'browser/child_process_launcher.h',
    'browser/child_process_security_policy.cc',
    'browser/child_process_security_policy.h',
    'browser/chrome_blob_storage_context.cc',
    'browser/chrome_blob_storage_context.h',
    'browser/content_ipc_logging.cc',
    'browser/cross_site_request_manager.cc',
    'browser/cross_site_request_manager.h',
    'browser/debugger/devtools_agent_host.cc',
    'browser/debugger/devtools_agent_host.h',
    'browser/debugger/devtools_frontend_host.cc',
    'browser/debugger/devtools_frontend_host.h',
    'browser/debugger/devtools_http_handler_impl.cc',
    'browser/debugger/devtools_http_handler_impl.h',
    'browser/debugger/devtools_manager_impl.cc',
    'browser/debugger/devtools_manager_impl.h',
    'browser/debugger/devtools_netlog_observer.cc',
    'browser/debugger/devtools_netlog_observer.h',
    'browser/debugger/render_view_devtools_agent_host.cc',
    'browser/debugger/render_view_devtools_agent_host.h',
    'browser/debugger/worker_devtools_manager.cc',
    'browser/debugger/worker_devtools_manager.h',
    'browser/debugger/worker_devtools_message_filter.cc',
    'browser/debugger/worker_devtools_message_filter.h',
    'browser/device_orientation/accelerometer_mac.cc',
    'browser/device_orientation/accelerometer_mac.h',
    'browser/device_orientation/data_fetcher.h',
    'browser/device_orientation/message_filter.cc',
    'browser/device_orientation/message_filter.h',
    'browser/device_orientation/orientation.h',
    'browser/device_orientation/provider.cc',
    'browser/device_orientation/provider.h',
    'browser/device_orientation/provider_impl.cc',
    'browser/device_orientation/provider_impl.h',
    'browser/disposition_utils.cc',
    'browser/disposition_utils.h',
    'browser/download/base_file.cc',
    'browser/download/base_file.h',
    'browser/download/download_buffer.cc',
    'browser/download/download_buffer.h',
    'browser/download/download_create_info.cc',
    'browser/download/download_create_info.h',
    'browser/download/download_file.h',
    'browser/download/download_file_impl.cc',
    'browser/download/download_file_impl.h',
    'browser/download/download_file_manager.cc',
    'browser/download/download_file_manager.h',
    'browser/download/download_item_impl.cc',
    'browser/download/download_item_impl.h',
    'browser/download/download_manager_impl.cc',
    'browser/download/download_manager_impl.h',
    'browser/download/download_persistent_store_info.cc',
    'browser/download/download_persistent_store_info.h',
    'browser/download/download_request_handle.cc',
    'browser/download/download_request_handle.h',
    'browser/download/download_resource_handler.cc',
    'browser/download/download_resource_handler.h',
    'browser/download/download_state_info.cc',
    'browser/download/download_state_info.h',
    'browser/download/download_stats.cc',
    'browser/download/download_stats.h',
    'browser/download/download_status_updater.cc',
    'browser/download/download_status_updater.h',
    'browser/download/download_status_updater_delegate.h',
    'browser/download/download_types.cc',
    'browser/download/download_types.h',
    'browser/download/drag_download_file.cc',
    'browser/download/drag_download_file.h',
    'browser/download/drag_download_util.cc',
    'browser/download/drag_download_util.h',
    'browser/download/interrupt_reason_values.h',
    'browser/download/interrupt_reasons.cc',
    'browser/download/interrupt_reasons.h',
    'browser/download/mhtml_generation_manager.cc',
    'browser/download/mhtml_generation_manager.h',
    'browser/download/save_file.cc',
    'browser/download/save_file.h',
    'browser/download/save_file_manager.cc',
    'browser/download/save_file_manager.h',
    'browser/download/save_file_resource_handler.cc',
    'browser/download/save_file_resource_handler.h',
    'browser/download/save_item.cc',
    'browser/download/save_item.h',
    'browser/download/save_package.cc',
    'browser/download/save_package.h',
    'browser/download/save_types.cc',
    'browser/download/save_types.h',
    'browser/file_metadata_mac.h',
    'browser/file_metadata_mac.mm',
    'browser/file_system/browser_file_system_helper.cc',
    'browser/file_system/browser_file_system_helper.h',
    'browser/file_system/file_system_dispatcher_host.cc',
    'browser/file_system/file_system_dispatcher_host.h',
    'browser/find_pasteboard.h',
    'browser/find_pasteboard.mm',
    'browser/font_list_async.cc',
    'browser/font_list_async.h',
    'browser/gamepad/data_fetcher.h',
    'browser/gamepad/gamepad_provider.cc',
    'browser/gamepad/gamepad_provider.h',
    'browser/gamepad/gamepad_service.cc',
    'browser/gamepad/gamepad_service.h',
    'browser/gamepad/gamepad_standard_mappings.h',
    'browser/gamepad/gamepad_standard_mappings_linux.cc',
    'browser/gamepad/gamepad_standard_mappings_mac.mm',
    'browser/gamepad/platform_data_fetcher.h',
    'browser/gamepad/platform_data_fetcher_linux.cc',
    'browser/gamepad/platform_data_fetcher_linux.h',
    'browser/gamepad/platform_data_fetcher_mac.h',
    'browser/gamepad/platform_data_fetcher_mac.mm',
    'browser/gamepad/platform_data_fetcher_win.cc',
    'browser/gamepad/platform_data_fetcher_win.h',
    'browser/geolocation/arbitrator_dependency_factory.cc',
    'browser/geolocation/arbitrator_dependency_factory.h',
    'browser/geolocation/core_location_data_provider_mac.h',
    'browser/geolocation/core_location_data_provider_mac.mm',
    'browser/geolocation/core_location_provider_mac.h',
    'browser/geolocation/core_location_provider_mac.mm',
    'browser/geolocation/device_data_provider.cc',
    'browser/geolocation/device_data_provider.h',
    'browser/geolocation/empty_device_data_provider.cc',
    'browser/geolocation/empty_device_data_provider.h',
    'browser/geolocation/geolocation_dispatcher_host.cc',
    'browser/geolocation/geolocation_dispatcher_host.h',
    'browser/geolocation/geolocation_observer.h',
    'browser/geolocation/geolocation_provider.cc',
    'browser/geolocation/geolocation_provider.h',
    'browser/geolocation/gps_location_provider_linux.cc',
    'browser/geolocation/gps_location_provider_linux.h',
    'browser/geolocation/libgps_wrapper_linux.cc',
    'browser/geolocation/libgps_wrapper_linux.h',
    'browser/geolocation/location_arbitrator.cc',
    'browser/geolocation/location_arbitrator.h',
    'browser/geolocation/location_provider.cc',
    'browser/geolocation/location_provider.h',
    'browser/geolocation/network_location_provider.cc',
    'browser/geolocation/network_location_provider.h',
    'browser/geolocation/network_location_request.cc',
    'browser/geolocation/network_location_request.h',
    'browser/geolocation/osx_wifi.h',
    'browser/geolocation/wifi_data_provider_common.cc',
    'browser/geolocation/wifi_data_provider_common.h',
    'browser/geolocation/wifi_data_provider_common_win.cc',
    'browser/geolocation/wifi_data_provider_common_win.h',
    'browser/geolocation/wifi_data_provider_corewlan_mac.mm',
    'browser/geolocation/wifi_data_provider_linux.cc',
    'browser/geolocation/wifi_data_provider_linux.h',
    'browser/geolocation/wifi_data_provider_mac.cc',
    'browser/geolocation/wifi_data_provider_mac.h',
    'browser/geolocation/wifi_data_provider_win.cc',
    'browser/geolocation/wifi_data_provider_win.h',
    'browser/geolocation/win7_location_api_win.cc',
    'browser/geolocation/win7_location_api_win.h',
    'browser/geolocation/win7_location_provider_win.cc',
    'browser/geolocation/win7_location_provider_win.h',
    'browser/gpu/gpu_blacklist.cc',
    'browser/gpu/gpu_blacklist.h',
    'browser/gpu/gpu_data_manager.cc',
    'browser/gpu/gpu_data_manager.h',
    'browser/gpu/gpu_process_host.cc',
    'browser/gpu/gpu_process_host.h',
    'browser/gpu/gpu_process_host_ui_shim.cc',
    'browser/gpu/gpu_process_host_ui_shim.h',
    'browser/gpu/gpu_surface_tracker.cc',
    'browser/gpu/gpu_surface_tracker.h',
    'browser/host_zoom_map_impl.cc',
    'browser/host_zoom_map_impl.h',
    'browser/in_process_webkit/browser_webkitplatformsupport_impl.cc',
    'browser/in_process_webkit/browser_webkitplatformsupport_impl.h',
    'browser/in_process_webkit/dom_storage_area.cc',
    'browser/in_process_webkit/dom_storage_area.h',
    'browser/in_process_webkit/dom_storage_context.cc',
    'browser/in_process_webkit/dom_storage_context.h',
    'browser/in_process_webkit/dom_storage_message_filter.cc',
    'browser/in_process_webkit/dom_storage_message_filter.h',
    'browser/in_process_webkit/dom_storage_namespace.cc',
    'browser/in_process_webkit/dom_storage_namespace.h',
    'browser/in_process_webkit/indexed_db_callbacks.cc',
    'browser/in_process_webkit/indexed_db_callbacks.h',
    'browser/in_process_webkit/indexed_db_context.cc',
    'browser/in_process_webkit/indexed_db_context.h',
    'browser/in_process_webkit/indexed_db_database_callbacks.cc',
    'browser/in_process_webkit/indexed_db_database_callbacks.h',
    'browser/in_process_webkit/indexed_db_dispatcher_host.cc',
    'browser/in_process_webkit/indexed_db_dispatcher_host.h',
    'browser/in_process_webkit/indexed_db_key_utility_client.cc',
    'browser/in_process_webkit/indexed_db_key_utility_client.h',
    'browser/in_process_webkit/indexed_db_quota_client.cc',
    'browser/in_process_webkit/indexed_db_quota_client.h',
    'browser/in_process_webkit/indexed_db_transaction_callbacks.cc',
    'browser/in_process_webkit/indexed_db_transaction_callbacks.h',
    'browser/in_process_webkit/session_storage_namespace.cc',
    'browser/in_process_webkit/session_storage_namespace.h',
    'browser/in_process_webkit/webkit_context.cc',
    'browser/in_process_webkit/webkit_context.h',
    'browser/in_process_webkit/webkit_thread.cc',
    'browser/in_process_webkit/webkit_thread.h',
    'browser/intents/intent_injector.cc',
    'browser/intents/intent_injector.h',
    'browser/intents/web_intents_dispatcher_impl.cc',
    'browser/intents/web_intents_dispatcher_impl.h',
    'browser/load_from_memory_cache_details.cc',
    'browser/load_from_memory_cache_details.h',
    'browser/load_notification_details.h',
    'browser/mach_broker_mac.cc',
    'browser/mach_broker_mac.h',
    'browser/mime_registry_message_filter.cc',
    'browser/mime_registry_message_filter.h',
    'browser/net/browser_online_state_observer.cc',
    'browser/net/browser_online_state_observer.h',
    # TODO:  These should be moved to test_support (see below), but
    # are currently used by production code in automation_provider.cc.
    'browser/net/url_request_failed_dns_job.cc',
    'browser/net/url_request_failed_dns_job.h',
    'browser/net/url_request_mock_http_job.cc',
    'browser/net/url_request_mock_http_job.h',
    'browser/net/url_request_slow_download_job.cc',
    'browser/net/url_request_slow_download_job.h',
    'browser/net/url_request_slow_http_job.cc',
    'browser/net/url_request_slow_http_job.h',
    'browser/notification_service_impl.cc',
    'browser/notification_service_impl.h',
    'browser/plugin_data_remover_impl.cc',
    'browser/plugin_data_remover_impl.h',
    'browser/plugin_loader_posix.cc',
    'browser/plugin_loader_posix.h',
    'browser/plugin_process_host.cc',
    'browser/plugin_process_host.h',
    'browser/plugin_process_host_mac.cc',
    'browser/plugin_service_impl.cc',
    'browser/plugin_service_impl.h',
    'browser/plugin_service_filter.h',
    'browser/power_save_blocker.h',
    'browser/power_save_blocker_common.cc',
    'browser/power_save_blocker_linux.cc',
    'browser/power_save_blocker_mac.cc',
    'browser/power_save_blocker_win.cc',
    'browser/ppapi_plugin_process_host.cc',
    'browser/ppapi_plugin_process_host.h',
    'browser/profiler_controller_impl.cc',
    'browser/profiler_controller_impl.h',
    'browser/profiler_message_filter.cc',
    'browser/profiler_message_filter.h',
    'browser/quota_permission_context.h',
    'browser/renderer_host/accelerated_plugin_view_mac.h',
    'browser/renderer_host/accelerated_plugin_view_mac.mm',
    'browser/renderer_host/accelerated_surface_container_linux.cc',
    'browser/renderer_host/accelerated_surface_container_linux.h',
    'browser/renderer_host/accelerated_surface_container_mac.cc',
    'browser/renderer_host/accelerated_surface_container_mac.h',
    'browser/renderer_host/accelerated_surface_container_manager_mac.cc',
    'browser/renderer_host/accelerated_surface_container_manager_mac.h',
    'browser/renderer_host/async_resource_handler.cc',
    'browser/renderer_host/async_resource_handler.h',
    'browser/renderer_host/backing_store.cc',
    'browser/renderer_host/backing_store.h',
    'browser/renderer_host/backing_store_gtk.cc',
    'browser/renderer_host/backing_store_gtk.h',
    'browser/renderer_host/backing_store_mac.h',
    'browser/renderer_host/backing_store_mac.mm',
    'browser/renderer_host/backing_store_manager.cc',
    'browser/renderer_host/backing_store_manager.h',
    'browser/renderer_host/backing_store_skia.cc',
    'browser/renderer_host/backing_store_skia.h',
    'browser/renderer_host/backing_store_win.cc',
    'browser/renderer_host/backing_store_win.h',
    'browser/renderer_host/blob_message_filter.cc',
    'browser/renderer_host/blob_message_filter.h',
    'browser/renderer_host/buffered_resource_handler.cc',
    'browser/renderer_host/buffered_resource_handler.h',
    'browser/renderer_host/clipboard_message_filter.cc',
    'browser/renderer_host/clipboard_message_filter.h',
    'browser/renderer_host/clipboard_message_filter_mac.mm',
    'browser/renderer_host/cross_site_resource_handler.cc',
    'browser/renderer_host/cross_site_resource_handler.h',
    'browser/renderer_host/database_message_filter.cc',
    'browser/renderer_host/database_message_filter.h',
    'browser/renderer_host/doomed_resource_handler.cc',
    'browser/renderer_host/doomed_resource_handler.h',
    'browser/renderer_host/file_utilities_message_filter.cc',
    'browser/renderer_host/file_utilities_message_filter.h',
    'browser/renderer_host/gamepad_browser_message_filter.cc',
    'browser/renderer_host/gamepad_browser_message_filter.h',
    'browser/renderer_host/gpu_message_filter.cc',
    'browser/renderer_host/gpu_message_filter.h',
    'browser/renderer_host/gtk_im_context_wrapper.cc',
    'browser/renderer_host/gtk_im_context_wrapper.h',
    'browser/renderer_host/gtk_key_bindings_handler.cc',
    'browser/renderer_host/gtk_key_bindings_handler.h',
    'browser/renderer_host/gtk_window_utils.cc',
    'browser/renderer_host/gtk_window_utils.h',
    'browser/renderer_host/image_transport_client.cc',
    'browser/renderer_host/image_transport_client.h',
    'browser/renderer_host/java/java_bound_object.cc',
    'browser/renderer_host/java/java_bound_object.h',
    'browser/renderer_host/java/java_bridge_channel_host.cc',
    'browser/renderer_host/java/java_bridge_channel_host.h',
    'browser/renderer_host/java/java_bridge_dispatcher_host_manager.cc',
    'browser/renderer_host/java/java_bridge_dispatcher_host_manager.h',
    'browser/renderer_host/java/java_bridge_dispatcher_host.cc',
    'browser/renderer_host/java/java_bridge_dispatcher_host.h',
    'browser/renderer_host/java/java_method.cc',
    'browser/renderer_host/java/java_method.h',
    'browser/renderer_host/java/java_type.cc',
    'browser/renderer_host/java/java_type.h',
    'browser/renderer_host/layered_resource_handler.cc',
    'browser/renderer_host/layered_resource_handler.h',
    'browser/renderer_host/media/audio_input_device_manager.cc',
    'browser/renderer_host/media/audio_input_device_manager.h',
    'browser/renderer_host/media/audio_input_device_manager_event_handler.h',
    'browser/renderer_host/media/audio_input_renderer_host.cc',
    'browser/renderer_host/media/audio_input_renderer_host.h',
    'browser/renderer_host/media/audio_input_sync_writer.cc',
    'browser/renderer_host/media/audio_input_sync_writer.h',
    'browser/renderer_host/media/audio_renderer_host.cc',
    'browser/renderer_host/media/audio_renderer_host.h',
    'browser/renderer_host/media/audio_sync_reader.cc',
    'browser/renderer_host/media/audio_sync_reader.h',
    'browser/renderer_host/media/media_stream_device_settings.cc',
    'browser/renderer_host/media/media_stream_device_settings.h',
    'browser/renderer_host/media/media_stream_dispatcher_host.cc',
    'browser/renderer_host/media/media_stream_dispatcher_host.h',
    'browser/renderer_host/media/media_stream_manager.cc',
    'browser/renderer_host/media/media_stream_manager.h',
    'browser/renderer_host/media/media_stream_provider.h',
    'browser/renderer_host/media/media_stream_requester.h',
    'browser/renderer_host/media/media_stream_settings_requester.h',
    'browser/renderer_host/media/video_capture_controller.cc',
    'browser/renderer_host/media/video_capture_controller.h',
    'browser/renderer_host/media/video_capture_controller_event_handler.cc',
    'browser/renderer_host/media/video_capture_controller_event_handler.h',
    'browser/renderer_host/media/video_capture_host.cc',
    'browser/renderer_host/media/video_capture_host.h',
    'browser/renderer_host/media/video_capture_manager.cc',
    'browser/renderer_host/media/video_capture_manager.h',
    'browser/renderer_host/native_web_keyboard_event_aura.cc',
    'browser/renderer_host/native_web_keyboard_event_gtk.cc',
    'browser/renderer_host/native_web_keyboard_event_mac.mm',
    'browser/renderer_host/native_web_keyboard_event_win.cc',
    'browser/renderer_host/pepper_file_message_filter.cc',
    'browser/renderer_host/pepper_file_message_filter.h',
    'browser/renderer_host/pepper_message_filter.cc',
    'browser/renderer_host/pepper_message_filter.h',
    'browser/renderer_host/pepper_tcp_socket.cc',
    'browser/renderer_host/pepper_tcp_socket.h',
    'browser/renderer_host/pepper_udp_socket.cc',
    'browser/renderer_host/pepper_udp_socket.h',
    'browser/renderer_host/quota_dispatcher_host.cc',
    'browser/renderer_host/quota_dispatcher_host.h',
    'browser/renderer_host/redirect_to_file_resource_handler.cc',
    'browser/renderer_host/redirect_to_file_resource_handler.h',
    'browser/renderer_host/render_message_filter.cc',
    'browser/renderer_host/render_message_filter.h',
    'browser/renderer_host/render_message_filter_win.cc',
    'browser/renderer_host/render_process_host_impl.cc',
    'browser/renderer_host/render_process_host_impl.h',
    'browser/renderer_host/render_sandbox_host_linux.cc',
    'browser/renderer_host/render_sandbox_host_linux.h',
    'browser/renderer_host/render_view_host.cc',
    'browser/renderer_host/render_view_host.h',
    'browser/renderer_host/render_view_host_factory.cc',
    'browser/renderer_host/render_view_host_factory.h',
    'browser/renderer_host/render_widget_helper.cc',
    'browser/renderer_host/render_widget_helper.h',
    'browser/renderer_host/render_widget_host.cc',
    'browser/renderer_host/render_widget_host.h',
    'browser/renderer_host/render_widget_host_gtk.cc',
    'browser/renderer_host/render_widget_host_mac.cc',
    'browser/renderer_host/render_widget_host_view.cc',
    'browser/renderer_host/render_widget_host_view.h',
    'browser/renderer_host/render_widget_host_view_aura.cc',
    'browser/renderer_host/render_widget_host_view_aura.h',
    'browser/renderer_host/render_widget_host_view_gtk.cc',
    'browser/renderer_host/render_widget_host_view_gtk.h',
    'browser/renderer_host/render_widget_host_view_mac.h',
    'browser/renderer_host/render_widget_host_view_mac.mm',
    'browser/renderer_host/render_widget_host_view_mac_delegate.h',
    'browser/renderer_host/render_widget_host_view_mac_editcommand_helper.h',
    'browser/renderer_host/render_widget_host_view_mac_editcommand_helper.mm',
    'browser/renderer_host/render_widget_host_view_win.cc',
    'browser/renderer_host/render_widget_host_view_win.h',
    'browser/renderer_host/resource_dispatcher_host.cc',
    'browser/renderer_host/resource_dispatcher_host.h',
    'browser/renderer_host/resource_dispatcher_host_request_info.cc',
    'browser/renderer_host/resource_dispatcher_host_request_info.h',
    'browser/renderer_host/resource_handler.h',
    'browser/renderer_host/resource_message_filter.cc',
    'browser/renderer_host/resource_message_filter.h',
    'browser/renderer_host/resource_queue.cc',
    'browser/renderer_host/resource_queue.h',
    'browser/renderer_host/resource_request_details.cc',
    'browser/renderer_host/resource_request_details.h',
    'browser/renderer_host/socket_stream_dispatcher_host.cc',
    'browser/renderer_host/socket_stream_dispatcher_host.h',
    'browser/renderer_host/socket_stream_host.cc',
    'browser/renderer_host/socket_stream_host.h',
    'browser/renderer_host/sync_resource_handler.cc',
    'browser/renderer_host/sync_resource_handler.h',
    'browser/renderer_host/text_input_client_mac.h',
    'browser/renderer_host/text_input_client_mac.mm',
    'browser/renderer_host/text_input_client_message_filter.h',
    'browser/renderer_host/text_input_client_message_filter.mm',
    'browser/renderer_host/web_input_event_aura.cc',
    'browser/renderer_host/web_input_event_aura.h',
    'browser/renderer_host/web_input_event_aurawin.cc',
    'browser/renderer_host/web_input_event_aurax11.cc',
    'browser/renderer_host/x509_user_cert_resource_handler.cc',
    'browser/renderer_host/x509_user_cert_resource_handler.h',
    'browser/resolve_proxy_msg_helper.cc',
    'browser/resolve_proxy_msg_helper.h',
    'browser/resource_context.cc',
    'browser/resource_context.h',
    'browser/safe_util_win.cc',
    'browser/safe_util_win.h',
    'browser/sensors/sensors_provider.cc',
    'browser/sensors/sensors_provider.h',
    'browser/sensors/sensors_provider_impl.cc',
    'browser/sensors/sensors_provider_impl.h',
    'browser/site_instance_impl.cc',
    'browser/site_instance_impl.h',
    'browser/speech/audio_encoder.cc',
    'browser/speech/audio_encoder.h',
    'browser/speech/endpointer/endpointer.cc',
    'browser/speech/endpointer/endpointer.h',
    'browser/speech/endpointer/energy_endpointer.cc',
    'browser/speech/endpointer/energy_endpointer.h',
    'browser/speech/endpointer/energy_endpointer_params.cc',
    'browser/speech/endpointer/energy_endpointer_params.h',
    'browser/speech/speech_input_dispatcher_host.cc',
    'browser/speech/speech_input_dispatcher_host.h',
    'browser/speech/speech_input_manager.cc',
    'browser/speech/speech_input_manager.h',
    'browser/speech/speech_input_preferences.cc',
    'browser/speech/speech_input_preferences.h',
    'browser/speech/speech_recognition_request.cc',
    'browser/speech/speech_recognition_request.h',
    'browser/speech/speech_recognizer.cc',
    'browser/speech/speech_recognizer.h',
    'browser/ssl/ssl_cert_error_handler.cc',
    'browser/ssl/ssl_cert_error_handler.h',
    'browser/ssl/ssl_client_auth_handler.cc',
    'browser/ssl/ssl_client_auth_handler.h',
    'browser/ssl/ssl_client_auth_notification_details.cc',
    'browser/ssl/ssl_client_auth_notification_details.h',
    'browser/ssl/ssl_error_handler.cc',
    'browser/ssl/ssl_error_handler.h',
    'browser/ssl/ssl_host_state.cc',
    'browser/ssl/ssl_host_state.h',
    'browser/ssl/ssl_manager.cc',
    'browser/ssl/ssl_manager.h',
    'browser/ssl/ssl_policy.cc',
    'browser/ssl/ssl_policy.h',
    'browser/ssl/ssl_policy_backend.cc',
    'browser/ssl/ssl_policy_backend.h',
    'browser/ssl/ssl_request_info.cc',
    'browser/ssl/ssl_request_info.h',
    'browser/system_message_window_win.cc',
    'browser/system_message_window_win.h',
    'browser/tab_contents/drag_utils_gtk.cc',
    'browser/tab_contents/drag_utils_gtk.h',
    'browser/tab_contents/interstitial_page.cc',
    'browser/tab_contents/interstitial_page.h',
    'browser/tab_contents/navigation_controller_impl.cc',
    'browser/tab_contents/navigation_controller_impl.h',
    'browser/tab_contents/navigation_entry_impl.cc',
    'browser/tab_contents/navigation_entry_impl.h',
    'browser/tab_contents/popup_menu_helper_mac.h',
    'browser/tab_contents/popup_menu_helper_mac.mm',
    'browser/tab_contents/provisional_load_details.cc',
    'browser/tab_contents/provisional_load_details.h',
    'browser/tab_contents/render_view_host_manager.cc',
    'browser/tab_contents/render_view_host_manager.h',
    'browser/tab_contents/tab_contents.cc',
    'browser/tab_contents/tab_contents.h',
    'browser/tab_contents/tab_contents_view_gtk.cc',
    'browser/tab_contents/tab_contents_view_gtk.h',
    'browser/tab_contents/tab_contents_view_helper.cc',
    'browser/tab_contents/tab_contents_view_helper.h',
    'browser/tab_contents/tab_contents_view_wrapper_gtk.h',
    'browser/tab_contents/title_updated_details.h',
    'browser/tab_contents/web_drag_dest_delegate.h',
    'browser/tab_contents/web_drag_dest_gtk.cc',
    'browser/tab_contents/web_drag_dest_gtk.h',
    'browser/tab_contents/web_drag_dest_mac.h',
    'browser/tab_contents/web_drag_dest_mac.mm',
    'browser/tab_contents/web_drag_source_gtk.cc',
    'browser/tab_contents/web_drag_source_gtk.h',
    'browser/tab_contents/web_drag_source_mac.h',
    'browser/tab_contents/web_drag_source_mac.mm',
    'browser/trace_controller.cc',
    'browser/trace_controller.h',
    'browser/trace_message_filter.cc',
    'browser/trace_message_filter.h',
    'browser/trace_subscriber_stdio.cc',
    'browser/trace_subscriber_stdio.h',
    'browser/user_metrics.cc',
    'browser/utility_process_host.cc',
    'browser/utility_process_host.h',
    'browser/webui/generic_handler.cc',
    'browser/webui/generic_handler.h',
    'browser/webui/web_ui_impl.cc',
    'browser/webui/web_ui_impl.h',
    'browser/webui/web_ui_message_handler.cc',
    'browser/worker_host/message_port_service.cc',
    'browser/worker_host/message_port_service.h',
    'browser/worker_host/worker_document_set.cc',
    'browser/worker_host/worker_document_set.h',
    'browser/worker_host/worker_message_filter.cc',
    'browser/worker_host/worker_message_filter.h',
    'browser/worker_host/worker_process_host.cc',
    'browser/worker_host/worker_process_host.h',
    'browser/worker_host/worker_service_impl.cc',
    'browser/worker_host/worker_service_impl.h',
    'browser/zygote_host_linux.cc',
    'browser/zygote_host_linux.h',
    'browser/zygote_main_linux.cc',
    '<(SHARED_INTERMEDIATE_DIR)/webkit/grit/devtools_resources.h',
    '<(SHARED_INTERMEDIATE_DIR)/webkit/grit/devtools_resources_map.cc',
    '<(SHARED_INTERMEDIATE_DIR)/webkit/grit/devtools_resources_map.h',
  ],
  'conditions': [
    ['OS!="win" and OS!="mac" and OS!="linux"', {
      'sources': [
        'browser/gamepad/platform_data_fetcher.cc',
      ]
    }],
    ['p2p_apis==1', {
      'sources': [
        'browser/renderer_host/p2p/socket_host.cc',
        'browser/renderer_host/p2p/socket_host.h',
        'browser/renderer_host/p2p/socket_host_tcp.cc',
        'browser/renderer_host/p2p/socket_host_tcp.h',
        'browser/renderer_host/p2p/socket_host_tcp_server.cc',
        'browser/renderer_host/p2p/socket_host_tcp_server.h',
        'browser/renderer_host/p2p/socket_host_udp.cc',
        'browser/renderer_host/p2p/socket_host_udp.h',
        'browser/renderer_host/p2p/socket_dispatcher_host.cc',
        'browser/renderer_host/p2p/socket_dispatcher_host.h',
      ],
    }],
    ['OS=="win"', {
      'dependencies': [
        # For accessibility
        '../third_party/iaccessible2/iaccessible2.gyp:iaccessible2',
        '../third_party/isimpledom/isimpledom.gyp:isimpledom',
      ],
      'link_settings': {
        'libraries': [
          '-lcomctl32.lib',
          '-llocationapi.lib',
          '-lsensorsapi.lib',
        ],
        'msvs_settings': {
          'VCLinkerTool': {
            'DelayLoadDLLs': [
              'user32.dll',
              'xinput1_3.dll',
            ],
          },
        },
      },
    }],
    ['toolkit_uses_gtk == 1', {
      'dependencies': [
        '../build/linux/system.gyp:dbus',
        # For FcLangSetAdd call in render_sandbox_host_linux.cc
        '../build/linux/system.gyp:fontconfig',
        '../build/linux/system.gyp:gtk',
        # For XShm* in backing_store_x.cc
        '../build/linux/system.gyp:x11',
        '../dbus/dbus.gyp:dbus',
      ],
      'conditions': [
        ['linux_sandbox_path != ""', {
          'defines': [
            'LINUX_SANDBOX_PATH="<(linux_sandbox_path)"',
          ],
        }],
      ],
    }],
    ['OS=="linux"', {
      'dependencies': [
        '../build/linux/system.gyp:udev',
      ],
    }],
    ['OS=="linux" and toolkit_uses_gtk==0', {
      'dependencies': [
        '../build/linux/system.gyp:dbus',
        '../build/linux/system.gyp:fontconfig',
        '../build/linux/system.gyp:x11',
        '../dbus/dbus.gyp:dbus',
      ],
    }],
    ['OS=="android"', {
      'sources!': [
        # A main loop would be handled in Java.
        'browser/browser_main_loop.cc',
        'browser/browser_main_loop.h',
      ]}],
    ['OS=="mac"', {
      'sources': [
        # Build necessary Mozilla sources
        '../third_party/mozilla/ComplexTextInputPanel.h',
        '../third_party/mozilla/ComplexTextInputPanel.mm',
        '../third_party/mozilla/NSPasteboard+Utils.h',
        '../third_party/mozilla/NSPasteboard+Utils.mm',
      ],
      'dependencies': [
        'closure_blocks_leopard_compat',
      ],
    }, { # OS!="mac"
      'dependencies': [
        '../sandbox/sandbox.gyp:sandbox',
      ],
    }],
    ['chromeos==1', {
      'dependencies': [
        '../build/linux/system.gyp:dbus-glib',
      ],
      'sources!': [
        'browser/renderer_host/gtk_key_bindings_handler.cc',
        'browser/renderer_host/gtk_key_bindings_handler.h',
      ],
    }],
    ['os_bsd==1', {
      'sources/': [
        ['exclude', '^browser/gamepad/platform_data_fetcher_linux\\.cc$'],
        ['exclude', '^browser/geolocation/wifi_data_provider_linux\\.cc$'],
      ],
    }],
    ['use_aura==1', {
      'dependencies': [
        '../ui/aura/aura.gyp:aura',
      ],
      'sources/': [
        ['exclude', '^browser/accessibility/browser_accessibility_manager_win.cc'],
        ['exclude', '^browser/accessibility/browser_accessibility_manager_win.h'],
        ['exclude', '^browser/accessibility/browser_accessibility_win.cc'],
        ['exclude', '^browser/accessibility/browser_accessibility_win.h'],
        ['exclude', '^browser/renderer_host/gtk_im_context_wrapper.cc'],
        ['exclude', '^browser/renderer_host/gtk_im_context_wrapper.h'],
        ['exclude', '^browser/renderer_host/native_web_keyboard_event_win.cc'],
        ['exclude', '^browser/renderer_host/render_widget_host.h'],
        ['exclude', '^browser/renderer_host/render_widget_host_view_win.cc'],
        ['exclude', '^browser/renderer_host/render_widget_host_view_win.h'],
        ['exclude', '^browser/renderer_host/render_message_filter_win.cc'],
      ],
    }, {
      'sources/': [
        ['exclude', '^browser/renderer_host/render_widget_host_view_aura.cc'],
        ['exclude', '^browser/renderer_host/render_widget_host_view_aura.h'],
      ],
    }],
    ['ui_compositor_image_transport==1', {
      'dependencies': [
        '../ui/gfx/compositor/compositor.gyp:compositor',
        '../ui/gfx/gl/gl.gyp:gl',
      ],
      'link_settings': {
        'libraries': [
          '-lXcomposite',
        ],
      },
      'include_dirs': [
        '../third_party/angle/include',
      ],
    }, {
      'sources/': [
        ['exclude', '^browser/renderer_host/accelerated_surface_container_linux.cc'],
        ['exclude', '^browser/renderer_host/accelerated_surface_container_linux.h'],
        ['exclude', '^browser/renderer_host/image_transport_client.cc'],
        ['exclude', '^browser/renderer_host/image_transport_client.h'],
      ],
    }],
    ['java_bridge==1', {
      'defines': [
        'ENABLE_JAVA_BRIDGE',
      ],
    }, {
      'sources/': [
        ['exclude', '^browser/renderer_host/java/'],
      ],
    }],
  ],
}

