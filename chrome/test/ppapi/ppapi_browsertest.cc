// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/path_service.h"
#include "base/test/test_timeouts.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/nacl/nacl_browsertest_util.h"
#include "chrome/test/ppapi/ppapi_test.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/nacl/common/nacl_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/javascript_test_observer.h"
#include "content/public/test/test_renderer_host.h"
#include "extensions/common/constants.h"
#include "extensions/test/extension_test_message_listener.h"
#include "ppapi/shared_impl/test_harness_utils.h"

using content::RenderViewHost;

// This macro finesses macro expansion to do what we want.
#define STRIP_PREFIXES(test_name) ppapi::StripTestPrefixes(#test_name)
// Turn the given token into a string. This allows us to use precompiler stuff
// to turn names into DISABLED_Foo, but still pass a string to RunTest.
#define STRINGIFY(test_name) #test_name
#define LIST_TEST(test_name) STRINGIFY(test_name) ","

// Use these macros to run the tests for a specific interface.
// Most interfaces should be tested with both macros.
#define TEST_PPAPI_OUT_OF_PROCESS(test_name) \
    IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, test_name) { \
      RunTest(STRIP_PREFIXES(test_name)); \
    }

// Similar macros that test over HTTP.
#define TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(test_name) \
    IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, test_name) { \
      RunTestViaHTTP(STRIP_PREFIXES(test_name)); \
    }

// Similar macros that test with an SSL server.
#define TEST_PPAPI_OUT_OF_PROCESS_WITH_SSL_SERVER(test_name) \
    IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, test_name) { \
      RunTestWithSSLServer(STRIP_PREFIXES(test_name)); \
    }

#if defined(DISABLE_NACL)

#define TEST_PPAPI_NACL(test_name)
#define TEST_PPAPI_NACL_DISALLOWED_SOCKETS(test_name)
#define TEST_PPAPI_NACL_WITH_SSL_SERVER(test_name)
#define TEST_PPAPI_NACL_SUBTESTS(test_name, run_statement)

#else

// NaCl based PPAPI tests
#define TEST_PPAPI_NACL(test_name) \
    IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, test_name) { \
      RunTestViaHTTP(STRIP_PREFIXES(test_name)); \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, test_name) { \
      RunTestViaHTTP(STRIP_PREFIXES(test_name)); \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest, \
                           MAYBE_PNACL_NONSFI(test_name)) { \
      RunTestViaHTTP(STRIP_PREFIXES(test_name)); \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest, \
                           MAYBE_PNACL_TRANSITIONAL_NONSFI(test_name)) { \
      RunTestViaHTTP(STRIP_PREFIXES(test_name)); \
    }

// NaCl based PPAPI tests
#define TEST_PPAPI_NACL_SUBTESTS(test_name, run_statement) \
    IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, test_name) { \
      run_statement; \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, test_name) { \
      run_statement; \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest, \
                           MAYBE_PNACL_NONSFI(test_name)) { \
      run_statement; \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest, \
                           MAYBE_PNACL_TRANSITIONAL_NONSFI(test_name)) { \
      run_statement; \
    }

// NaCl based PPAPI tests with disallowed socket API
#define TEST_PPAPI_NACL_DISALLOWED_SOCKETS(test_name) \
    IN_PROC_BROWSER_TEST_F(PPAPINaClTestDisallowedSockets, test_name) { \
      RunTestViaHTTP(STRIP_PREFIXES(test_name)); \
    }

// NaCl based PPAPI tests with SSL server
#define TEST_PPAPI_NACL_WITH_SSL_SERVER(test_name) \
    IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, test_name) { \
      RunTestWithSSLServer(STRIP_PREFIXES(test_name)); \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, test_name) { \
      RunTestWithSSLServer(STRIP_PREFIXES(test_name)); \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest, \
                           MAYBE_PNACL_NONSFI(test_name)) { \
      RunTestWithSSLServer(STRIP_PREFIXES(test_name)); \
    } \
    IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest, \
                           MAYBE_PNACL_TRANSITIONAL_NONSFI(test_name)) { \
      RunTestWithSSLServer(STRIP_PREFIXES(test_name)); \
    }


#endif  // DISABLE_NACL

//
// Interface tests.
//

// Flaky, http://crbug.com/111355
TEST_PPAPI_OUT_OF_PROCESS(DISABLED_Broker)

IN_PROC_BROWSER_TEST_F(PPAPIBrokerInfoBarTest, Accept) {
  // Accepting the infobar should grant permission to access the PPAPI broker.
  InfoBarObserver observer(this);
  observer.ExpectInfoBarAndAccept(true);

  // PPB_Broker_Trusted::IsAllowed should return false before the infobar is
  // popped and true after the infobar is popped.
  RunTest("Broker_IsAllowedPermissionDenied");
  RunTest("Broker_ConnectPermissionGranted");
  RunTest("Broker_IsAllowedPermissionGranted");

  // It should also set a content settings exception for the site.
  GURL url = GetTestFileUrl("Broker_ConnectPermissionGranted");
  HostContentSettingsMap* content_settings =
      browser()->profile()->GetHostContentSettingsMap();
  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            content_settings->GetContentSetting(
                url, url, CONTENT_SETTINGS_TYPE_PPAPI_BROKER, std::string()));
}

IN_PROC_BROWSER_TEST_F(PPAPIBrokerInfoBarTest, Deny) {
  // Canceling the infobar should deny permission to access the PPAPI broker.
  InfoBarObserver observer(this);
  observer.ExpectInfoBarAndAccept(false);

  // PPB_Broker_Trusted::IsAllowed should return false before and after the
  // infobar is popped.
  RunTest("Broker_IsAllowedPermissionDenied");
  RunTest("Broker_ConnectPermissionDenied");
  RunTest("Broker_IsAllowedPermissionDenied");

  // It should also set a content settings exception for the site.
  GURL url = GetTestFileUrl("Broker_ConnectPermissionDenied");
  HostContentSettingsMap* content_settings =
      browser()->profile()->GetHostContentSettingsMap();
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            content_settings->GetContentSetting(
                url, url, CONTENT_SETTINGS_TYPE_PPAPI_BROKER, std::string()));
}

IN_PROC_BROWSER_TEST_F(PPAPIBrokerInfoBarTest, Blocked) {
  // Block access to the PPAPI broker.
  browser()->profile()->GetHostContentSettingsMap()->SetDefaultContentSetting(
      CONTENT_SETTINGS_TYPE_PPAPI_BROKER, CONTENT_SETTING_BLOCK);

  // We shouldn't see an infobar.
  InfoBarObserver observer(this);

  RunTest("Broker_ConnectPermissionDenied");
  RunTest("Broker_IsAllowedPermissionDenied");
}

