# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'conditions': [
    ['OS=="android"', {
      # TODO(mef): Figure out what needs to be done for gn script.
      'targets': [
        {
          'target_name': 'cronet_jni_headers',
          'type': 'none',
          'sources': [
            'cronet/android/java/src/org/chromium/net/CronetHistogramManager.java',
            'cronet/android/java/src/org/chromium/net/CronetLibraryLoader.java',
            'cronet/android/java/src/org/chromium/net/CronetUploadDataStream.java',
            'cronet/android/java/src/org/chromium/net/CronetUrlRequest.java',
            'cronet/android/java/src/org/chromium/net/CronetUrlRequestContext.java',
            'cronet/android/java/src/org/chromium/net/ChromiumUrlRequest.java',
            'cronet/android/java/src/org/chromium/net/ChromiumUrlRequestContext.java',
          ],
          'variables': {
            'jni_gen_package': 'cronet',
          },
          'includes': [ '../build/jni_generator.gypi' ],
        },
        {
          'target_name': 'cronet_url_request_java',
          'type': 'none',
          'variables': {
            'source_file': 'cronet/android/chromium_url_request.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'net_request_priority_java',
          'type': 'none',
          'variables': {
            'source_file': '../net/base/request_priority.h',
          },
          'includes': [ '../build/android/java_cpp_enum.gypi' ],
        },
        {
          'target_name': 'cronet_url_request_context_config_list',
          'type': 'none',
          'sources': [
            'cronet/android/java/src/org/chromium/net/UrlRequestContextConfigList.template',
          ],
          'variables': {
            'package_name': 'org/chromium/cronet',
            'template_deps': ['cronet/url_request_context_config_list.h'],
          },
          'includes': [ '../build/android/java_cpp_template.gypi' ],
        },
        {
          'target_name': 'cronet_version',
          'type': 'none',
          'variables': {
            'lastchange_path': '<(DEPTH)/build/util/LASTCHANGE',
            'version_py_path': '<(DEPTH)/build/util/version.py',
            'version_path': '<(DEPTH)/chrome/VERSION',
            'template_input_path': 'cronet/android/java/src/org/chromium/net/Version.template',
            'output_path': '<(SHARED_INTERMEDIATE_DIR)/templates/<(_target_name)/org/chromium/cronet/Version.java',
          },
          'direct_dependent_settings': {
            'variables': {
              # Ensure that the output directory is used in the class path
              # when building targets that depend on this one.
              'generated_src_dirs': [
                '<(SHARED_INTERMEDIATE_DIR)/templates/<(_target_name)',
              ],
              # Ensure dependents are rebuilt when the generated Java file changes.
              'additional_input_paths': [
                '<(output_path)',
              ],
            },
          },
          'actions': [
            {
              'action_name': 'cronet_version',
              'inputs': [
                '<(template_input_path)',
                '<(version_path)',
                '<(lastchange_path)',
              ],
              'outputs': [
                '<(output_path)',
              ],
              'action': [
                'python',
                '<(version_py_path)',
                '-f', '<(version_path)',
                '-f', '<(lastchange_path)',
                '<(template_input_path)',
                '<(output_path)',
              ],
              'message': 'Generating version information',
            },
          ],
        },
        {
          'target_name': 'cronet_static',
          'type': 'static_library',
          'dependencies': [
            '../base/base.gyp:base',
            '../base/base.gyp:base_i18n',
            '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
            '../third_party/icu/icu.gyp:icui18n',
            '../third_party/icu/icu.gyp:icuuc',
            'cronet_jni_headers',
            'cronet_url_request_context_config_list',
            'cronet_url_request_java',
            'cronet_version',
            'metrics',
            '../net/net.gyp:net',
          ],
          'sources': [
            'cronet/android/chromium_url_request.cc',
            'cronet/android/chromium_url_request.h',
            'cronet/android/chromium_url_request_context.cc',
            'cronet/android/chromium_url_request_context.h',
            'cronet/android/chromium_url_request_error_list.h',
            'cronet/android/chromium_url_request_priority_list.h',
            'cronet/android/cronet_histogram_manager.cc',
            'cronet/android/cronet_histogram_manager.h',
            'cronet/android/cronet_library_loader.cc',
            'cronet/android/cronet_library_loader.h',
            'cronet/android/cronet_upload_data_stream_adapter.cc',
            'cronet/android/cronet_upload_data_stream_adapter.h',
            'cronet/android/cronet_upload_data_stream_delegate.cc',
            'cronet/android/cronet_upload_data_stream_delegate.h',
            'cronet/android/cronet_url_request.cc',
            'cronet/android/cronet_url_request.h',
            'cronet/android/cronet_url_request_adapter.cc',
            'cronet/android/cronet_url_request_adapter.h',
            'cronet/android/cronet_url_request_context_adapter.cc',
            'cronet/android/cronet_url_request_context_adapter.h',
            'cronet/android/url_request_adapter.cc',
            'cronet/android/url_request_adapter.h',
            'cronet/android/url_request_context_adapter.cc',
            'cronet/android/url_request_context_adapter.h',
            'cronet/android/wrapped_channel_upload_element_reader.cc',
            'cronet/android/wrapped_channel_upload_element_reader.h',
            'cronet/url_request_context_config.cc',
            'cronet/url_request_context_config.h',
            'cronet/url_request_context_config_list.h',
          ],
          'cflags': [
            '-DLOGGING=1',
            '-fdata-sections',
            '-ffunction-sections',
            '-fno-rtti',
            '-fvisibility=hidden',
            '-fvisibility-inlines-hidden',
            '-Wno-sign-promo',
            '-Wno-missing-field-initializers',
          ],
          'ldflags': [
            '-llog',
            '-landroid',
            '-Wl,--gc-sections',
            '-Wl,--exclude-libs,ALL'
          ],
          'conditions': [
            [ 'use_icu_alternatives_on_android == 1', {
                'dependencies!': [
                  '../base/base.gyp:base_i18n',
                  '../third_party/icu/icu.gyp:icui18n',
                  '../third_party/icu/icu.gyp:icuuc',
                ]
              },
            ],
          ],
        },
        {
          'target_name': 'libcronet',
          'type': 'shared_library',
          'sources': [
            'cronet/android/cronet_jni.cc',
          ],
          'dependencies': [
            'cronet_static',
            '../base/base.gyp:base',
            '../net/net.gyp:net',
            '../url/url.gyp:url_lib',
          ],
        },
        { # cronet_stub.jar defines HttpUrlRequest interface and provides its
          # its implementation using HttpUrlConnection (not the Chromium stack).
          'target_name': 'cronet_stub',
          'type': 'none',
          'dependencies': [
            'cronet_url_request_context_config_list',
            'cronet_version',
          ],
          'variables': {
            'java_in_dir': 'cronet/android/java',
            'javac_includes': [
              '**/ChunkedWritableByteChannel.java',
              '**/ExtendedResponseInfo.java',
              '**/HistogramManager.java',
              '**/HttpUrlConnection*.java',
              '**/HttpUrlRequest*.java',
              '**/ResponseInfo.java',
              '**/ResponseTooLargeException.java',
              '**/UploadDataProvider.java',
              '**/UploadDataSink.java',
              '**/UrlRequest.java',
              '**/UrlRequestContext.java',
              '**/UrlRequestContextConfig.java',
              '**/UrlRequestContextConfigList.java',
              '**/UrlRequestException.java',
              '**/UrlRequestListener.java',
              '**/UserAgent.java',
              '**/Version.java',
            ],
          },
          'includes': [ '../build/java.gypi' ],
        },
        { # cronet.jar implements HttpUrlRequest interface using Chromium stack
          # in native libcronet.so library.
          'target_name': 'cronet',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base',
            'cronet_stub',
            'cronet_url_request_java',
            'libcronet',
            'net_request_priority_java',
          ],
          'variables': {
            'java_in_dir': 'cronet/android/java',
            'javac_includes': [
              '**/ChromiumAsyncUrlRequest.java',
              '**/ChromiumUrlRequest.java',
              '**/ChromiumUrlRequestContext.java',
              '**/ChromiumUrlRequestError.java',
              '**/ChromiumUrlRequestFactory.java',
              '**/ChromiumUrlRequestPriority.java',
              '**/CronetHistogramManager.java',
              '**/CronetResponseInfo.java',
              '**/CronetLibraryLoader.java',
              '**/CronetUploadDataStream.java',
              '**/CronetUrlRequest.java',
              '**/CronetUrlRequestContext.java',
              '**/CronetUrlRequestFactory.java',
              '**/RequestPriority.java',
              '**/urlconnection/CronetInputStream.java',
              '**/urlconnection/CronetHttpURLConnection.java',
              '**/urlconnection/CronetHttpURLStreamHandler.java',
              '**/urlconnection/CronetURLStreamHandlerFactory.java',
              '**/urlconnection/MessageLoop.java',
            ],
          },
          'includes': [ '../build/java.gypi' ],
        },
        {
          'target_name': 'cronet_sample_apk',
          'type': 'none',
          'dependencies': [
            'cronet',
            'cronet_stub',
          ],
          'variables': {
            'apk_name': 'CronetSample',
            'java_in_dir': 'cronet/android/sample',
            'resource_dir': 'cronet/android/sample/res',
            'native_lib_target': 'libcronet',
            'proguard_enabled': 'true',
            'proguard_flags_paths': [
              'cronet/android/proguard.cfg',
              'cronet/android/sample/javatests/proguard.cfg',
            ],
          },
          'includes': [ '../build/java_apk.gypi' ],
        },
        {
          # cronet_sample_apk creates a .jar as a side effect. Any java targets
          # that need that .jar in their classpath should depend on this target,
          # cronet_sample_apk_java. Dependents of cronet_sample_apk receive its
          # jar path in the variable 'apk_output_jar_path'. This target should
          # only be used by targets which instrument cronet_sample_apk.
          'target_name': 'cronet_sample_apk_java',
          'type': 'none',
          'dependencies': [
            'cronet_sample_apk',
          ],
          'includes': [ '../build/apk_fake_jar.gypi' ],
        },
        {
          'target_name': 'cronet_sample_test_apk',
          'type': 'none',
          'dependencies': [
            'cronet',
            'cronet_sample_apk_java',
            'cronet_stub',
            '../base/base.gyp:base_java_test_support',
          ],
          'variables': {
            'apk_name': 'CronetSampleTest',
            'java_in_dir': 'cronet/android/sample/javatests',
            'resource_dir': 'cronet/android/sample/res',
            'is_test_apk': 1,
          },
          'includes': [ '../build/java_apk.gypi' ],
        },
        {
          'target_name': 'cronet_tests_jni_headers',
          'type': 'none',
          'sources': [
            'cronet/android/test/src/org/chromium/net/CronetTestUtil.java',
            'cronet/android/test/src/org/chromium/net/MockUrlRequestJobFactory.java',
            'cronet/android/test/src/org/chromium/net/NativeTestServer.java',
            'cronet/android/test/src/org/chromium/net/NetworkChangeNotifierUtil.java',
            'cronet/android/test/src/org/chromium/net/TestUploadDataStreamHandler.java',
          ],
          'variables': {
            'jni_gen_package': 'cronet_tests',
          },
          'includes': [ '../build/jni_generator.gypi' ],
        },
        {
          'target_name': 'libcronet_tests',
          'type': 'shared_library',
          'sources': [
            'cronet/android/test/cronet_test_jni.cc',
            'cronet/android/test/mock_url_request_job_factory.cc',
            'cronet/android/test/mock_url_request_job_factory.h',
            'cronet/android/test/native_test_server.cc',
            'cronet/android/test/native_test_server.h',
            'cronet/android/test/test_upload_data_stream_handler.cc',
            'cronet/android/test/test_upload_data_stream_handler.h',
            'cronet/android/test/network_change_notifier_util.cc',
            'cronet/android/test/network_change_notifier_util.h',
          ],
          'dependencies': [
            'cronet_static',
            'cronet_tests_jni_headers',
            '../base/base.gyp:base',
            '../net/net.gyp:net',
            '../net/net.gyp:net_test_support',
            '../net/net.gyp:quic_tools',
            '../url/url.gyp:url_lib',
            '../base/base.gyp:base_i18n',
            '../third_party/icu/icu.gyp:icui18n',
            '../third_party/icu/icu.gyp:icuuc',
          ],
          'conditions': [
            ['use_icu_alternatives_on_android==1',
              {
                'sources': [
                  '../net/base/directory_lister.cc',
                  '../net/base/directory_lister.h',
                  '../net/url_request/url_request_file_job.cc',
                  '../net/url_request/url_request_file_job.h',
                ]
              }
            ],
          ],
        },
        {
          'target_name': 'cronet_test_apk',
          'type': 'none',
          'dependencies': [
            'cronet',
          ],
          'variables': {
            'apk_name': 'CronetTest',
            'java_in_dir': 'cronet/android/test',
            'resource_dir': 'cronet/android/test/res',
            'asset_location': 'cronet/android/test/assets',
            'native_lib_target': 'libcronet_tests',
          },
          'includes': [ '../build/java_apk.gypi' ],
        },
        {
          # cronet_test_apk creates a .jar as a side effect. Any java targets
          # that need that .jar in their classpath should depend on this target,
          # cronet_test_apk_java. Dependents of cronet_test_apk receive its
          # jar path in the variable 'apk_output_jar_path'. This target should
          # only be used by targets which instrument cronet_test_apk.
          'target_name': 'cronet_test_apk_java',
          'type': 'none',
          'dependencies': [
            'cronet_test_apk',
          ],
          'includes': [ '../build/apk_fake_jar.gypi' ],
        },
        {
          'target_name': 'cronet_test_instrumentation_apk',
          'type': 'none',
          'dependencies': [
            'cronet_test_apk_java',
            '../base/base.gyp:base_java_test_support',
          ],
          'variables': {
            'apk_name': 'CronetTestInstrumentation',
            'java_in_dir': 'cronet/android/test/javatests',
            'resource_dir': 'cronet/android/test/res',
            'is_test_apk': 1,
          },
          'includes': [ '../build/java_apk.gypi' ],
        },
      ],
    }],  # OS=="android"
    ['OS=="android" and use_icu_alternatives_on_android==1', {
      'targets': [
        {
          'target_name': 'cronet_package',
          'type': 'none',
          'dependencies': [
            'libcronet',
            'cronet',
            'cronet_stub',
          ],
          'variables': {
            'native_lib': 'libcronet.>(android_product_extension)',
            'java_lib': 'cronet.jar',
            'java_stub_lib': 'cronet_stub.jar',
            'java_src_lib': 'cronet-src.jar',
            'java_sample_src_lib': 'cronet-sample-src.jar',
            'lib_java_dir': '<(PRODUCT_DIR)/lib.java',
            'package_dir': '<(PRODUCT_DIR)/cronet',
            'intermediate_dir': '<(SHARED_INTERMEDIATE_DIR)/cronet',
            'jar_extract_dir': '<(intermediate_dir)/cronet_jar_extract',
            'jar_excluded_classes': [
              '*/library_loader/*',
            ],
            'jar_extract_stamp': '<(intermediate_dir)/jar_extract.stamp',
            'cronet_jar_stamp': '<(intermediate_dir)/cronet_jar.stamp',
          },
          'actions': [
            {
              'action_name': 'strip libcronet',
              'inputs': ['<(SHARED_LIB_DIR)/<(native_lib)'],
              'outputs': ['<(package_dir)/libs/<(android_app_abi)/<(native_lib)'],
              'action': [
                '<(android_strip)',
                '--strip-unneeded',
                '<@(_inputs)',
                '-o',
                '<@(_outputs)',
              ],
            },
            {
              'action_name': 'extracting from jars',
              'inputs':  [
                '<(lib_java_dir)/<(java_lib)',
                '<(lib_java_dir)/base_java.jar',
                '<(lib_java_dir)/net_java.jar',
                '<(lib_java_dir)/url_java.jar',
              ],
              'outputs': ['<(jar_extract_stamp)', '<(jar_extract_dir)'],
              'action': [
                'python',
                'cronet/tools/extract_from_jars.py',
                '--classes-dir=<(jar_extract_dir)',
                '--jars=<@(_inputs)',
                '--stamp=<(jar_extract_stamp)',
              ],
            },
            {
              'action_name': 'jar_<(_target_name)',
              'message': 'Creating <(_target_name) jar',
              'inputs': [
                '<(DEPTH)/build/android/gyp/util/build_utils.py',
                '<(DEPTH)/build/android/gyp/util/md5_check.py',
                '<(DEPTH)/build/android/gyp/jar.py',
                '<(jar_extract_stamp)',
              ],
              'outputs': [
                '<(package_dir)/<(java_lib)',
                '<(cronet_jar_stamp)',
              ],
              'action': [
                'python', '<(DEPTH)/build/android/gyp/jar.py',
                '--classes-dir=<(jar_extract_dir)',
                '--jar-path=<(package_dir)/<(java_lib)',
                '--excluded-classes=<@(jar_excluded_classes)',
                '--stamp=<(cronet_jar_stamp)',
              ]
            },
            {
              'action_name': 'jar_src_<(_target_name)',
              'inputs': ['cronet/tools/jar_src.py'] ,
              'outputs': ['<(package_dir)/<(java_src_lib)'],
              'action': [
                'python',
                '<@(_inputs)',
                '--src-dir=cronet/android/java/src',
                '--jar-path=<(package_dir)/<(java_src_lib)',
              ],
            },
            {
              'action_name': 'jar_sample_src_<(_target_name)',
              'inputs': ['cronet/tools/jar_src.py'] ,
              'outputs': ['<(package_dir)/<(java_sample_src_lib)'],
              'action': [
                'python',
                '<@(_inputs)',
                '--src-dir=cronet/android/sample',
                '--jar-path=<(package_dir)/<(java_sample_src_lib)',
              ],
            },
            {
              'action_name': 'generate licenses',
              'inputs': ['cronet/tools/cronet_licenses.py'] ,
              'outputs': ['<(package_dir)/LICENSE'],
              'action': [
                'python',
                '<@(_inputs)',
                'license',
                '<@(_outputs)',
              ],
            },
          ],
          'copies': [
            {
              'destination': '<(package_dir)',
              'files': [
                '../AUTHORS',
                '../chrome/VERSION',
                'cronet/android/proguard.cfg',
                '<(lib_java_dir)/<(java_stub_lib)'
              ],
            },
            {
              'destination': '<(package_dir)/symbols/<(android_app_abi)',
              'files': [
                '<(SHARED_LIB_DIR)/<(native_lib)',
              ],
            },
          ],
        },
      ],
    }],  # OS=="android" and use_icu_alternatives_on_android==1
  ],
}
