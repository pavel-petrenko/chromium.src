# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'variables': {
      'base_target': 0,
      'base_i18n_target': 0,
    },
    'target_conditions': [
      # This part is shared between the targets defined below.
      ['base_target==1', {
        'sources': [
          '../build/build_config.h',
          'third_party/dmg_fp/dmg_fp.h',
          'third_party/dmg_fp/g_fmt.cc',
          'third_party/dmg_fp/dtoa_wrapper.cc',
          'third_party/icu/icu_utf.cc',
          'third_party/icu/icu_utf.h',
          'third_party/nspr/prtime.cc',
          'third_party/nspr/prtime.h',
          'third_party/superfasthash/superfasthash.c',
          'third_party/xdg_mime/xdgmime.h',
          'allocator/allocator_extension.cc',
          'allocator/allocator_extension.h',
          'allocator/type_profiler_control.cc',
          'allocator/type_profiler_control.h',
          'android/application_status_listener.cc',
          'android/application_status_listener.h',
          'android/base_jni_registrar.cc',
          'android/base_jni_registrar.h',
          'android/build_info.cc',
          'android/build_info.h',
          'android/command_line_android.cc',
          'android/command_line_android.h',
          'android/content_uri_utils.cc',
          'android/content_uri_utils.h',
          'android/cpu_features.cc',
          'android/event_log.cc',
          'android/event_log.h',
          'android/field_trial_list.cc',
          'android/field_trial_list.h',
          'android/fifo_utils.cc',
          'android/fifo_utils.h',
          'android/important_file_writer_android.cc',
          'android/important_file_writer_android.h',
          'android/scoped_java_ref.cc',
          'android/scoped_java_ref.h',
          'android/jni_android.cc',
          'android/jni_android.h',
          'android/jni_array.cc',
          'android/jni_array.h',
          'android/jni_registrar.cc',
          'android/jni_registrar.h',
          'android/jni_string.cc',
          'android/jni_string.h',
          'android/jni_weak_ref.cc',
          'android/jni_weak_ref.h',
          'android/library_loader/library_loader_hooks.cc',
          'android/library_loader/library_loader_hooks.h',
          'android/memory_pressure_listener_android.cc',
          'android/memory_pressure_listener_android.h',
          'android/java_handler_thread.cc',
          'android/java_handler_thread.h',
          'android/path_service_android.cc',
          'android/path_service_android.h',
          'android/path_utils.cc',
          'android/path_utils.h',
          'android/sys_utils.cc',
          'android/sys_utils.h',
          'android/thread_utils.h',
          'android/trace_event_binding.cc',
          'android/trace_event_binding.h',
          'at_exit.cc',
          'at_exit.h',
          'atomic_ref_count.h',
          'atomic_sequence_num.h',
          'atomicops.h',
          'atomicops_internals_gcc.h',
          'atomicops_internals_mac.h',
          'atomicops_internals_tsan.h',
          'atomicops_internals_x86_gcc.cc',
          'atomicops_internals_x86_gcc.h',
          'atomicops_internals_x86_msvc.h',
          'barrier_closure.cc',
          'barrier_closure.h',
          'base64.cc',
          'base64.h',
          'base_export.h',
          'base_paths.cc',
          'base_paths.h',
          'base_paths_android.cc',
          'base_paths_android.h',
          'base_paths_mac.h',
          'base_paths_mac.mm',
          'base_paths_posix.cc',
          'base_paths_posix.h',
          'base_paths_win.cc',
          'base_paths_win.h',
          'base_switches.h',
          'basictypes.h',
          'big_endian.cc',
          'big_endian.h',
          'bind.h',
          'bind_helpers.cc',
          'bind_helpers.h',
          'bind_internal.h',
          'bind_internal_win.h',
          'bits.h',
          'build_time.cc',
          'build_time.h',
          'callback.h',
          'callback_helpers.cc',
          'callback_helpers.h',
          'callback_internal.cc',
          'callback_internal.h',
          'callback_list.h',
          'cancelable_callback.h',
          'command_line.cc',
          'command_line.h',
          'compiler_specific.h',
          'containers/hash_tables.h',
          'containers/linked_list.h',
          'containers/mru_cache.h',
          'containers/scoped_ptr_hash_map.h',
          'containers/small_map.h',
          'containers/stack_container.h',
          'cpu.cc',
          'cpu.h',
          'critical_closure.h',
          'critical_closure_internal_ios.mm',
          'debug/alias.cc',
          'debug/alias.h',
          'debug/asan_invalid_access.cc',
          'debug/asan_invalid_access.h',
          'debug/crash_logging.cc',
          'debug/crash_logging.h',
          'debug/debugger.cc',
          'debug/debugger.h',
          'debug/debugger_posix.cc',
          'debug/debugger_win.cc',
          'debug/dump_without_crashing.cc',
          'debug/dump_without_crashing.h',
          'debug/gdi_debug_util_win.cc',
          'debug/gdi_debug_util_win.h',
          # This file depends on files from the 'allocator' target,
          # but this target does not depend on 'allocator' (see
          # allocator.gyp for details).
          'debug/leak_annotations.h',
          'debug/leak_tracker.h',
          'debug/proc_maps_linux.cc',
          'debug/proc_maps_linux.h',
          'debug/profiler.cc',
          'debug/profiler.h',
          'debug/stack_trace.cc',
          'debug/stack_trace.h',
          'debug/stack_trace_android.cc',
          'debug/stack_trace_posix.cc',
          'debug/stack_trace_win.cc',
          'debug/task_annotator.cc',
          'debug/task_annotator.h',
          'debug/trace_event.h',
          'debug/trace_event_android.cc',
          'debug/trace_event_argument.cc',
          'debug/trace_event_argument.h',
          'debug/trace_event_impl.cc',
          'debug/trace_event_impl.h',
          'debug/trace_event_impl_constants.cc',
          'debug/trace_event_synthetic_delay.cc',
          'debug/trace_event_synthetic_delay.h',
          'debug/trace_event_system_stats_monitor.cc',
          'debug/trace_event_memory.cc',
          'debug/trace_event_memory.h',
          'debug/trace_event_win.cc',
          'deferred_sequenced_task_runner.cc',
          'deferred_sequenced_task_runner.h',
          'environment.cc',
          'environment.h',
          'file_descriptor_posix.h',
          'file_version_info.h',
          'file_version_info_mac.h',
          'file_version_info_mac.mm',
          'file_version_info_win.cc',
          'file_version_info_win.h',
          'files/dir_reader_fallback.h',
          'files/dir_reader_linux.h',
          'files/dir_reader_posix.h',
          'files/file.cc',
          'files/file.h',
          'files/file_enumerator.cc',
          'files/file_enumerator.h',
          'files/file_enumerator_posix.cc',
          'files/file_enumerator_win.cc',
          'files/file_path.cc',
          'files/file_path.h',
          'files/file_path_constants.cc',
          'files/file_path_watcher.cc',
          'files/file_path_watcher.h',
          'files/file_path_watcher_fsevents.cc',
          'files/file_path_watcher_fsevents.h',
          'files/file_path_watcher_kqueue.cc',
          'files/file_path_watcher_kqueue.h',
          'files/file_path_watcher_linux.cc',
          'files/file_path_watcher_mac.cc',
          'files/file_path_watcher_stub.cc',
          'files/file_path_watcher_win.cc',
          'files/file_posix.cc',
          'files/file_proxy.cc',
          'files/file_proxy.h',
          'files/file_util.cc',
          'files/file_util.h',
          'files/file_util_android.cc',
          'files/file_util_linux.cc',
          'files/file_util_mac.mm',
          'files/file_util_posix.cc',
          'files/file_util_proxy.cc',
          'files/file_util_proxy.h',
          'files/file_util_win.cc',
          'files/file_win.cc',
          'files/important_file_writer.h',
          'files/important_file_writer.cc',
          'files/memory_mapped_file.cc',
          'files/memory_mapped_file.h',
          'files/memory_mapped_file_posix.cc',
          'files/memory_mapped_file_win.cc',
          'files/scoped_file.cc',
          'files/scoped_file.h',
          'files/scoped_temp_dir.cc',
          'files/scoped_temp_dir.h',
          'float_util.h',
          'format_macros.h',
          'gtest_prod_util.h',
          'guid.cc',
          'guid.h',
          'guid_posix.cc',
          'guid_win.cc',
          'hash.cc',
          'hash.h',
          'id_map.h',
          'ios/device_util.h',
          'ios/device_util.mm',
          'ios/ios_util.h',
          'ios/ios_util.mm',
          'ios/scoped_critical_action.h',
          'ios/scoped_critical_action.mm',
          'json/json_file_value_serializer.cc',
          'json/json_file_value_serializer.h',
          'json/json_parser.cc',
          'json/json_parser.h',
          'json/json_reader.cc',
          'json/json_reader.h',
          'json/json_string_value_serializer.cc',
          'json/json_string_value_serializer.h',
          'json/json_value_converter.h',
          'json/json_writer.cc',
          'json/json_writer.h',
          'json/string_escape.cc',
          'json/string_escape.h',
          'lazy_instance.cc',
          'lazy_instance.h',
          'location.cc',
          'location.h',
          'logging.cc',
          'logging.h',
          'logging_win.cc',
          'logging_win.h',
          'mac/authorization_util.h',
          'mac/authorization_util.mm',
          'mac/bind_objc_block.h',
          'mac/bundle_locations.h',
          'mac/bundle_locations.mm',
          'mac/close_nocancel.cc',
          'mac/cocoa_protocols.h',
          'mac/foundation_util.h',
          'mac/foundation_util.mm',
          'mac/launch_services_util.cc',
          'mac/launch_services_util.h',
          'mac/launchd.cc',
          'mac/launchd.h',
          'mac/libdispatch_task_runner.cc',
          'mac/libdispatch_task_runner.h',
          'mac/mac_logging.h',
          'mac/mac_logging.cc',
          'mac/mac_util.h',
          'mac/mac_util.mm',
          'mac/mach_logging.cc',
          'mac/mach_logging.h',
          'mac/objc_property_releaser.h',
          'mac/objc_property_releaser.mm',
          'mac/os_crash_dumps.cc',
          'mac/os_crash_dumps.h',
          'mac/scoped_aedesc.h',
          'mac/scoped_authorizationref.h',
          'mac/scoped_block.h',
          'mac/scoped_cftyperef.h',
          'mac/scoped_ioobject.h',
          'mac/scoped_ioplugininterface.h',
          'mac/scoped_launch_data.h',
          'mac/scoped_mach_port.cc',
          'mac/scoped_mach_port.h',
          'mac/scoped_mach_vm.cc',
          'mac/scoped_mach_vm.h',
          'mac/scoped_nsautorelease_pool.h',
          'mac/scoped_nsautorelease_pool.mm',
          'mac/scoped_nsexception_enabler.h',
          'mac/scoped_nsexception_enabler.mm',
          'mac/scoped_nsobject.h',
          'mac/scoped_objc_class_swizzler.h',
          'mac/scoped_objc_class_swizzler.mm',
          'mac/scoped_sending_event.h',
          'mac/scoped_sending_event.mm',
          'mac/scoped_typeref.h',
          'mac/sdk_forward_declarations.h',
          'mac/sdk_forward_declarations.mm',
          'macros.h',
          'md5.cc',
          'md5.h',
          'memory/aligned_memory.cc',
          'memory/aligned_memory.h',
          'memory/discardable_memory.cc',
          'memory/discardable_memory.h',
          'memory/discardable_memory_android.cc',
          'memory/discardable_memory_emulated.cc',
          'memory/discardable_memory_emulated.h',
          'memory/discardable_memory_linux.cc',
          'memory/discardable_memory_mac.cc',
          'memory/discardable_memory_malloc.cc',
          'memory/discardable_memory_malloc.h',
          'memory/discardable_memory_manager.cc',
          'memory/discardable_memory_manager.h',
          'memory/discardable_memory_win.cc',
          'memory/linked_ptr.h',
          'memory/manual_constructor.h',
          'memory/memory_pressure_listener.cc',
          'memory/memory_pressure_listener.h',
          'memory/raw_scoped_refptr_mismatch_checker.h',
          'memory/ref_counted.cc',
          'memory/ref_counted.h',
          'memory/ref_counted_delete_on_message_loop.h',
          'memory/ref_counted_memory.cc',
          'memory/ref_counted_memory.h',
          'memory/scoped_open_process.h',
          'memory/scoped_policy.h',
          'memory/scoped_ptr.h',
          'memory/scoped_vector.h',
          'memory/shared_memory.h',
          'memory/shared_memory_android.cc',
          'memory/shared_memory_nacl.cc',
          'memory/shared_memory_posix.cc',
          'memory/shared_memory_win.cc',
          'memory/singleton.cc',
          'memory/singleton.h',
          'memory/weak_ptr.cc',
          'memory/weak_ptr.h',
          'message_loop/incoming_task_queue.cc',
          'message_loop/incoming_task_queue.h',
          'message_loop/message_loop.cc',
          'message_loop/message_loop.h',
          'message_loop/message_loop_proxy.cc',
          'message_loop/message_loop_proxy.h',
          'message_loop/message_loop_proxy_impl.cc',
          'message_loop/message_loop_proxy_impl.h',
          'message_loop/message_pump.cc',
          'message_loop/message_pump.h',
          'message_loop/message_pump_android.cc',
          'message_loop/message_pump_android.h',
          'message_loop/message_pump_default.cc',
          'message_loop/message_pump_default.h',
          'message_loop/message_pump_win.cc',
          'message_loop/message_pump_win.h',
          'message_loop/timer_slack.h',
          'metrics/sample_map.cc',
          'metrics/sample_map.h',
          'metrics/sample_vector.cc',
          'metrics/sample_vector.h',
          'metrics/bucket_ranges.cc',
          'metrics/bucket_ranges.h',
          'metrics/histogram.cc',
          'metrics/histogram.h',
          'metrics/histogram_base.cc',
          'metrics/histogram_base.h',
          'metrics/histogram_delta_serialization.cc',
          'metrics/histogram_delta_serialization.h',
          'metrics/histogram_flattener.h',
          'metrics/histogram_samples.cc',
          'metrics/histogram_samples.h',
          'metrics/histogram_snapshot_manager.cc',
          'metrics/histogram_snapshot_manager.h',
          'metrics/sparse_histogram.cc',
          'metrics/sparse_histogram.h',
          'metrics/statistics_recorder.cc',
          'metrics/statistics_recorder.h',
          'metrics/stats_counters.cc',
          'metrics/stats_counters.h',
          'metrics/stats_table.cc',
          'metrics/stats_table.h',
          'metrics/user_metrics.cc',
          'metrics/user_metrics.h',
          'metrics/user_metrics_action.h',
          'move.h',
          'native_library.h',
          'native_library_mac.mm',
          'native_library_posix.cc',
          'native_library_win.cc',
          'nix/mime_util_xdg.cc',
          'nix/mime_util_xdg.h',
          'nix/xdg_util.cc',
          'nix/xdg_util.h',
          'observer_list.h',
          'observer_list_threadsafe.h',
          'os_compat_android.cc',
          'os_compat_android.h',
          'os_compat_nacl.cc',
          'os_compat_nacl.h',
          'path_service.cc',
          'path_service.h',
          'pending_task.cc',
          'pending_task.h',
          'pickle.cc',
          'pickle.h',
          'port.h',
          'posix/eintr_wrapper.h',
          'posix/global_descriptors.cc',
          'posix/global_descriptors.h',
          'posix/unix_domain_socket_linux.cc',
          'posix/unix_domain_socket_linux.h',
          'power_monitor/power_monitor.cc',
          'power_monitor/power_monitor.h',
          'power_monitor/power_monitor_device_source_android.cc',
          'power_monitor/power_monitor_device_source_android.h',
          'power_monitor/power_monitor_device_source.cc',
          'power_monitor/power_monitor_device_source.h',
          'power_monitor/power_monitor_device_source_ios.mm',
          'power_monitor/power_monitor_device_source_mac.mm',
          'power_monitor/power_monitor_device_source_posix.cc',
          'power_monitor/power_monitor_device_source_win.cc',
          'power_monitor/power_monitor_source.cc',
          'power_monitor/power_monitor_source.h',
          'power_monitor/power_observer.h',
          'process/internal_linux.cc',
          'process/internal_linux.h',
          'process/kill.cc',
          'process/kill.h',
          'process/kill_mac.cc',
          'process/kill_posix.cc',
          'process/kill_win.cc',
          'process/launch.cc',
          'process/launch.h',
          'process/launch_ios.cc',
          'process/launch_mac.cc',
          'process/launch_posix.cc',
          'process/launch_win.cc',
          'process/memory.h',
          'process/memory.cc',
          'process/memory_linux.cc',
          'process/memory_mac.mm',
          'process/memory_win.cc',
          'process/process.h',
          'process/process_handle_freebsd.cc',
          'process/process_handle_linux.cc',
          'process/process_handle_mac.cc',
          'process/process_handle_openbsd.cc',
          'process/process_handle_posix.cc',
          'process/process_handle_win.cc',
          'process/process_info.h',
          'process/process_info_linux.cc',
          'process/process_info_mac.cc',
          'process/process_info_win.cc',
          'process/process_iterator.cc',
          'process/process_iterator.h',
          'process/process_iterator_freebsd.cc',
          'process/process_iterator_linux.cc',
          'process/process_iterator_mac.cc',
          'process/process_iterator_openbsd.cc',
          'process/process_iterator_win.cc',
          'process/process_linux.cc',
          'process/process_metrics.h',
          'process/process_metrics.cc',
          'process/process_metrics_freebsd.cc',
          'process/process_metrics_ios.cc',
          'process/process_metrics_linux.cc',
          'process/process_metrics_mac.cc',
          'process/process_metrics_openbsd.cc',
          'process/process_metrics_posix.cc',
          'process/process_metrics_win.cc',
          'process/process_posix.cc',
          'process/process_win.cc',
          'profiler/scoped_profile.cc',
          'profiler/scoped_profile.h',
          'profiler/alternate_timer.cc',
          'profiler/alternate_timer.h',
          'profiler/tracked_time.cc',
          'profiler/tracked_time.h',
          'rand_util.cc',
          'rand_util.h',
          'rand_util_nacl.cc',
          'rand_util_posix.cc',
          'rand_util_win.cc',
          'run_loop.cc',
          'run_loop.h',
          'numerics/safe_conversions.h',
          'numerics/safe_conversions_impl.h',
          'numerics/safe_math.h',
          'numerics/safe_math_impl.h',
          'safe_strerror_posix.cc',
          'safe_strerror_posix.h',
          'scoped_generic.h',
          'scoped_native_library.cc',
          'scoped_native_library.h',
          'scoped_observer.h',
          'sequence_checker.h',
          'sequence_checker_impl.cc',
          'sequence_checker_impl.h',
          'sequenced_task_runner.cc',
          'sequenced_task_runner.h',
          'sequenced_task_runner_helpers.h',
          'sha1.h',
          'sha1_portable.cc',
          'sha1_win.cc',
          'single_thread_task_runner.h',
          'stl_util.h',
          'strings/latin1_string_conversions.cc',
          'strings/latin1_string_conversions.h',
          'strings/nullable_string16.cc',
          'strings/nullable_string16.h',
          'strings/safe_sprintf.cc',
          'strings/safe_sprintf.h',
          'strings/string16.cc',
          'strings/string16.h',
          'strings/string_number_conversions.cc',
          'strings/string_split.cc',
          'strings/string_split.h',
          'strings/string_number_conversions.h',
          'strings/string_piece.cc',
          'strings/string_piece.h',
          'strings/string_tokenizer.h',
          'strings/string_util.cc',
          'strings/string_util.h',
          'strings/string_util_constants.cc',
          'strings/string_util_posix.h',
          'strings/string_util_win.h',
          'strings/stringize_macros.h',
          'strings/stringprintf.cc',
          'strings/stringprintf.h',
          'strings/sys_string_conversions.h',
          'strings/sys_string_conversions_mac.mm',
          'strings/sys_string_conversions_posix.cc',
          'strings/sys_string_conversions_win.cc',
          'strings/utf_offset_string_conversions.cc',
          'strings/utf_offset_string_conversions.h',
          'strings/utf_string_conversion_utils.cc',
          'strings/utf_string_conversion_utils.h',
          'strings/utf_string_conversions.cc',
          'strings/utf_string_conversions.h',
          'supports_user_data.cc',
          'supports_user_data.h',
          'synchronization/cancellation_flag.cc',
          'synchronization/cancellation_flag.h',
          'synchronization/condition_variable.h',
          'synchronization/condition_variable_posix.cc',
          'synchronization/condition_variable_win.cc',
          'synchronization/lock.cc',
          'synchronization/lock.h',
          'synchronization/lock_impl.h',
          'synchronization/lock_impl_posix.cc',
          'synchronization/lock_impl_win.cc',
          'synchronization/spin_wait.h',
          'synchronization/waitable_event.h',
          'synchronization/waitable_event_posix.cc',
          'synchronization/waitable_event_watcher.h',
          'synchronization/waitable_event_watcher_posix.cc',
          'synchronization/waitable_event_watcher_win.cc',
          'synchronization/waitable_event_win.cc',
          'system_monitor/system_monitor.cc',
          'system_monitor/system_monitor.h',
          'sys_byteorder.h',
          'sys_info.cc',
          'sys_info.h',
          'sys_info_android.cc',
          'sys_info_chromeos.cc',
          'sys_info_freebsd.cc',
          'sys_info_internal.h',
          'sys_info_ios.mm',
          'sys_info_linux.cc',
          'sys_info_mac.cc',
          'sys_info_openbsd.cc',
          'sys_info_posix.cc',
          'sys_info_win.cc',
          'task/cancelable_task_tracker.cc',
          'task/cancelable_task_tracker.h',
          'task_runner.cc',
          'task_runner.h',
          'task_runner_util.h',
          'template_util.h',
          'thread_task_runner_handle.cc',
          'thread_task_runner_handle.h',
          'threading/non_thread_safe.h',
          'threading/non_thread_safe_impl.cc',
          'threading/non_thread_safe_impl.h',
          'threading/platform_thread.h',
          'threading/platform_thread_android.cc',
          'threading/platform_thread_linux.cc',
          'threading/platform_thread_mac.mm',
          'threading/platform_thread_posix.cc',
          'threading/platform_thread_win.cc',
          'threading/post_task_and_reply_impl.cc',
          'threading/post_task_and_reply_impl.h',
          'threading/sequenced_worker_pool.cc',
          'threading/sequenced_worker_pool.h',
          'threading/simple_thread.cc',
          'threading/simple_thread.h',
          'threading/thread.cc',
          'threading/thread.h',
          'threading/thread_checker.h',
          'threading/thread_checker_impl.cc',
          'threading/thread_checker_impl.h',
          'threading/thread_collision_warner.cc',
          'threading/thread_collision_warner.h',
          'threading/thread_id_name_manager.cc',
          'threading/thread_id_name_manager.h',
          'threading/thread_local.h',
          'threading/thread_local_android.cc',
          'threading/thread_local_posix.cc',
          'threading/thread_local_storage.cc',
          'threading/thread_local_storage.h',
          'threading/thread_local_storage_posix.cc',
          'threading/thread_local_storage_win.cc',
          'threading/thread_local_win.cc',
          'threading/thread_restrictions.h',
          'threading/thread_restrictions.cc',
          'threading/watchdog.cc',
          'threading/watchdog.h',
          'threading/worker_pool.h',
          'threading/worker_pool.cc',
          'threading/worker_pool_posix.cc',
          'threading/worker_pool_posix.h',
          'threading/worker_pool_win.cc',
          'time/clock.cc',
          'time/clock.h',
          'time/default_clock.cc',
          'time/default_clock.h',
          'time/default_tick_clock.cc',
          'time/default_tick_clock.h',
          'time/tick_clock.cc',
          'time/tick_clock.h',
          'time/time.cc',
          'time/time.h',
          'time/time_mac.cc',
          'time/time_posix.cc',
          'time/time_win.cc',
          'timer/elapsed_timer.cc',
          'timer/elapsed_timer.h',
          'timer/hi_res_timer_manager.h',
          'timer/hi_res_timer_manager_posix.cc',
          'timer/hi_res_timer_manager_win.cc',
          'timer/mock_timer.cc',
          'timer/mock_timer.h',
          'timer/timer.cc',
          'timer/timer.h',
          'tracked_objects.cc',
          'tracked_objects.h',
          'tracking_info.cc',
          'tracking_info.h',
          'tuple.h',
          'values.cc',
          'values.h',
          'value_conversions.cc',
          'value_conversions.h',
          'version.cc',
          'version.h',
          'vlog.cc',
          'vlog.h',
          'win/enum_variant.cc',
          'win/enum_variant.h',
          'win/event_trace_consumer.h',
          'win/event_trace_controller.cc',
          'win/event_trace_controller.h',
          'win/event_trace_provider.cc',
          'win/event_trace_provider.h',
          'win/i18n.cc',
          'win/i18n.h',
          'win/iat_patch_function.cc',
          'win/iat_patch_function.h',
          'win/iunknown_impl.cc',
          'win/iunknown_impl.h',
          'win/message_window.cc',
          'win/message_window.h',
          'win/metro.cc',
          'win/metro.h',
          'win/object_watcher.cc',
          'win/object_watcher.h',
          'win/registry.cc',
          'win/registry.h',
          'win/resource_util.cc',
          'win/resource_util.h',
          'win/scoped_bstr.cc',
          'win/scoped_bstr.h',
          'win/scoped_co_mem.h',
          'win/scoped_com_initializer.h',
          'win/scoped_comptr.h',
          'win/scoped_gdi_object.h',
          'win/scoped_handle.cc',
          'win/scoped_handle.h',
          'win/scoped_hdc.h',
          'win/scoped_hglobal.h',
          'win/scoped_process_information.cc',
          'win/scoped_process_information.h',
          'win/scoped_propvariant.h',
          'win/scoped_select_object.h',
          'win/scoped_variant.cc',
          'win/scoped_variant.h',
          'win/shortcut.cc',
          'win/shortcut.h',
          'win/startup_information.cc',
          'win/startup_information.h',
          'win/win_util.cc',
          'win/win_util.h',
          'win/windows_version.cc',
          'win/windows_version.h',
          'win/wrapped_window_proc.cc',
          'win/wrapped_window_proc.h',
        ],
        'defines': [
          'BASE_IMPLEMENTATION',
        ],
        'include_dirs': [
          '..',
        ],
        'msvs_disabled_warnings': [
          4018,
        ],
        'target_conditions': [
          ['(<(desktop_linux) == 0 and <(chromeos) == 0) or >(nacl_untrusted_build)==1', {
              'sources/': [
                ['exclude', '^nix/'],
              ],
              'sources!': [
                'atomicops_internals_x86_gcc.cc',
              ],
          }],
          ['<(use_glib)==0 or >(nacl_untrusted_build)==1', {
              'sources!': [
                'message_loop/message_pump_glib.cc',
              ],
          }],
          ['(OS != "linux" and <(os_bsd) != 1 and OS != "android") or >(nacl_untrusted_build)==1', {
              'sources!': [
                # Not automatically excluded by the *linux.cc rules.
                'linux_util.cc',
              ],
            },
          ],
          ['>(nacl_untrusted_build)==1', {
            'sources!': [
               'allocator/type_profiler_control.cc',
               'allocator/type_profiler_control.h',
               'base_paths.cc',
               'cpu.cc',
               'debug/stack_trace_posix.cc',
               'files/file_enumerator_posix.cc',
               'files/file_path_watcher_fsevents.cc',
               'files/file_path_watcher_fsevents.h',
               'files/file_path_watcher_kqueue.cc',
               'files/file_path_watcher_kqueue.h',
               'files/file_proxy.cc',
               'files/file_util.cc',
               'files/file_util_posix.cc',
               'files/file_util_proxy.cc',
               'memory/shared_memory_posix.cc',
               'native_library_posix.cc',
               'path_service.cc',
               'posix/unix_domain_socket_linux.cc',
               'process/kill_posix.cc',
               'process/launch_posix.cc',
               'process/process_metrics_posix.cc',
               'process/process_posix.cc',
               'rand_util_posix.cc',
               'scoped_native_library.cc',
               'files/scoped_temp_dir.cc',
               'sys_info_posix.cc',
               'third_party/dynamic_annotations/dynamic_annotations.c',
            ],
            'sources/': [
              ['include', '^threading/platform_thread_linux\\.cc$'],
            ],
          }],
          ['OS == "android" and _toolset == "target" and >(nacl_untrusted_build)==0', {
           'sources': [
             'memory/discardable_memory_ashmem_allocator.cc',
             'memory/discardable_memory_ashmem_allocator.h',
             'memory/discardable_memory_ashmem.cc',
             'memory/discardable_memory_ashmem.h',
           ],
          }],
          ['OS == "android" and >(nacl_untrusted_build)==0', {
            'sources!': [
              'base_paths_posix.cc',
              'files/file_path_watcher_fsevents.cc',
              'files/file_path_watcher_fsevents.h',
              'files/file_path_watcher_kqueue.cc',
              'files/file_path_watcher_kqueue.h',
              'files/file_path_watcher_stub.cc',
              'power_monitor/power_monitor_device_source_posix.cc',
            ],
            'sources/': [
              ['include', '^debug/proc_maps_linux\\.cc$'],
              ['include', '^files/file_path_watcher_linux\\.cc$'],
              ['include', '^process/memory_linux\\.cc$'],
              ['include', '^process/internal_linux\\.cc$'],
              ['include', '^process/process_handle_linux\\.cc$'],
              ['include', '^process/process_iterator\\.cc$'],
              ['include', '^process/process_iterator_linux\\.cc$'],
              ['include', '^process/process_metrics_linux\\.cc$'],
              ['include', '^posix/unix_domain_socket_linux\\.cc$'],
              ['include', '^strings/sys_string_conversions_posix\\.cc$'],
              ['include', '^sys_info_linux\\.cc$'],
              ['include', '^worker_pool_linux\\.cc$'],
            ],
          }],
          ['OS == "android" and _toolset == "host" and host_os == "linux"', {
            'defines': [
              'OS_ANDROID_HOST=Linux',
            ],
            'sources/': [
              # Pull in specific files for host builds.
              ['include', '^atomicops_internals_x86_gcc\\.cc$'],
              ['include', '^threading/platform_thread_linux\\.cc$'],
            ],
          }],
          ['OS == "android" and <(android_webview_build)==1', {
            'defines': [
               # WebView builds as part of the system which already has sincos;
               # avoid defining it again as it causes a linker warning.
               'ANDROID_SINCOS_PROVIDED',
            ],
          }],
          ['OS == "ios" and _toolset != "host"', {
            'sources/': [
              # Pull in specific Mac files for iOS (which have been filtered out
              # by file name rules).
              ['include', '^atomicops_internals_mac\\.'],
              ['include', '^base_paths_mac\\.'],
              ['include', '^files/file_util_mac\\.'],
              ['include', '^file_version_info_mac\\.'],
              ['include', '^mac/bundle_locations\\.'],
              ['include', '^mac/foundation_util\\.'],
              ['include', '^mac/mac_logging\\.'],
              ['include', '^mac/mach_logging\\.'],
              ['include', '^mac/objc_property_releaser\\.'],
              ['include', '^mac/scoped_mach_port\\.'],
              ['include', '^mac/scoped_mach_vm\\.'],
              ['include', '^mac/scoped_nsautorelease_pool\\.'],
              ['include', '^mac/scoped_nsobject\\.'],
              ['include', '^memory/discardable_memory_mac\\.'],
              ['include', '^message_loop/message_pump_mac\\.'],
              ['include', '^strings/sys_string_conversions_mac\\.'],
              ['include', '^threading/platform_thread_mac\\.'],
              ['include', '^time/time_mac\\.'],
              ['include', '^worker_pool_mac\\.'],
              # Exclude all process/ except the minimal implementation
              # needed on iOS (mostly for unit tests).
              ['exclude', '^process/.*'],
              ['include', '^process/.*_ios\.(cc|mm)$'],
              ['include', '^process/memory_stubs\.cc$'],
              ['include', '^process/process_handle_posix\.cc$'],
              ['exclude', 'files/file_path_watcher_fsevents.cc'],
              ['exclude', 'files/file_path_watcher_fsevents.h'],
              ['include', 'files/file_path_watcher_mac.cc'],
            ],
            'sources': [
              'process/memory_stubs.cc',
            ],
            'sources!': [
              'message_loop/message_pump_libevent.cc'
            ],
          }],
          ['OS == "ios" and _toolset == "host"', {
            'sources/': [
              # Copied filename_rules to switch from iOS to Mac inclusions.
              ['include', '_(cocoa|mac)(_unittest)?\\.(h|cc|mm?)$'],
              ['include', '(^|/)(cocoa|mac)/'],
              ['exclude', '_ios(_unittest)?\\.(h|cc|mm?)$'],
              ['exclude', '(^|/)ios/'],
              ['exclude', 'files/file_path_watcher_fsevents.cc'],
              ['exclude', 'files/file_path_watcher_fsevents.h'],
              ['include', 'files/file_path_watcher_mac.cc'],
            ]
          }],
          # For now, just test the *BSD platforms enough to exclude them.
          # Subsequent changes will include them further.
          ['OS != "freebsd" or >(nacl_untrusted_build)==1', {
              'sources/': [ ['exclude', '_freebsd\\.cc$'] ],
            },
          ],
          ['OS != "openbsd" or >(nacl_untrusted_build)==1', {
              'sources/': [ ['exclude', '_openbsd\\.cc$'] ],
            },
          ],
          ['OS == "win" and >(nacl_untrusted_build)==0', {
            'include_dirs': [
              '<(DEPTH)/third_party/wtl/include',
            ],
            'sources!': [
              'event_recorder_stubs.cc',
              'files/file_path_watcher_fsevents.cc',
              'files/file_path_watcher_fsevents.h',
              'files/file_path_watcher_kqueue.cc',
              'files/file_path_watcher_kqueue.h',
              'files/file_path_watcher_stub.cc',
              'message_loop/message_pump_libevent.cc',
              'posix/file_descriptor_shuffle.cc',
              # Not using sha1_win.cc because it may have caused a
              # regression to page cycler moz.
              'sha1_win.cc',
              'strings/string16.cc',
            ],
          },],
          ['<(use_ozone) == 1', {
            'sources!': [
              'message_loop/message_pump_glib.cc',
            ]
          }],
          ['OS == "linux" and >(nacl_untrusted_build)==0', {
            'sources!': [
              'files/file_path_watcher_fsevents.cc',
              'files/file_path_watcher_fsevents.h',
              'files/file_path_watcher_kqueue.cc',
              'files/file_path_watcher_kqueue.h',
              'files/file_path_watcher_stub.cc',
            ],
          }],
          ['(OS == "mac" or OS == "ios") and >(nacl_untrusted_build)==0', {
            'sources/': [
              ['exclude', '^files/file_path_watcher_stub\\.cc$'],
              ['exclude', '^base_paths_posix\\.cc$'],
              ['exclude', '^native_library_posix\\.cc$'],
              ['exclude', '^strings/sys_string_conversions_posix\\.cc$'],
            ],
          }],
          ['<(os_bsd)==1 and >(nacl_untrusted_build)==0', {
            'sources': [
              'process/memory_stubs.cc',
            ],
            'sources/': [
              ['exclude', '^files/file_path_watcher_linux\\.cc$'],
              ['exclude', '^files/file_path_watcher_stub\\.cc$'],
              ['exclude', '^files/file_util_linux\\.cc$'],
              ['exclude', '^process/process_linux\\.cc$'],
              ['exclude', '^sys_info_linux\\.cc$'],
            ],
          }],
          # Remove all unnecessary files for build_nexe.py to avoid exceeding
          # command-line-string limitation when building NaCl on Windows.
          ['OS == "win" and >(nacl_untrusted_build)==1', {
              'sources/': [ ['exclude', '\\.h$'] ],
          }],
        ],
      }],
      ['base_i18n_target==1', {
        'defines': [
          'BASE_I18N_IMPLEMENTATION',
        ],
        'sources': [
          'i18n/base_i18n_export.h',
          'i18n/bidi_line_iterator.cc',
          'i18n/bidi_line_iterator.h',
          'i18n/break_iterator.cc',
          'i18n/break_iterator.h',
          'i18n/case_conversion.cc',
          'i18n/case_conversion.h',
          'i18n/char_iterator.cc',
          'i18n/char_iterator.h',
          'i18n/file_util_icu.cc',
          'i18n/file_util_icu.h',
          'i18n/i18n_constants.cc',
          'i18n/i18n_constants.h',
          'i18n/icu_encoding_detection.cc',
          'i18n/icu_encoding_detection.h',
          'i18n/icu_string_conversions.cc',
          'i18n/icu_string_conversions.h',
          'i18n/icu_util.cc',
          'i18n/icu_util.h',
          'i18n/number_formatting.cc',
          'i18n/number_formatting.h',
          'i18n/rtl.cc',
          'i18n/rtl.h',
          'i18n/streaming_utf8_validator.cc',
          'i18n/streaming_utf8_validator.h',
          'i18n/string_compare.cc',
          'i18n/string_compare.h',
          'i18n/string_search.cc',
          'i18n/string_search.h',
          'i18n/time_formatting.cc',
          'i18n/time_formatting.h',
          'i18n/timezone.cc',
          'i18n/timezone.h',
          'i18n/utf8_validator_tables.cc',
          'i18n/utf8_validator_tables.h',
        ],
      }]
    ],
  },
}