IN_PROC_BROWSER_TEST_F(PPAPIBrokerInfoBarTest, Allowed) {
  // Always allow access to the PPAPI broker.
  browser()->profile()->GetHostContentSettingsMap()->SetDefaultContentSetting(
      CONTENT_SETTINGS_TYPE_PPAPI_BROKER, CONTENT_SETTING_ALLOW);

  // We shouldn't see an infobar.
  InfoBarObserver observer(this);

  RunTest("Broker_ConnectPermissionGranted");
  RunTest("Broker_IsAllowedPermissionGranted");
}

TEST_PPAPI_NACL(Console)

TEST_PPAPI_NACL(Core)

// Non-NaCl TraceEvent tests are in content/test/ppapi/ppapi_browsertest.cc.
TEST_PPAPI_NACL(TraceEvent)

TEST_PPAPI_NACL(InputEvent)

// Flaky on Linux and Windows. http://crbug.com/135403
#if defined(OS_LINUX) || defined(OS_WIN)
#define MAYBE_ImeInputEvent DISABLED_ImeInputEvent
#else
#define MAYBE_ImeInputEvent ImeInputEvent
#endif

TEST_PPAPI_OUT_OF_PROCESS(MAYBE_ImeInputEvent)
TEST_PPAPI_NACL(MAYBE_ImeInputEvent)

// Graphics2D_Dev isn't supported in NaCl, only test the other interfaces
// TODO(jhorwich) Enable when Graphics2D_Dev interfaces are proxied in NaCl.
TEST_PPAPI_NACL(Graphics2D_InvalidResource)
TEST_PPAPI_NACL(Graphics2D_InvalidSize)
TEST_PPAPI_NACL(Graphics2D_Humongous)
TEST_PPAPI_NACL(Graphics2D_InitToZero)
TEST_PPAPI_NACL(Graphics2D_Describe)
TEST_PPAPI_NACL(Graphics2D_Paint)
TEST_PPAPI_NACL(Graphics2D_Scroll)
TEST_PPAPI_NACL(Graphics2D_Replace)
TEST_PPAPI_NACL(Graphics2D_Flush)
TEST_PPAPI_NACL(Graphics2D_FlushOffscreenUpdate)
TEST_PPAPI_NACL(Graphics2D_BindNull)

#if defined(OS_WIN)
#if defined(USE_AURA)
// These tests fail with the test compositor which is what's used by default for
// browser tests on Windows Aura. Renable when the software compositor is
// available.
#define MAYBE_IN_Graphics3D DISABLED_Graphics3D
#define MAYBE_OUT_Graphics3D DISABLED_Graphics3D
#define MAYBE_NACL_Graphics3D DISABLED_Graphics3D
#else  // defined(USE_AURA)
// In-process and NaCl tests are having flaky failures on Win: crbug.com/242252
#define MAYBE_IN_Graphics3D DISABLED_Graphics3D
#define MAYBE_OUT_Graphics3D Graphics3D
#define MAYBE_NACL_Graphics3D DISABLED_Graphics3D
#endif  // defined(USE_AURA)
#elif defined(OS_MACOSX)
// These tests fail when using the legacy software mode. Reenable when the
// software compositor is enabled crbug.com/286038
#define MAYBE_IN_Graphics3D DISABLED_Graphics3D
#define MAYBE_OUT_Graphics3D DISABLED_Graphics3D
#define MAYBE_NACL_Graphics3D DISABLED_Graphics3D
#else
// The tests are failing in-process. crbug.com/280282
#define MAYBE_IN_Graphics3D DISABLED_Graphics3D
#define MAYBE_OUT_Graphics3D Graphics3D
#define MAYBE_NACL_Graphics3D Graphics3D
#endif
TEST_PPAPI_OUT_OF_PROCESS(MAYBE_OUT_Graphics3D)
TEST_PPAPI_NACL(MAYBE_NACL_Graphics3D)

TEST_PPAPI_NACL(ImageData)

// TCPSocket and TCPSocketPrivate tests.
#define RUN_TCPSOCKET_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(TCPSocket_Connect) \
      LIST_TEST(TCPSocket_ReadWrite) \
      LIST_TEST(TCPSocket_SetOption) \
      LIST_TEST(TCPSocket_Listen) \
      LIST_TEST(TCPSocket_Backlog) \
      LIST_TEST(TCPSocket_Interface_1_0) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, TCPSocket) {
  RUN_TCPSOCKET_SUBTESTS;
}
#if defined(OS_MACOSX) && defined(ADDRESS_SANITIZER)
// Flaky on Mac ASAN: http://crbug.com/437408 and http://crbug.com/457501.
#define MAYBE_TCPSocket DISABLED_TCPSocket
#else
#define MAYBE_TCPSocket TCPSocket
#endif
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, MAYBE_TCPSocket) {
  RUN_TCPSOCKET_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, TCPSocket) {
  RUN_TCPSOCKET_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(TCPSocket)) {
  RUN_TCPSOCKET_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(TCPSocket)) {
  RUN_TCPSOCKET_SUBTESTS;
}


TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(TCPServerSocketPrivate)
TEST_PPAPI_NACL(TCPServerSocketPrivate)

TEST_PPAPI_OUT_OF_PROCESS_WITH_SSL_SERVER(TCPSocketPrivate)

#if defined(OS_MACOSX) && defined(ADDRESS_SANITIZER)
// Flaky on Mac ASAN: http://crbug.com/437408.
#define MAYBE_TCPSocketPrivate DISABLED_TCPSocketPrivate
#else
#define MAYBE_TCPSocketPrivate TCPSocketPrivate
#endif
TEST_PPAPI_NACL_WITH_SSL_SERVER(MAYBE_TCPSocketPrivate)

TEST_PPAPI_OUT_OF_PROCESS_WITH_SSL_SERVER(TCPSocketPrivateTrusted)

// UDPSocket tests.
// UDPSocket_Broadcast is disabled for OSX because it requires root
// permissions on OSX 10.7+.
#if defined(OS_MACOSX)
#define MAYBE_UDPSocket_Broadcast DISABLED_UDPSocket_Broadcast
#else
#define MAYBE_UDPSocket_Broadcast UDPSocket_Broadcast
#endif

#define UDPSOCKET_TEST(_test) \
  IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, _test) { \
    RunTestViaHTTP(LIST_TEST(_test)); \
  } \
  IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, _test) { \
    RunTestViaHTTP(LIST_TEST(_test)); \
  } \
  IN_PROC_BROWSER_TEST_F(PPAPINaClGLibcTest, MAYBE_GLIBC(_test)) { \
    RunTestViaHTTP(LIST_TEST(_test)); \
  } \
  IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, _test) { \
    RunTestViaHTTP(LIST_TEST(_test)); \
  } \
  IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest, \
                         MAYBE_PNACL_NONSFI(_test)) { \
    RunTestViaHTTP(LIST_TEST(_test)); \
  } \
  IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest, \
                         MAYBE_PNACL_TRANSITIONAL_NONSFI(_test)) { \
    RunTestViaHTTP(LIST_TEST(_test)); \
  }

