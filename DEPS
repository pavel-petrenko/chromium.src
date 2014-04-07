#  
#  To use this DEPS file to re-create a Chromium release you
#  need the tools from http://dev.chromium.org installed.
#  
#  This DEPS file corresponds to Chromium 3.0.195.25
#  
#  
#  
deps_os = {
   'win': {
      'src/third_party/cygwin':
         '/trunk/deps/third_party/cygwin@11984',
      'src/third_party/python_24':
         '/trunk/deps/third_party/python_24@19441',
      'src/third_party/ffmpeg/binaries/chromium':
         '/trunk/deps/third_party/ffmpeg/binaries/win@27457',
   },
   'mac': {
      'src/third_party/GTM':
         'http://google-toolbox-for-mac.googlecode.com/svn/trunk@154',
      'src/third_party/pdfsqueeze':
         'http://pdfsqueeze.googlecode.com/svn/trunk@2',
      'src/third_party/WebKit/WebKitLibraries':
         'http://svn.webkit.org/repository/webkit/trunk/WebKitLibraries@46143',
      'src/third_party/WebKit/WebKit/mac':
         'http://svn.webkit.org/repository/webkit/trunk/WebKit/mac@46143',
      'src/third_party/ffmpeg/binaries/chromium':
         '/trunk/deps/third_party/ffmpeg/binaries/mac@27457',
   },
   'unix': {
      'src/third_party/ffmpeg/binaries/chromium':
         '/trunk/deps/third_party/ffmpeg/binaries/linux@27457',
      'src/third_party/xdg-utils':
         '/trunk/deps/third_party/xdg-utils@20073',
   },
}

deps = {
   'src/chrome/test/data/workers/LayoutTests/http/tests/xmlhttprequest':
      '/branches/WebKit/195/LayoutTests/http/tests/xmlhttprequest@27142',
   'src/third_party/WebKit/WebCore':
      '/branches/WebKit/195/WebCore@27142',
   'src/breakpad/src':
      'http://google-breakpad.googlecode.com/svn/trunk/src@346',
   'src/third_party/skia':
      'http://skia.googlecode.com/svn/trunk@330',
   'src/third_party/tcmalloc/tcmalloc':
      'http://google-perftools.googlecode.com/svn/trunk@74',
   'src/googleurl':
      'http://google-url.googlecode.com/svn/trunk@114',
   'src/chrome/test/data/workers/LayoutTests/http/tests/workers':
      '/branches/WebKit/195/LayoutTests/http/tests/workers@27142',
   'src/third_party/protobuf2/src':
      'http://protobuf.googlecode.com/svn/trunk@154',
   'src/native_client':
      'http://nativeclient.googlecode.com/svn/trunk/src/native_client@385',
   'src/chrome/test/data/workers/LayoutTests/http/tests/resources':
      '/branches/WebKit/195/LayoutTests/http/tests/resources@27142',
   'src/tools/gyp':
      'http://gyp.googlecode.com/svn/trunk@553',
   'src/sdch/open-vcdiff':
      'http://open-vcdiff.googlecode.com/svn/trunk@26',
   'src/third_party/WebKit/JavaScriptCore':
      '/branches/WebKit/195/JavaScriptCore@27142',
   'src/chrome/test/data/workers/LayoutTests/fast/workers':
      '/branches/WebKit/195/LayoutTests/fast/workers@27142',
   'src/tools/page_cycler/acid3':
      '/trunk/deps/page_cycler/acid3@19546',
   'src/build/util/support':
      '/trunk/deps/support@28471',
   'src/third_party/icu38':
      '/trunk/deps/third_party/icu38@25921',
   'src/testing/gtest':
      'http://googletest.googlecode.com/svn/trunk@267',
   'src/third_party/WebKit/LayoutTests':
      '/branches/WebKit/195/LayoutTests@27142',
   'src':
      '/branches/195/src@28294',
   'src/third_party/gecko-sdk':
      '/trunk/deps/third_party/gecko-sdk@21193',
   'src/third_party/WebKit':
      '/trunk/deps/third_party/WebKit@20601',
   'src/v8':
      'http://v8.googlecode.com/svn/branches/1.2@2986',
}

skip_child_includes =  ['breakpad', 'gears', 'native_client', 'o3d', 'sdch', 'skia', 'testing', 'third_party', 'v8'] 

hooks =  [{'action': ['python', 'src/tools/gyp/gyp_chromium'], 'pattern': '\\.gypi?$|[/\\\\]src[/\\\\]tools[/\\\\]gyp[/\\\\]'}] 

include_rules =  ['+base', '+build', '+ipc', '+unicode', '+testing', '+webkit/port/platform/graphics/skia/public']