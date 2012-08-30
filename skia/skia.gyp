# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'skia',
      'type': '<(component)',
      'variables': {
        'optimize': 'max',

        # These two set the paths so we can include skia/gyp/core.gypi
        'skia_src_path': '../third_party/skia/src',
        'skia_include_path': '../third_party/skia/include',
      },

      'includes': [
        '../third_party/skia/gyp/core.gypi',
        '../third_party/skia/gyp/effects.gypi',
        '../third_party/skia/gyp/gpu.gypi',
      ],

      'sources': [
        '<@(gr_sources)',
        '<@(skgr_sources)',
        # this should likely be moved into src/utils in skia
        '../third_party/skia/src/core/SkFlate.cpp',

        '../third_party/skia/src/images/bmpdecoderhelper.cpp',
        '../third_party/skia/src/images/bmpdecoderhelper.h',
        #'../third_party/skia/src/images/SkFDStream.cpp',
        #'../third_party/skia/src/images/SkFlipPixelRef.cpp',
        '../third_party/skia/src/images/SkImageDecoder.cpp',
        '../third_party/skia/src/images/SkImageDecoder_Factory.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_fpdfemb.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_libbmp.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_libgif.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_libico.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_libjpeg.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_libpng.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_libpvjpeg.cpp',
        #'../third_party/skia/src/images/SkImageDecoder_wbmp.cpp',
        #'../third_party/skia/src/images/SkImageEncoder.cpp',
        #'../third_party/skia/src/images/SkImageEncoder_Factory.cpp',
        #'../third_party/skia/src/images/SkImageRef.cpp',
        #'../third_party/skia/src/images/SkImageRefPool.cpp',
        #'../third_party/skia/src/images/SkImageRefPool.h',
        #'../third_party/skia/src/images/SkImageRef_GlobalPool.cpp',
        #'../third_party/skia/src/images/SkMovie.cpp',
        #'../third_party/skia/src/images/SkMovie_gif.cpp',
        '../third_party/skia/src/images/SkScaledBitmapSampler.cpp',
        '../third_party/skia/src/images/SkScaledBitmapSampler.h',

        '../third_party/skia/src/opts/opts_check_SSE2.cpp',

        '../third_party/skia/src/pdf/SkPDFCatalog.cpp',
        '../third_party/skia/src/pdf/SkPDFCatalog.h',
        '../third_party/skia/src/pdf/SkPDFDevice.cpp',
        '../third_party/skia/src/pdf/SkPDFDocument.cpp',
        '../third_party/skia/src/pdf/SkPDFFont.cpp',
        '../third_party/skia/src/pdf/SkPDFFont.h',
        '../third_party/skia/src/pdf/SkPDFFormXObject.cpp',
        '../third_party/skia/src/pdf/SkPDFFormXObject.h',
        '../third_party/skia/src/pdf/SkPDFGraphicState.cpp',
        '../third_party/skia/src/pdf/SkPDFGraphicState.h',
        '../third_party/skia/src/pdf/SkPDFImage.cpp',
        '../third_party/skia/src/pdf/SkPDFImage.h',
        '../third_party/skia/src/pdf/SkPDFPage.cpp',
        '../third_party/skia/src/pdf/SkPDFPage.h',
        '../third_party/skia/src/pdf/SkPDFShader.cpp',
        '../third_party/skia/src/pdf/SkPDFShader.h',
        '../third_party/skia/src/pdf/SkPDFStream.cpp',
        '../third_party/skia/src/pdf/SkPDFStream.h',
        '../third_party/skia/src/pdf/SkPDFTypes.cpp',
        '../third_party/skia/src/pdf/SkPDFTypes.h',
        '../third_party/skia/src/pdf/SkPDFUtils.cpp',
        '../third_party/skia/src/pdf/SkPDFUtils.h',

        '../third_party/skia/src/ports/FontHostConfiguration_android.cpp',
        '../third_party/skia/src/ports/SkFontDescriptor.cpp',
        '../third_party/skia/src/ports/SkFontDescriptor.h',
        #'../third_party/skia/src/ports/SkFontHost_FONTPATH.cpp',
        '../third_party/skia/src/ports/SkFontHost_FreeType.cpp',
        '../third_party/skia/src/ports/SkFontHost_FreeType_common.cpp',
        '../third_party/skia/src/ports/SkFontHost_FreeType_common.h',
        '../third_party/skia/src/ports/SkFontHost_android.cpp',
        #'../third_party/skia/src/ports/SkFontHost_ascender.cpp',
        '../third_party/skia/src/ports/SkFontHost_tables.cpp',
        #'../third_party/skia/src/ports/SkFontHost_linux.cpp',
        '../third_party/skia/src/ports/SkFontHost_mac.cpp',
        #'../third_party/skia/src/ports/SkFontHost_none.cpp',
        '../third_party/skia/src/ports/SkFontHost_sandbox_none.cpp',
        '../third_party/skia/src/ports/SkFontHost_win.cpp',
        '../third_party/skia/src/ports/SkGlobalInitialization_chromium.cpp',
        #'../third_party/skia/src/ports/SkImageDecoder_CG.cpp',
        #'../third_party/skia/src/ports/SkImageDecoder_empty.cpp',
        #'../third_party/skia/src/ports/SkImageRef_ashmem.cpp',
        #'../third_party/skia/src/ports/SkImageRef_ashmem.h',
        #'../third_party/skia/src/ports/SkOSEvent_android.cpp',
        #'../third_party/skia/src/ports/SkOSEvent_dummy.cpp',
        '../third_party/skia/src/ports/SkOSFile_stdio.cpp',
        #'../third_party/skia/src/ports/SkThread_none.cpp',
        '../third_party/skia/src/ports/SkThread_pthread.cpp',
        '../third_party/skia/src/ports/SkThread_win.cpp',
        '../third_party/skia/src/ports/SkTime_Unix.cpp',
        #'../third_party/skia/src/ports/SkXMLParser_empty.cpp',
        #'../third_party/skia/src/ports/SkXMLParser_expat.cpp',
        #'../third_party/skia/src/ports/SkXMLParser_tinyxml.cpp',
        #'../third_party/skia/src/ports/SkXMLPullParser_expat.cpp',

        '../third_party/skia/src/sfnt/SkOTUtils.cpp',
        '../third_party/skia/src/sfnt/SkOTUtils.h',

        '../third_party/skia/include/utils/mac/SkCGUtils.h',
        '../third_party/skia/include/utils/SkDeferredCanvas.h',
        '../third_party/skia/include/utils/SkMatrix44.h',
        '../third_party/skia/src/utils/mac/SkCreateCGImageRef.cpp',
        '../third_party/skia/src/utils/SkBase64.cpp',
        '../third_party/skia/src/utils/SkBase64.h',
        '../third_party/skia/src/utils/SkBitSet.cpp',
        '../third_party/skia/src/utils/SkBitSet.h',
        '../third_party/skia/src/utils/SkDeferredCanvas.cpp',
        '../third_party/skia/src/utils/SkMatrix44.cpp',
        '../third_party/skia/src/utils/SkNullCanvas.cpp',
        '../third_party/skia/include/utils/SkNWayCanvas.h',
        '../third_party/skia/src/utils/SkNWayCanvas.cpp',

        '../third_party/skia/include/pdf/SkPDFDevice.h',
        '../third_party/skia/include/pdf/SkPDFDocument.h',

        '../third_party/skia/include/ports/SkStream_Win.h',
        '../third_party/skia/include/ports/SkTypeface_win.h',

        '../third_party/skia/include/images/SkFlipPixelRef.h',
        '../third_party/skia/include/images/SkImageDecoder.h',
        '../third_party/skia/include/images/SkImageEncoder.h',
        '../third_party/skia/include/images/SkImageRef.h',
        '../third_party/skia/include/images/SkImageRef_GlobalPool.h',
        '../third_party/skia/include/images/SkMovie.h',
        '../third_party/skia/include/images/SkPageFlipper.h',

        '../third_party/skia/include/utils/SkNullCanvas.h',

        'ext/bitmap_platform_device.h',
        'ext/bitmap_platform_device_android.cc',
        'ext/bitmap_platform_device_android.h',
        'ext/bitmap_platform_device_data.h',
        'ext/bitmap_platform_device_linux.cc',
        'ext/bitmap_platform_device_linux.h',
        'ext/bitmap_platform_device_mac.cc',
        'ext/bitmap_platform_device_mac.h',
        'ext/bitmap_platform_device_win.cc',
        'ext/bitmap_platform_device_win.h',
        'ext/canvas_paint.h',
        'ext/canvas_paint_common.h',
        'ext/canvas_paint_gtk.h',
        'ext/canvas_paint_mac.h',
        'ext/canvas_paint_win.h',
        'ext/convolver.cc',
        'ext/convolver.h',
        'ext/google_logging.cc',
        'ext/image_operations.cc',
        'ext/image_operations.h',
        'ext/SkThread_chrome.cc',
        'ext/platform_canvas.cc',
        'ext/platform_canvas.h',
        'ext/platform_canvas_linux.cc',
        'ext/platform_canvas_mac.cc',
        'ext/platform_canvas_skia.cc',
        'ext/platform_canvas_win.cc',
        'ext/platform_device.cc',
        'ext/platform_device.h',
        'ext/platform_device_linux.cc',
        'ext/platform_device_mac.cc',
        'ext/platform_device_win.cc',
        'ext/SkMemory_new_handler.cpp',
        'ext/skia_sandbox_support_win.h',
        'ext/skia_sandbox_support_win.cc',
        'ext/skia_trace_shim.h',
        'ext/skia_utils_mac.mm',
        'ext/skia_utils_mac.h',
        'ext/skia_utils_win.cc',
        'ext/skia_utils_win.h',
        'ext/vector_canvas.cc',
        'ext/vector_canvas.h',
        'ext/vector_platform_device_emf_win.cc',
        'ext/vector_platform_device_emf_win.h',
        'ext/vector_platform_device_skia.cc',
        'ext/vector_platform_device_skia.h',
      ],
      'include_dirs': [
        '..',
        'config',
        '../third_party/skia/include/config',
        '../third_party/skia/include/core',
        '../third_party/skia/include/effects',
        '../third_party/skia/include/gpu',
        '../third_party/skia/include/gpu/gl',
        '../third_party/skia/include/images',
        '../third_party/skia/include/pdf',
        '../third_party/skia/include/pipe',
        '../third_party/skia/include/ports',
        '../third_party/skia/include/utils',
        '../third_party/skia/src/core',
        '../third_party/skia/src/gpu',
        '../third_party/skia/src/sfnt',
        '../third_party/skia/src/utils',
      ],
      'msvs_disabled_warnings': [4244, 4267, 4341, 4345, 4390, 4554, 4748, 4800],
      'defines': [
        'SK_GAMMA_SRGB',
        #'SK_GAMMA_APPLY_TO_A8',
        'SK_BUILD_NO_IMAGE_ENCODE',
        'GR_GL_CUSTOM_SETUP_HEADER="GrGLConfig_chrome.h"',
        'GR_STATIC_RECT_VB=1',
        'GR_AGGRESSIVE_SHADER_OPTS=1',
        'SK_DISABLE_FAST_AA_STROKE_RECT',
        'SK_DEFERRED_CANVAS_USES_GPIPE=1',
        
        # this flag can be removed entirely once this has baked for a while
        'SK_ALLOW_OVER_32K_BITMAPS',

        # temporary for landing Skia rev 3077 with minimal layout test breakage
        'SK_SIMPLE_TWOCOLOR_VERTICAL_GRADIENTS',

        # skia uses static initializers to initialize the serialization logic
        # of its "pictures" library. This is currently not used in chrome; if
        # it ever gets used the processes that use it need to call
        # SkGraphics::Init().
        'SK_ALLOW_STATIC_GLOBAL_INITIALIZERS=0',

        # Temporarily disable the Skia fix in
        # http://code.google.com/p/skia/source/detail?r=3037 ; enabling that
        # fix will require substantial rebaselining.
        'SK_DRAW_POS_TEXT_IGNORE_SUBPIXEL_LEFT_ALIGN_FIX',

        # Temporarily ignore fix to antialias coverage, until we can rebaseline
        'SK_USE_LEGACY_AA_COVERAGE',
      ],
      'sources!': [
        '../third_party/skia/include/core/SkTypes.h',
      ],
      'conditions': [
        ['order_profiling != 0', {
          'target_conditions' : [
            ['_toolset=="target"', {
              'cflags!': [ '-finstrument-functions' ],
            }],
          ],
        }],
        # For POSIX platforms, prefer the Mutex implementation provided by Skia
        # since it does not generate static initializers.
        [ 'OS == "android" or OS == "linux" or OS == "mac"', {
          'defines+': [
            'SK_USE_POSIX_THREADS',
          ],
          'sources!': [
            'ext/SkThread_chrome.cc',
          ],
        }],
        [ 'OS != "android"', {
          'sources/': [
            ['exclude', '_android\\.(cc|cpp)$'],
          ],
          'defines': [
            'SK_DEFAULT_FONT_CACHE_LIMIT=(20*1024*1024)',
          ],
        }],
        [ 'OS != "mac"', {
          'sources/': [
            ['exclude', '_mac\\.(cc|cpp|mm?)$'],
            ['exclude', '/mac/']
          ],
        }],
        [ 'OS != "win"', {
          'sources/': [ ['exclude', '_win\\.(cc|cpp)$'] ],
        }],
        [ 'armv7 == 1', {
          'defines': [
            '__ARM_ARCH__=7',
          ],
        }],
        [ 'armv7 == 1 and arm_neon == 1', {
          'defines': [
            '__ARM_HAVE_NEON',
          ],
        }],
        [ 'target_arch == "arm"', {
          'sources!': [
            '../third_party/skia/src/opts/opts_check_SSE2.cpp'
          ],
        }],
        [ 'use_glib == 1', {
          'dependencies': [
            '../build/linux/system.gyp:fontconfig',
            '../build/linux/system.gyp:freetype2',
            '../build/linux/system.gyp:pangocairo',
            '../third_party/harfbuzz/harfbuzz.gyp:harfbuzz',
            '../third_party/icu/icu.gyp:icuuc',
          ],
          'cflags': [
            '-Wno-unused',
            '-Wno-unused-function',
          ],
          'sources': [
            'ext/SkFontHost_fontconfig.cpp',
            'ext/SkFontHost_fontconfig_direct.cpp',
          ],
          'defines': [
#            'SK_USE_COLOR_LUMINANCE',
          ],
        }],
        [ 'use_glib == 0 and OS != "android"', {
          'sources/': [ ['exclude', '_linux\\.(cc|cpp)$'] ],
          'sources!': [
            '../third_party/skia/src/ports/SkFontHost_FreeType.cpp',
            '../third_party/skia/src/ports/SkFontHost_FreeType_common.cpp',
          ],
        }],
        [ 'use_aura == 1 and use_canvas_skia == 1', {
          'sources/': [
            ['exclude', 'ext/platform_canvas_mac\\.cc$'],
            ['exclude', 'ext/platform_canvas_linux\\.cc$'],
            ['exclude', 'ext/platform_canvas_win\\.cc$'],
          ],
        }, { # use_aura == 0 and use_canvas_skia == 1
          'sources/': [ ['exclude', 'ext/platform_canvas_skia\\.cc$'] ],
        }],
        [ 'toolkit_uses_gtk == 1', {
          'dependencies': [
            '../build/linux/system.gyp:gdk',
          ],
        }, {  # toolkit_uses_gtk == 0
          'sources/': [ ['exclude', '_gtk\\.(cc|cpp)$'] ],
        }],
        [ 'OS == "android"', {
          'sources/': [
            ['exclude', '_linux\\.(cc|cpp)$'],
          ],
          'conditions': [
            [ '_toolset == "target"', {
              'defines': [
                'HAVE_PTHREADS',
                'OS_ANDROID',
                'SK_BUILD_FOR_ANDROID_NDK',
                # Android devices are typically more memory constrained, so
                # use a smaller glyph cache.
                'SK_DEFAULT_FONT_CACHE_LIMIT=(8*1024*1024)',
                'USE_CHROMIUM_SKIA',
              ],
              'dependencies': [
                '../third_party/expat/expat.gyp:expat',
                '../third_party/freetype/freetype.gyp:ft2',
                '../third_party/harfbuzz/harfbuzz.gyp:harfbuzz',
                'skia_opts'
              ],
              'dependencies!': [
                # Android doesn't use Skia's PDF generation, which is what uses
                # sfntly.
                '../third_party/sfntly/sfntly.gyp:sfntly',
              ],
              # This exports a hard dependency because it needs to run its
              # symlink action in order to expose the skia header files.
              'hard_dependency': 1,
              'include_dirs': [
                '../third_party/expat/files/lib',
              ],
              'sources/': [
                ['include', 'ext/platform_device_linux\\.cc$'],
                ['include', 'ext/platform_canvas_linux\\.cc$'],
                ['exclude', '../third_party/skia/src/pdf/'],
              ],
              'sources!': [
                'ext/vector_platform_device_skia.cc',
              ],
              'export_dependent_settings': [
                '../third_party/harfbuzz/harfbuzz.gyp:harfbuzz',
              ],
            }],
            [ '_toolset == "target" and android_build_type == 0', {
              'defines': [
                'HAVE_ENDIAN_H',
              ],
            }],
            [ '_toolset=="host" and host_os=="linux"', {
              'sources': [
                'ext/platform_device_linux.cc',
                'ext/platform_canvas_linux.cc',
              ],
            }],
          ],
        }],
        [ 'OS == "ios"', {
          'sources/': [
            # iOS does not require most of skia and only needs a single file.
            # Rather than creating a separate top-level target, simply exclude
            # all files except for the one that is needed.
            ['exclude', '.*'],
            ['include', '^ext/google_logging\\.cc$'],
          ],
        }],
        [ 'OS == "mac"', {
          'defines': [
            'SK_BUILD_FOR_MAC',
          ],
          'include_dirs': [
            '../third_party/skia/include/utils/mac',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
            ],
          },
          'sources': [
            '../third_party/skia/src/utils/mac/SkStream_mac.cpp',
          ],
          'sources!': [
            # The mac's fonthost implements the table methods natively,
            # so no need for these generic versions.
            '../third_party/skia/src/ports/SkFontHost_tables.cpp',
          ],
          'conditions': [
             [ 'use_skia == 0', {
               'sources/': [
                 ['exclude', '/pdf/'],
                 ['exclude', 'ext/vector_platform_device_skia\\.(cc|h)'],
               ],
            },
            { # use_skia
              'defines': [
                'SK_USE_MAC_CORE_TEXT',
#                'SK_USE_COLOR_LUMINANCE',
              ],
            }],
          ],
        }],
        [ 'OS == "win"', {
          'sources!': [
            '../third_party/skia/src/core/SkMMapStream.cpp',
            '../third_party/skia/src/ports/SkFontHost_sandbox_none.cpp',
            '../third_party/skia/src/ports/SkThread_pthread.cpp',
            '../third_party/skia/src/ports/SkTime_Unix.cpp',
            'ext/SkThread_chrome.cc',
          ],
          'include_dirs': [
            'config/win',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'config/win',
            ],
          },
        }],
        ['component=="shared_library"', {
          'defines': [
            'GR_DLL=1',
            'GR_IMPLEMENTATION=1',
            'SKIA_DLL',
            'SKIA_IMPLEMENTATION=1',
          ],
          'dependencies': [
            '../base/base.gyp:base',
          ],
          'direct_dependent_settings': {
            'defines': [
              'GR_DLL',
              'SKIA_DLL',
            ],
          },
        }],
        ['OS != "ios"', {

          'dependencies': [
            'skia_opts',
            '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
            '../third_party/sfntly/sfntly.gyp:sfntly',
            '../third_party/zlib/zlib.gyp:zlib',
          ],
        }],
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'config',

          #temporary until we can hide SkFontHost
          '../third_party/skia/src/core',

          '../third_party/skia/include/config',
          '../third_party/skia/include/core',
          '../third_party/skia/include/effects',
          '../third_party/skia/include/pdf',
          '../third_party/skia/include/gpu',
          '../third_party/skia/include/gpu/gl',
          '../third_party/skia/include/pipe',
          '../third_party/skia/include/ports',
          '../third_party/skia/include/utils',
          'ext',
        ],
        'defines': [
          'SK_BUILD_NO_IMAGE_ENCODE',
          'SK_DEFERRED_CANVAS_USES_GPIPE=1',
          'GR_GL_CUSTOM_SETUP_HEADER="GrGLConfig_chrome.h"',
          'GR_AGGRESSIVE_SHADER_OPTS=1',
        ],
        'conditions': [
          ['OS=="android"', {
            'dependencies!': [
              'skia_opts',
              '../third_party/zlib/zlib.gyp:zlib',
            ],
            'defines': [
              # Don't use non-NDK available stuff.
              'SK_BUILD_FOR_ANDROID_NDK',
            ],
            'conditions': [
              [ '_toolset == "target" and android_build_type == 0', {
                'defines': [
                  'HAVE_ENDIAN_H',
                ],
              }],
            ],
          }],
          ['OS=="mac"', {
            'include_dirs': [
              '../third_party/skia/include/utils/mac',
            ],
          }],
        ],
      },
    },

    # Due to an unfortunate intersection of lameness between gcc and gyp,
    # we have to build the *_SSE2.cpp files in a separate target.  The
    # gcc lameness is that, in order to compile SSE2 intrinsics code, it
    # must be passed the -msse2 flag.  However, with this flag, it may
    # emit SSE2 instructions even for scalar code, such as the CPUID
    # test used to test for the presence of SSE2.  So that, and all other
    # code must be compiled *without* -msse2.  The gyp lameness is that it
    # does not allow file-specific CFLAGS, so we must create this extra
    # target for those files to be compiled with -msse2.
    #
    # This is actually only a problem on 32-bit Linux (all Intel Macs have
    # SSE2, Linux x86_64 has SSE2 by definition, and MSC will happily emit
    # SSE2 from instrinsics, which generating plain ol' 386 for everything
    # else).  However, to keep the .gyp file simple and avoid platform-specific
    # build breakage, we do this on all platforms.

    # For about the same reason, we need to compile the ARM opts files
    # separately as well.
    {
      'target_name': 'skia_opts',
      'type': 'static_library',
      'variables': {
        'optimize': 'max',
      },
      'include_dirs': [
        '..',
        'config',
        '../third_party/skia/include/config',
        '../third_party/skia/include/core',
        '../third_party/skia/include/effects',
        '../third_party/skia/include/images',
        '../third_party/skia/include/utils',
        '../third_party/skia/src/core',
      ],
      'conditions': [
        ['order_profiling != 0', {
          'target_conditions' : [
            ['_toolset=="target"', {
              'cflags!': [ '-finstrument-functions' ],
            }],
          ],
        }],
        [ 'os_posix == 1 and OS != "mac" and OS != "android" and target_arch != "arm"', {
          'cflags': [
            '-msse2',
          ],
        }],
        [ 'OS == "android"', {
          'defines': [
            'SK_BUILD_FOR_ANDROID_NDK',
          ],
        }],
        [ 'target_arch != "arm"', {
          'sources': [
            '../third_party/skia/src/opts/SkBitmapProcState_opts_SSE2.cpp',
            '../third_party/skia/src/opts/SkBlitRect_opts_SSE2.cpp',
            '../third_party/skia/src/opts/SkBlitRow_opts_SSE2.cpp',
            '../third_party/skia/src/opts/SkUtils_opts_SSE2.cpp',
          ],
          'conditions': [
            # x86 Android doesn't support SSSE3 instructions.
            [ 'OS != "android"', {
              'dependencies': [
                'skia_opts_ssse3',
              ],
            }],
          ],
        },
        {  # arm
          'conditions': [
            ['order_profiling != 0', {
              'target_conditions' : [
                ['_toolset=="target"', {
                  'cflags!': [ '-finstrument-functions' ],
                }],
              ],
            }],
            [ 'armv7 == 1', {
              'defines': [
                '__ARM_ARCH__=7',
              ],
            }],
            [ 'armv7 == 1 and arm_neon == 1', {
              'defines': [
                '__ARM_HAVE_NEON',
              ],
              'cflags': [
                # The neon assembly contains conditional instructions which
                # aren't enclosed in an IT block. The assembler complains
                # without this option.
                # See #86592.
                '-Wa,-mimplicit-it=always',
              ],
           }],
          ],
          # The assembly uses the frame pointer register (r7 in Thumb/r11 in
          # ARM), the compiler doesn't like that. Explicitly remove the
          # -fno-omit-frame-pointer flag for Android, as that gets added to all
          # targets via common.gypi.
          'cflags!': [
            '-fno-omit-frame-pointer',
          ],
          'cflags': [
            '-fomit-frame-pointer',
          ],
          'sources': [
            '../third_party/skia/src/opts/SkBitmapProcState_opts_arm.cpp',
            '../third_party/skia/src/opts/SkBlitRow_opts_arm.cpp',
            '../third_party/skia/src/opts/SkBlitRow_opts_arm.h',
            '../third_party/skia/src/opts/opts_check_arm.cpp',
          ],
        }],
        [ 'armv7 == 1 and arm_neon == 0', {
          'sources': [
            '../third_party/skia/src/opts/memset.arm.S',
        ],
        }],
        [ 'armv7 == 1 and arm_neon == 1', {
          'sources': [
            '../third_party/skia/src/opts/memset16_neon.S',
            '../third_party/skia/src/opts/memset32_neon.S',
            '../third_party/skia/src/opts/SkBitmapProcState_arm_neon.cpp',
            '../third_party/skia/src/opts/SkBitmapProcState_matrixProcs_neon.cpp',
            '../third_party/skia/src/opts/SkBitmapProcState_matrix_clamp_neon.h',
            '../third_party/skia/src/opts/SkBitmapProcState_matrix_repeat_neon.h',
            '../third_party/skia/src/opts/SkBlitRow_opts_arm_neon.cpp',
        ],
        }],
        [ 'target_arch == "arm" and armv7 != 1', {
          'sources': [
            '../third_party/skia/src/opts/SkBlitRow_opts_none.cpp',
          ],
          'sources!': [
            '../third_party/skia/src/opts/SkBlitRow_opts_arm.cpp',
          ],
        }],
      ],
    },
    # For the same lame reasons as what is done for skia_opts, we have to
    # create another target specifically for SSSE3 code as we would not want
    # to compile the SSE2 code with -mssse3 which would potentially allow
    # gcc to generate SSSE3 code.
    {
      'target_name': 'skia_opts_ssse3',
      'type': 'static_library',
      'variables': {
        'optimize': 'max',
      },
      'include_dirs': [
        '..',
        'config',
        '../third_party/skia/include/config',
        '../third_party/skia/include/core',
        '../third_party/skia/src/core',
      ],
      'conditions': [
        [ 'OS in ["linux", "freebsd", "openbsd", "solaris"]', {
          'cflags': [
            '-mssse3',
          ],
        }],
        ['order_profiling != 0', {
          'target_conditions' : [
            ['_toolset=="target"', {
              'cflags!': [ '-finstrument-functions' ],
            }],
          ],
        }],
        [ 'OS == "mac"', {
          'xcode_settings': {
            'GCC_ENABLE_SUPPLEMENTAL_SSE3_INSTRUCTIONS': 'YES',
          },
        }],
        [ 'OS == "win"', {
          'include_dirs': [
            'config/win',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'config/win',
            ],
          },
        }],
        [ 'target_arch != "arm"', {
          'sources': [
            '../third_party/skia/src/opts/SkBitmapProcState_opts_SSSE3.cpp',
          ],
        }],
      ],
    },
    {
      'target_name': 'image_operations_bench',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:base',
        'skia',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'ext/image_operations_bench.cc',
      ],
    },
  ],
}