// Instead of one single test for all UDPSocket features (like it is done for
// TCPSocket), split them into multiple, making it easier to isolate which tests
// are failing.
UDPSOCKET_TEST(UDPSocket_ReadWrite)
UDPSOCKET_TEST(UDPSocket_SetOption)
UDPSOCKET_TEST(UDPSocket_SetOption_1_0)
UDPSOCKET_TEST(UDPSocket_SetOption_1_1)
UDPSOCKET_TEST(MAYBE_UDPSocket_Broadcast)
UDPSOCKET_TEST(UDPSocket_ParallelSend)
UDPSOCKET_TEST(UDPSocket_Multicast)

// UDPSocketPrivate tests.
// UDPSocketPrivate_Broadcast is disabled for OSX because it requires root
// permissions on OSX 10.7+.
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(UDPSocketPrivate_Connect)
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(UDPSocketPrivate_ConnectFailure)
#if !defined(OS_MACOSX)
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(UDPSocketPrivate_Broadcast)
#endif  // !defined(OS_MACOSX)
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(UDPSocketPrivate_SetSocketFeatureErrors)
TEST_PPAPI_NACL(UDPSocketPrivate_Connect)
TEST_PPAPI_NACL(UDPSocketPrivate_ConnectFailure)
#if !defined(OS_MACOSX)
TEST_PPAPI_NACL(UDPSocketPrivate_Broadcast)
#endif  // !defined(OS_MACOSX)
TEST_PPAPI_NACL(UDPSocketPrivate_SetSocketFeatureErrors)

// Disallowed socket tests.
TEST_PPAPI_NACL_DISALLOWED_SOCKETS(HostResolverPrivateDisallowed)
TEST_PPAPI_NACL_DISALLOWED_SOCKETS(TCPServerSocketPrivateDisallowed)
TEST_PPAPI_NACL_DISALLOWED_SOCKETS(TCPSocketPrivateDisallowed)
TEST_PPAPI_NACL_DISALLOWED_SOCKETS(UDPSocketPrivateDisallowed)

// HostResolver and HostResolverPrivate tests.
#define RUN_HOST_RESOLVER_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(HostResolver_Empty) \
      LIST_TEST(HostResolver_Resolve) \
      LIST_TEST(HostResolver_ResolveIPv4) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, HostResolver) {
  RUN_HOST_RESOLVER_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, HostResolver) {
  RUN_HOST_RESOLVER_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, HostResolver) {
  RUN_HOST_RESOLVER_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(HostResolver)) {
  RUN_HOST_RESOLVER_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(HostResolver)) {
  RUN_HOST_RESOLVER_SUBTESTS;
}

TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(HostResolverPrivate_Resolve)
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(HostResolverPrivate_ResolveIPv4)
TEST_PPAPI_NACL(HostResolverPrivate_Resolve)
TEST_PPAPI_NACL(HostResolverPrivate_ResolveIPv4)

// URLLoader tests. These are split into multiple test fixtures because if we
// run them all together, they tend to time out.
#define RUN_URLLOADER_SUBTESTS_0 \
  RunTestViaHTTP( \
      LIST_TEST(URLLoader_BasicGET) \
      LIST_TEST(URLLoader_BasicPOST) \
      LIST_TEST(URLLoader_BasicFilePOST) \
      LIST_TEST(URLLoader_BasicFileRangePOST) \
      LIST_TEST(URLLoader_CompoundBodyPOST) \
  )

#define RUN_URLLOADER_SUBTESTS_1 \
  RunTestViaHTTP( \
      LIST_TEST(URLLoader_EmptyDataPOST) \
      LIST_TEST(URLLoader_BinaryDataPOST) \
      LIST_TEST(URLLoader_CustomRequestHeader) \
      LIST_TEST(URLLoader_FailsBogusContentLength) \
      LIST_TEST(URLLoader_StreamToFile) \
  )

// TODO(bbudge) Fix Javascript URLs for trusted loaders.
// http://crbug.com/103062
#define RUN_URLLOADER_SUBTESTS_2 \
  RunTestViaHTTP( \
      LIST_TEST(URLLoader_UntrustedSameOriginRestriction) \
      LIST_TEST(URLLoader_UntrustedCrossOriginRequest) \
      LIST_TEST(URLLoader_UntrustedJavascriptURLRestriction) \
      LIST_TEST(DISABLED_URLLoader_TrustedJavascriptURLRestriction) \
  )

#define RUN_URLLOADER_NACL_SUBTESTS_2 \
  RunTestViaHTTP( \
      LIST_TEST(URLLoader_UntrustedSameOriginRestriction) \
      LIST_TEST(URLLoader_UntrustedCrossOriginRequest) \
      LIST_TEST(URLLoader_UntrustedJavascriptURLRestriction) \
      LIST_TEST(DISABLED_URLLoader_TrustedJavascriptURLRestriction) \
  )

#define RUN_URLLOADER_SUBTESTS_3 \
  RunTestViaHTTP( \
      LIST_TEST(URLLoader_UntrustedHttpRequests) \
      LIST_TEST(URLLoader_FollowURLRedirect) \
      LIST_TEST(URLLoader_AuditURLRedirect) \
      LIST_TEST(URLLoader_AbortCalls) \
      LIST_TEST(URLLoader_UntendedLoad) \
      LIST_TEST(URLLoader_PrefetchBufferThreshold) \
  )

// Note: we do not support Trusted APIs in NaCl, so these will be skipped.
// XRequestedWithHeader isn't trusted per-se, but the header isn't provided
// for NaCl and thus must be skipped.
#define RUN_URLLOADER_TRUSTED_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(URLLoader_TrustedSameOriginRestriction) \
      LIST_TEST(URLLoader_TrustedCrossOriginRequest) \
      LIST_TEST(URLLoader_TrustedHttpRequests) \
      LIST_TEST(URLLoader_XRequestedWithHeader) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, URLLoader0) {
  RUN_URLLOADER_SUBTESTS_0;
}
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, URLLoader1) {
  RUN_URLLOADER_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, URLLoader2) {
  RUN_URLLOADER_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, URLLoader3) {
  RUN_URLLOADER_SUBTESTS_3;
}
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, URLLoaderTrusted) {
  RUN_URLLOADER_TRUSTED_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, URLLoader0) {
  RUN_URLLOADER_SUBTESTS_0;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, URLLoader1) {
  RUN_URLLOADER_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, URLLoader2) {
  RUN_URLLOADER_SUBTESTS_2;
}
#if defined(OS_MACOSX) && defined(ADDRESS_SANITIZER)
// Flaky on Mac ASAN: http://crbug.com/437411.
#define MAYBE_URLLoader3 DISABLED_URLLoader3
#else
#define MAYBE_URLLoader3 URLLoader3
#endif
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, MAYBE_URLLoader3) {
  RUN_URLLOADER_SUBTESTS_3;
}

// Flaky on 32-bit linux bot; http://crbug.com/308906
#if defined(OS_LINUX) && defined(ARCH_CPU_X86)
#define MAYBE_URLLoader_BasicFilePOST DISABLED_URLLoader_BasicFilePOST
#else
#define MAYBE_URLLoader_BasicFilePOST URLLoader_BasicFilePOST
#endif

IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, URLLoader0) {
  RUN_URLLOADER_SUBTESTS_0;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, URLLoader1) {
  RUN_URLLOADER_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, URLLoader2) {
  RUN_URLLOADER_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, URLLoader3) {
  RUN_URLLOADER_SUBTESTS_3;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(URLLoader0)) {
  RUN_URLLOADER_SUBTESTS_0;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(URLLoader1)) {
  RUN_URLLOADER_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(URLLoader2)) {
  RUN_URLLOADER_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(URLLoader3)) {
  RUN_URLLOADER_SUBTESTS_3;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(URLLoader0)) {
  RUN_URLLOADER_SUBTESTS_0;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(URLLoader1)) {
  RUN_URLLOADER_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(URLLoader2)) {
  RUN_URLLOADER_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(URLLoader3)) {
  RUN_URLLOADER_SUBTESTS_3;
}


// URLRequestInfo tests.
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(URLRequest_CreateAndIsURLRequestInfo)

// Timing out on Windows. http://crbug.com/129571
#if defined(OS_WIN)
#define MAYBE_URLRequest_CreateAndIsURLRequestInfo \
  DISABLED_URLRequest_CreateAndIsURLRequestInfo
#else
#define MAYBE_URLRequest_CreateAndIsURLRequestInfo \
    URLRequest_CreateAndIsURLRequestInfo
#endif
TEST_PPAPI_NACL(MAYBE_URLRequest_CreateAndIsURLRequestInfo)

TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(URLRequest_SetProperty)
TEST_PPAPI_NACL(URLRequest_SetProperty)
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(URLRequest_AppendDataToBody)
TEST_PPAPI_NACL(URLRequest_AppendDataToBody)
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(DISABLED_URLRequest_AppendFileToBody)
TEST_PPAPI_NACL(DISABLED_URLRequest_AppendFileToBody)
TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(URLRequest_Stress)
TEST_PPAPI_NACL(URLRequest_Stress)

TEST_PPAPI_OUT_OF_PROCESS(PaintAggregator)
TEST_PPAPI_NACL(PaintAggregator)

// TODO(danakj): http://crbug.com/115286
TEST_PPAPI_NACL(DISABLED_Scrollbar)

TEST_PPAPI_NACL(Var)

TEST_PPAPI_NACL(VarResource)

// PostMessage tests.
#define RUN_POSTMESSAGE_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(PostMessage_SendInInit) \
      LIST_TEST(PostMessage_SendingData) \
      LIST_TEST(PostMessage_SendingString) \
      LIST_TEST(PostMessage_SendingArrayBuffer) \
      LIST_TEST(PostMessage_SendingArray) \
      LIST_TEST(PostMessage_SendingDictionary) \
      LIST_TEST(PostMessage_SendingResource) \
      LIST_TEST(PostMessage_SendingComplexVar) \
      LIST_TEST(PostMessage_MessageEvent) \
      LIST_TEST(PostMessage_NoHandler) \
      LIST_TEST(PostMessage_ExtraParam) \
      LIST_TEST(PostMessage_NonMainThread) \
  )

// Windows defines 'PostMessage', so we have to undef it.
#ifdef PostMessage
#undef PostMessage
#endif

#if defined(OS_WIN)
// http://crbug.com/95557
#define MAYBE_PostMessage DISABLED_PostMessage
#else
#define MAYBE_PostMessage PostMessage
#endif
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, MAYBE_PostMessage) {
  RUN_POSTMESSAGE_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, PostMessage) {
  RUN_POSTMESSAGE_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, PostMessage) {
  RUN_POSTMESSAGE_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(PostMessage)) {
  RUN_POSTMESSAGE_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(PostMessage)) {
  RUN_POSTMESSAGE_SUBTESTS;
}

TEST_PPAPI_NACL(Memory)

// FileIO tests.
#define RUN_FILEIO_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(FileIO_Open) \
      LIST_TEST(FileIO_OpenDirectory) \
      LIST_TEST(FileIO_AbortCalls) \
      LIST_TEST(FileIO_ParallelReads) \
      LIST_TEST(FileIO_ParallelWrites) \
      LIST_TEST(FileIO_NotAllowMixedReadWrite) \
      LIST_TEST(FileIO_ReadWriteSetLength) \
      LIST_TEST(FileIO_ReadToArrayWriteSetLength) \
      LIST_TEST(FileIO_TouchQuery) \
  )

#define RUN_FILEIO_PRIVATE_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(FileIO_RequestOSFileHandle) \
      LIST_TEST(FileIO_RequestOSFileHandleWithOpenExclusive) \
      LIST_TEST(FileIO_Mmap) \
  )

IN_PROC_BROWSER_TEST_F(PPAPIPrivateTest, FileIO_Private) {
  RUN_FILEIO_PRIVATE_SUBTESTS;
}

// See: crbug.com/421284
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, DISABLED_FileIO) {
  RUN_FILEIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPIPrivateTest, FileIO_Private) {
  RUN_FILEIO_PRIVATE_SUBTESTS;
}

// http://crbug.com/313401
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, DISABLED_FileIO) {
  RUN_FILEIO_SUBTESTS;
}
// http://crbug.com/313401
IN_PROC_BROWSER_TEST_F(PPAPIPrivateNaClNewlibTest,
                       DISABLED_NaCl_Newlib_FileIO_Private) {
  RUN_FILEIO_PRIVATE_SUBTESTS;
}

// http://crbug.com/313205
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, DISABLED_FileIO) {
  RUN_FILEIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPIPrivateNaClPNaClTest,
                       DISABLED_PNaCl_FileIO_Private) {
  RUN_FILEIO_PRIVATE_SUBTESTS;
}

IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest, MAYBE_PNACL_NONSFI(FileIO)) {
  RUN_FILEIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPIPrivateNaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(FILEIO_Private)) {
  RUN_FILEIO_PRIVATE_SUBTESTS;
}

IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(FileIO)) {
  RUN_FILEIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPIPrivateNaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(FILEIO_Private)) {
  RUN_FILEIO_PRIVATE_SUBTESTS;
}

// FileRef tests.
#define RUN_FILEREF_SUBTESTS_1 \
  RunTestViaHTTP( \
      LIST_TEST(FileRef_Create) \
      LIST_TEST(FileRef_GetFileSystemType) \
      LIST_TEST(FileRef_GetName) \
      LIST_TEST(FileRef_GetPath) \
      LIST_TEST(FileRef_GetParent) \
      LIST_TEST(FileRef_MakeDirectory) \
  )

#define RUN_FILEREF_SUBTESTS_2 \
  RunTestViaHTTP( \
      LIST_TEST(FileRef_QueryAndTouchFile) \
      LIST_TEST(FileRef_DeleteFileAndDirectory) \
      LIST_TEST(FileRef_RenameFileAndDirectory) \
      LIST_TEST(FileRef_Query) \
      LIST_TEST(FileRef_FileNameEscaping) \
  )

// Note, the FileRef tests are split into two, because all of them together
// sometimes take too long on windows: crbug.com/336999
// FileRef_ReadDirectoryEntries is flaky, so left out. See crbug.com/241646.
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, FileRef1) {
  RUN_FILEREF_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, FileRef2) {
  RUN_FILEREF_SUBTESTS_2;
}

#if defined(OS_MACOSX) && defined(ADDRESS_SANITIZER)
// Flaky on Mac ASAN: http://crbug.com/437411.
#define MAYBE_FileRef1 DISABLED_FileRef1
#else
#define MAYBE_FileRef1 FileRef1
#endif
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, MAYBE_FileRef1) {
  RUN_FILEREF_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, FileRef2) {
  RUN_FILEREF_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, FileRef1) {
  RUN_FILEREF_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, FileRef2) {
  RUN_FILEREF_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(FileRef1)) {
  RUN_FILEREF_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(FileRef2)) {
  RUN_FILEREF_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(FileRef1)) {
  RUN_FILEREF_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(FileRef2)) {
  RUN_FILEREF_SUBTESTS_2;
}

TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(FileSystem)

// PPAPINaClTest.FileSystem times out consistently on Windows and Mac.
// http://crbug.com/130372
#if defined(OS_MACOSX) || defined(OS_WIN)
#define MAYBE_FileSystem DISABLED_FileSystem
#else
#define MAYBE_FileSystem FileSystem
#endif

TEST_PPAPI_NACL(MAYBE_FileSystem)

#if defined(OS_MACOSX)
// http://crbug.com/103912
#define MAYBE_Fullscreen DISABLED_Fullscreen
#elif defined(OS_LINUX)
// http://crbug.com/146008
#define MAYBE_Fullscreen DISABLED_Fullscreen
#elif defined(OS_WIN)
// http://crbug.com/342269
#define MAYBE_Fullscreen DISABLED_Fullscreen
#else
#define MAYBE_Fullscreen Fullscreen
#endif

TEST_PPAPI_OUT_OF_PROCESS_VIA_HTTP(MAYBE_Fullscreen)
TEST_PPAPI_NACL(MAYBE_Fullscreen)

TEST_PPAPI_OUT_OF_PROCESS(X509CertificatePrivate)

TEST_PPAPI_OUT_OF_PROCESS(UMA)
TEST_PPAPI_NACL(UMA)

// NetAddress tests.
#define RUN_NETADDRESS_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(NetAddress_IPv4Address) \
      LIST_TEST(NetAddress_IPv6Address) \
      LIST_TEST(NetAddress_DescribeAsString) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, NetAddress) {
  RUN_NETADDRESS_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, NetAddress) {
  RUN_NETADDRESS_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, NetAddress) {
  RUN_NETADDRESS_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(NetAddress)) {
  RUN_NETADDRESS_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(NetAddress)) {
  RUN_NETADDRESS_SUBTESTS;
}

// NetAddressPrivate tests.
#define RUN_NETADDRESS_PRIVATE_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(NetAddressPrivate_AreEqual) \
      LIST_TEST(NetAddressPrivate_AreHostsEqual) \
      LIST_TEST(NetAddressPrivate_Describe) \
      LIST_TEST(NetAddressPrivate_ReplacePort) \
      LIST_TEST(NetAddressPrivate_GetAnyAddress) \
      LIST_TEST(NetAddressPrivate_DescribeIPv6) \
      LIST_TEST(NetAddressPrivate_GetFamily) \
      LIST_TEST(NetAddressPrivate_GetPort) \
      LIST_TEST(NetAddressPrivate_GetAddress) \
      LIST_TEST(NetAddressPrivate_GetScopeID) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, NetAddressPrivate) {
  RUN_NETADDRESS_PRIVATE_SUBTESTS;
}

#define RUN_NETADDRESS_PRIVATE_UNTRUSTED_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(NetAddressPrivateUntrusted_AreEqual) \
      LIST_TEST(NetAddressPrivateUntrusted_AreHostsEqual) \
      LIST_TEST(NetAddressPrivateUntrusted_Describe) \
      LIST_TEST(NetAddressPrivateUntrusted_ReplacePort) \
      LIST_TEST(NetAddressPrivateUntrusted_GetAnyAddress) \
      LIST_TEST(NetAddressPrivateUntrusted_GetFamily) \
      LIST_TEST(NetAddressPrivateUntrusted_GetPort) \
      LIST_TEST(NetAddressPrivateUntrusted_GetAddress) \
  )

IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, NetAddressPrivate) {
  RUN_NETADDRESS_PRIVATE_UNTRUSTED_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, NetAddressPrivate) {
  RUN_NETADDRESS_PRIVATE_UNTRUSTED_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(NetAddressPrivate)) {
  RUN_NETADDRESS_PRIVATE_UNTRUSTED_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(NetAddressPrivate)) {
  RUN_NETADDRESS_PRIVATE_UNTRUSTED_SUBTESTS;
}

// NetworkMonitor tests.
#define RUN_NETWORK_MONITOR_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(NetworkMonitor_Basic) \
      LIST_TEST(NetworkMonitor_2Monitors) \
      LIST_TEST(NetworkMonitor_DeleteInCallback) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, NetworkMonitor) {
  RUN_NETWORK_MONITOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, NetworkMonitor) {
  RUN_NETWORK_MONITOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, NetworkMonitor) {
  RUN_NETWORK_MONITOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(NetworkMonitor)) {
  RUN_NETWORK_MONITOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(NetworkMonitor)) {
  RUN_NETWORK_MONITOR_SUBTESTS;
}

// Flash tests.
#define RUN_FLASH_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(Flash_SetInstanceAlwaysOnTop) \
      LIST_TEST(Flash_GetCommandLineArgs) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, Flash) {
  RUN_FLASH_SUBTESTS;
}

// In-process WebSocket tests. Note, the WebSocket tests are split into two,
// because all of them together sometimes take too long on windows:
// crbug.com/336999
#define RUN_WEBSOCKET_SUBTESTS_1 \
  RunTestWithWebSocketServer( \
      LIST_TEST(WebSocket_IsWebSocket) \
      LIST_TEST(WebSocket_UninitializedPropertiesAccess) \
      LIST_TEST(WebSocket_InvalidConnect) \
      LIST_TEST(WebSocket_Protocols) \
      LIST_TEST(WebSocket_GetURL) \
      LIST_TEST(WebSocket_ValidConnect) \
      LIST_TEST(WebSocket_InvalidClose) \
      LIST_TEST(WebSocket_ValidClose) \
      LIST_TEST(WebSocket_GetProtocol) \
      LIST_TEST(WebSocket_TextSendReceive) \
      LIST_TEST(WebSocket_BinarySendReceive) \
      LIST_TEST(WebSocket_StressedSendReceive) \
      LIST_TEST(WebSocket_BufferedAmount) \
  )

#define RUN_WEBSOCKET_SUBTESTS_2 \
  RunTestWithWebSocketServer( \
      LIST_TEST(WebSocket_AbortCallsWithCallback) \
      LIST_TEST(WebSocket_AbortSendMessageCall) \
      LIST_TEST(WebSocket_AbortCloseCall) \
      LIST_TEST(WebSocket_AbortReceiveMessageCall) \
      LIST_TEST(WebSocket_ClosedFromServerWhileSending) \
      LIST_TEST(WebSocket_CcInterfaces) \
      LIST_TEST(WebSocket_UtilityInvalidConnect) \
      LIST_TEST(WebSocket_UtilityProtocols) \
      LIST_TEST(WebSocket_UtilityGetURL) \
      LIST_TEST(WebSocket_UtilityValidConnect) \
      LIST_TEST(WebSocket_UtilityInvalidClose) \
      LIST_TEST(WebSocket_UtilityValidClose) \
      LIST_TEST(WebSocket_UtilityGetProtocol) \
      LIST_TEST(WebSocket_UtilityTextSendReceive) \
      LIST_TEST(WebSocket_UtilityBinarySendReceive) \
      LIST_TEST(WebSocket_UtilityBufferedAmount) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, WebSocket1) {
  RUN_WEBSOCKET_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, WebSocket2) {
  RUN_WEBSOCKET_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, WebSocket1) {
  RUN_WEBSOCKET_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, WebSocket2) {
  RUN_WEBSOCKET_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, WebSocket1) {
  RUN_WEBSOCKET_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, WebSocket2) {
  RUN_WEBSOCKET_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(WebSocket1)) {
  RUN_WEBSOCKET_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(WebSocket2)) {
  RUN_WEBSOCKET_SUBTESTS_2;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(WebSocket1)) {
  RUN_WEBSOCKET_SUBTESTS_1;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(WebSocket2)) {
  RUN_WEBSOCKET_SUBTESTS_2;
}

// AudioConfig tests
#define RUN_AUDIO_CONFIG_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(AudioConfig_RecommendSampleRate) \
      LIST_TEST(AudioConfig_ValidConfigs) \
      LIST_TEST(AudioConfig_InvalidConfigs) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, AudioConfig) {
  RUN_AUDIO_CONFIG_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, AudioConfig) {
  RUN_AUDIO_CONFIG_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClGLibcTest, MAYBE_GLIBC(AudioConfig)) {
  RUN_AUDIO_CONFIG_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, AudioConfig) {
  RUN_AUDIO_CONFIG_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(AudioConfig)) {
  RUN_AUDIO_CONFIG_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(AudioConfig)) {
  RUN_AUDIO_CONFIG_SUBTESTS;
}

// PPB_Audio tests.
#define RUN_AUDIO_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(Audio_Creation) \
      LIST_TEST(Audio_DestroyNoStop) \
      LIST_TEST(Audio_Failures) \
      LIST_TEST(Audio_AudioCallback1) \
      LIST_TEST(Audio_AudioCallback2) \
      LIST_TEST(Audio_AudioCallback3) \
      LIST_TEST(Audio_AudioCallback4) \
  )

#if defined(OS_LINUX)
// http://crbug.com/396464
#define MAYBE_Audio DISABLED_Audio
#else
#define MAYBE_Audio Audio
#endif
// PPB_Audio is not supported in-process.
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, MAYBE_Audio) {
  RUN_AUDIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, Audio) {
  RUN_AUDIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClGLibcTest, MAYBE_GLIBC(Audio)) {
  RUN_AUDIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, Audio) {
  RUN_AUDIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(Audio)) {
  RUN_AUDIO_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(Audio)) {
  RUN_AUDIO_SUBTESTS;
}

#define RUN_AUDIO_THREAD_CREATOR_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(Audio_AudioThreadCreatorIsRequired) \
      LIST_TEST(Audio_AudioThreadCreatorIsCalled) \
  )

IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, AudioThreadCreator) {
  RUN_AUDIO_THREAD_CREATOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClGLibcTest, MAYBE_GLIBC(AudioThreadCreator)) {
  RUN_AUDIO_THREAD_CREATOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, AudioThreadCreator) {
  RUN_AUDIO_THREAD_CREATOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest,
                       MAYBE_PNACL_NONSFI(AudioThreadCreator)) {
  RUN_AUDIO_THREAD_CREATOR_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(AudioThreadCreator)) {
  RUN_AUDIO_THREAD_CREATOR_SUBTESTS;
}

TEST_PPAPI_OUT_OF_PROCESS(View_CreatedVisible);
TEST_PPAPI_NACL(View_CreatedVisible);
// This test ensures that plugins created in a background tab have their
// initial visibility set to false. We don't bother testing in-process for this
// custom test since the out of process code also exercises in-process.

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, View_CreateInvisible) {
  // Make a second tab in the foreground.
  GURL url = GetTestFileUrl("View_CreatedInvisible");
  chrome::NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = NEW_BACKGROUND_TAB;
  ui_test_utils::NavigateToURL(&params);
}

// This test messes with tab visibility so is custom.
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, DISABLED_View_PageHideShow) {
  // The plugin will be loaded in the foreground tab and will send us a message.
  PPAPITestMessageHandler handler;
  content::JavascriptTestObserver observer(
      browser()->tab_strip_model()->GetActiveWebContents(),
      &handler);

  GURL url = GetTestFileUrl("View_PageHideShow");
  ui_test_utils::NavigateToURL(browser(), url);

  ASSERT_TRUE(observer.Run()) << handler.error_message();
  EXPECT_STREQ("TestPageHideShow:Created", handler.message().c_str());
  observer.Reset();

  // Make a new tab to cause the original one to hide, this should trigger the
  // next phase of the test.
  chrome::NavigateParams params(
      browser(), GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_LINK);
  params.disposition = NEW_FOREGROUND_TAB;
  ui_test_utils::NavigateToURL(&params);

  // Wait until the test acks that it got hidden.
  ASSERT_TRUE(observer.Run()) << handler.error_message();
  EXPECT_STREQ("TestPageHideShow:Hidden", handler.message().c_str());
  observer.Reset();

  // Switch back to the test tab.
  browser()->tab_strip_model()->ActivateTabAt(0, true);

  ASSERT_TRUE(observer.Run()) << handler.error_message();
  EXPECT_STREQ("PASS", handler.message().c_str());
}

// Tests that if a plugin accepts touch events, the browser knows to send touch
// events to the renderer.
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, InputEvent_AcceptTouchEvent) {
  std::string positive_tests[] = { "InputEvent_AcceptTouchEvent_1",
                                   "InputEvent_AcceptTouchEvent_2",
                                   "InputEvent_AcceptTouchEvent_3",
                                   "InputEvent_AcceptTouchEvent_4"
                                 };

  for (size_t i = 0; i < arraysize(positive_tests); ++i) {
    RenderViewHost* host = browser()->tab_strip_model()->
        GetActiveWebContents()->GetRenderViewHost();
    RunTest(positive_tests[i]);
    EXPECT_TRUE(content::RenderViewHostTester::HasTouchEventHandler(host));
  }
}

// View tests.
#define RUN_VIEW_SUBTESTS \
  RunTestViaHTTP( \
      LIST_TEST(View_SizeChange) \
      LIST_TEST(View_ClipChange) \
      LIST_TEST(View_ScrollOffsetChange) \
  )

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, View) {
  RUN_VIEW_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClNewlibTest, View) {
  RUN_VIEW_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTest, View) {
  RUN_VIEW_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClNonSfiTest, MAYBE_PNACL_NONSFI(View)) {
  RUN_VIEW_SUBTESTS;
}
IN_PROC_BROWSER_TEST_F(PPAPINaClPNaClTransitionalNonSfiTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(View)) {
  RUN_VIEW_SUBTESTS;
}

// FlashMessageLoop tests.
#define RUN_FLASH_MESSAGE_LOOP_SUBTESTS \
  RunTest( \
      LIST_TEST(FlashMessageLoop_Basics) \
      LIST_TEST(FlashMessageLoop_RunWithoutQuit) \
  )

#if defined(OS_LINUX)  // Disabled due to flakiness http://crbug.com/316925
#define MAYBE_FlashMessageLoop DISABLED_FlashMessageLoop
#else
#define MAYBE_FlashMessageLoop FlashMessageLoop
#endif
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, MAYBE_FlashMessageLoop) {
  RUN_FLASH_MESSAGE_LOOP_SUBTESTS;
}

// The compositor test timeouts sometimes, so we have to split it to two
// subtests.
#define RUN_COMPOSITOR_SUBTESTS_0 \
  RunTestViaHTTP( \
      LIST_TEST(Compositor_BindUnbind) \
      LIST_TEST(Compositor_Release) \
      LIST_TEST(Compositor_ReleaseUnbound) \
      LIST_TEST(Compositor_ReleaseWithoutCommit) \
      LIST_TEST(Compositor_ReleaseWithoutCommitUnbound) \
  )

#define RUN_COMPOSITOR_SUBTESTS_1 \
  RunTestViaHTTP( \
      LIST_TEST(Compositor_CommitTwoTimesWithoutChange) \
      LIST_TEST(Compositor_CommitTwoTimesWithoutChangeUnbound) \
      LIST_TEST(Compositor_General) \
      LIST_TEST(Compositor_GeneralUnbound) \
  )

#if defined(OS_WIN)
// This test fails with the test compositor which is what's used by default for
// browser tests on Windows. Renable when the software compositor is available.
#define MAYBE_Compositor0 DISABLED_Compositor0
#define MAYBE_Compositor1 DISABLED_Compositor1
#elif defined(OS_MACOSX)
// This test fails when using the legacy software mode. Reenable when the
// software compositor is enabled crbug.com/286038
#define MAYBE_Compositor0 DISABLED_Compositor0
#define MAYBE_Compositor1 DISABLED_Compositor1
#else
// flaky on Linux: http://crbug.com/396482
#define MAYBE_Compositor0 DISABLED_Compositor0
#define MAYBE_Compositor1 DISABLED_Compositor1
#endif

TEST_PPAPI_NACL_SUBTESTS(MAYBE_Compositor0, RUN_COMPOSITOR_SUBTESTS_0)
TEST_PPAPI_NACL_SUBTESTS(MAYBE_Compositor1, RUN_COMPOSITOR_SUBTESTS_1)

#if defined(OS_WIN)
// Flaky on Windows (crbug.com/438729)
#define MAYBE_MediaStreamAudioTrack DISABLED_MediaStreamAudioTrack
#else
#define MAYBE_MediaStreamAudioTrack MediaStreamAudioTrack
#endif
TEST_PPAPI_NACL(MAYBE_MediaStreamAudioTrack)

TEST_PPAPI_NACL(MediaStreamVideoTrack)

TEST_PPAPI_NACL(MouseCursor)

TEST_PPAPI_NACL(NetworkProxy)

TEST_PPAPI_NACL(TrueTypeFont)

TEST_PPAPI_NACL(VideoDecoder)

TEST_PPAPI_NACL(VideoEncoder)

// VideoDestination doesn't work in content_browsertests.
TEST_PPAPI_OUT_OF_PROCESS(VideoDestination)
TEST_PPAPI_NACL(VideoDestination)

// VideoSource doesn't work in content_browsertests.
TEST_PPAPI_OUT_OF_PROCESS(VideoSource)
TEST_PPAPI_NACL(VideoSource)

// Printing doesn't work in content_browsertests.
TEST_PPAPI_OUT_OF_PROCESS(Printing)

TEST_PPAPI_NACL(MessageHandler)

TEST_PPAPI_NACL(MessageLoop_Basics)
TEST_PPAPI_NACL(MessageLoop_Post)

// Going forward, Flash APIs will only work out-of-process.
TEST_PPAPI_OUT_OF_PROCESS(Flash_GetLocalTimeZoneOffset)
TEST_PPAPI_OUT_OF_PROCESS(Flash_GetProxyForURL)
TEST_PPAPI_OUT_OF_PROCESS(Flash_GetSetting)
TEST_PPAPI_OUT_OF_PROCESS(Flash_SetCrashData)
// http://crbug.com/176822
#if !defined(OS_WIN)
TEST_PPAPI_OUT_OF_PROCESS(FlashClipboard)
#endif
TEST_PPAPI_OUT_OF_PROCESS(FlashFile)
// Mac/Aura reach NOTIMPLEMENTED/time out.
// mac: http://crbug.com/96767
// aura: http://crbug.com/104384
// cros: http://crbug.com/396502
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
#define MAYBE_FlashFullscreen DISABLED_FlashFullscreen
#else
#define MAYBE_FlashFullscreen FlashFullscreen
#endif
TEST_PPAPI_OUT_OF_PROCESS(MAYBE_FlashFullscreen)

TEST_PPAPI_OUT_OF_PROCESS(PDF)

// TODO(dalecurtis): Renable once the platform verification infobar has been
// implemented; see http://crbug.com/270908
// #if defined(OS_CHROMEOS)
// TEST_PPAPI_OUT_OF_PROCESS(PlatformVerificationPrivate)
// #endif

IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, FlashDRM) {
  RunTest(
#if (defined(OS_WIN) && defined(ENABLE_RLZ)) || defined(OS_CHROMEOS)
          // Only implemented on Windows and ChromeOS currently.
          LIST_TEST(FlashDRM_GetDeviceID)
#endif
          LIST_TEST(FlashDRM_GetHmonitor)
          LIST_TEST(FlashDRM_GetVoucherFile));
}

TEST_PPAPI_OUT_OF_PROCESS(TalkPrivate)

#if defined(OS_CHROMEOS)
TEST_PPAPI_OUT_OF_PROCESS(OutputProtectionPrivate)
#endif

#if !defined(DISABLE_NACL)
class PackagedAppTest : public ExtensionBrowserTest {
 public:
  explicit PackagedAppTest(const std::string& toolchain)
      : toolchain_(toolchain) { }

  void LaunchTestingApp(const std::string& extension_dirname) {
    base::FilePath data_dir;
    ASSERT_TRUE(PathService::Get(chrome::DIR_GEN_TEST_DATA, &data_dir));
    base::FilePath app_dir = data_dir.AppendASCII("ppapi")
                                     .AppendASCII("tests")
                                     .AppendASCII("extensions")
                                     .AppendASCII(extension_dirname)
                                     .AppendASCII(toolchain_);

    const extensions::Extension* extension = LoadExtension(app_dir);
    ASSERT_TRUE(extension);

    AppLaunchParams params(browser()->profile(), extension,
                           extensions::LAUNCH_CONTAINER_NONE, NEW_WINDOW,
                           extensions::SOURCE_TEST);
    params.command_line = *base::CommandLine::ForCurrentProcess();
    OpenApplication(params);
  }

  void RunTests(const std::string& extension_dirname) {
    ExtensionTestMessageListener listener("PASS", true);
    LaunchTestingApp(extension_dirname);
    EXPECT_TRUE(listener.WaitUntilSatisfied());
  }
 protected:
  std::string toolchain_;
};

class NewlibPackagedAppTest : public PackagedAppTest {
 public:
  NewlibPackagedAppTest() : PackagedAppTest("newlib") { }
};

class NonSfiPackagedAppTest : public PackagedAppTest {
 public:
  NonSfiPackagedAppTest() : PackagedAppTest("nonsfi") { }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PackagedAppTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableNaClNonSfiMode);
  }
};

// TODO(hidehiko): Switch for NonSfi tests to use nacl_helper_nonsfi, when
// it is launched officially. See NaClBrowserTestPnaclTransitionalNonSfi
// for more details.
class TransitionalNonSfiPackagedAppTest : public NonSfiPackagedAppTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    NonSfiPackagedAppTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kUseNaClHelperNonSfi);
    // TODO(hidehiko): Remove this flag, when namespace sandbox is supported
    // by nacl_helper_nonsfi. (cf. crbug.com/464663)
    command_line->AppendSwitch(switches::kDisableNamespaceSandbox);
  }
};

// Load a packaged app, and wait for it to successfully post a "hello" message
// back.
IN_PROC_BROWSER_TEST_F(NewlibPackagedAppTest, SuccessfulLoad) {
  RunTests("packaged_app");
}

IN_PROC_BROWSER_TEST_F(NonSfiPackagedAppTest,
                       MAYBE_PNACL_NONSFI(SuccessfulLoad)) {
  RunTests("packaged_app");
}

IN_PROC_BROWSER_TEST_F(TransitionalNonSfiPackagedAppTest,
                       MAYBE_PNACL_TRANSITIONAL_NONSFI(SuccessfulLoad)) {
  RunTests("packaged_app");
}

IN_PROC_BROWSER_TEST_F(NewlibPackagedAppTest, SocketPermissions) {
  RunTests("socket_permissions");
}

class MojoPPAPITest : public InProcessBrowserTest {
 public:
  MojoPPAPITest() : InProcessBrowserTest() { }
  virtual ~MojoPPAPITest() { }

  void RunTestInternal() {
    base::FilePath document_root;
    ASSERT_TRUE(ui_test_utils::GetRelativeBuildDirectory(&document_root));
    net::SpawnedTestServer http_server(net::SpawnedTestServer::TYPE_HTTP,
                                       net::SpawnedTestServer::kLocalhost,
                                       document_root);
    ASSERT_TRUE(http_server.Start());

    std::string query = "files/test_case.html?testcase=Mojo&mode=mojo";
    GURL test_url = http_server.GetURL(query);

    PPAPITestMessageHandler handler;
    content::JavascriptTestObserver observer(
        browser()->tab_strip_model()->GetActiveWebContents(),
        &handler);
    ui_test_utils::NavigateToURL(browser(), test_url);

    ASSERT_TRUE(observer.Run()) << handler.error_message();
    result_ = handler.message();
  }

  void RunTest() {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableNaClMojo);
    RunTestInternal();
    EXPECT_STREQ("PASS", result_.c_str());
  }
  void RunTestWithoutFlag() {
    RunTestInternal();
    EXPECT_STREQ("Plugin crashed. 'NaCl module crashed'", result_.c_str());
  }
 private:
  std::string result_;
};

#if defined(OS_POSIX)
#define MAYBE_MOJO(test_name) test_name
#else
#define MAYBE_MOJO(test_name) DISABLED_##test_name
#endif

IN_PROC_BROWSER_TEST_F(MojoPPAPITest, MAYBE_MOJO(Mojo)) {
  RunTest();
}

IN_PROC_BROWSER_TEST_F(MojoPPAPITest, MAYBE_MOJO(MojoFailsWithoutFlag)) {
  RunTestWithoutFlag();
}
#endif
