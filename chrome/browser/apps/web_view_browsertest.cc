// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/ui/native_app_window.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/automation/automation_util.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/prerender/prerender_link_manager.h"
#include "chrome/browser/prerender/prerender_link_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/fake_speech_recognition_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/extensions_client.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "ui/gl/gl_switches.h"

// For fine-grained suppression on flaky tests.
#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

using prerender::PrerenderLinkManager;
using prerender::PrerenderLinkManagerFactory;

namespace {
const char kEmptyResponsePath[] = "/close-socket";
const char kRedirectResponsePath[] = "/server-redirect";
const char kRedirectResponseFullPath[] =
    "/extensions/platform_apps/web_view/shim/guest_redirect.html";

// Platform-specific filename relative to the chrome executable.
#if defined(OS_WIN)
const wchar_t library_name[] = L"ppapi_tests.dll";
#elif defined(OS_MACOSX)
const char library_name[] = "ppapi_tests.plugin";
#elif defined(OS_POSIX)
const char library_name[] = "libppapi_tests.so";
#endif

class EmptyHttpResponse : public net::test_server::HttpResponse {
 public:
  virtual std::string ToResponseString() const OVERRIDE {
    return std::string();
  }
};

class TestInterstitialPageDelegate : public content::InterstitialPageDelegate {
 public:
  TestInterstitialPageDelegate() {
  }
  virtual ~TestInterstitialPageDelegate() {}
  virtual std::string GetHTMLContents() OVERRIDE { return std::string(); }
};

// Used to get notified when a guest is created.
class GuestContentBrowserClient : public chrome::ChromeContentBrowserClient {
 public:
  GuestContentBrowserClient() : web_contents_(NULL) {}

  content::WebContents* WaitForGuestCreated() {
    if (web_contents_)
      return web_contents_;

    message_loop_runner_ = new content::MessageLoopRunner;
    message_loop_runner_->Run();
    return web_contents_;
  }

 private:
  // ChromeContentBrowserClient implementation:
  virtual void GuestWebContentsAttached(
      content::WebContents* guest_web_contents,
      content::WebContents* embedder_web_contents,
      const base::DictionaryValue& extra_params) OVERRIDE {
    ChromeContentBrowserClient::GuestWebContentsAttached(
        guest_web_contents, embedder_web_contents, extra_params);
    web_contents_ = guest_web_contents;

    if (message_loop_runner_)
      message_loop_runner_->Quit();
  }

  content::WebContents* web_contents_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;
};

class InterstitialObserver : public content::WebContentsObserver {
 public:
  InterstitialObserver(content::WebContents* web_contents,
                       const base::Closure& attach_callback,
                       const base::Closure& detach_callback)
      : WebContentsObserver(web_contents),
        attach_callback_(attach_callback),
        detach_callback_(detach_callback) {
  }

  virtual void DidAttachInterstitialPage() OVERRIDE {
    attach_callback_.Run();
  }

  virtual void DidDetachInterstitialPage() OVERRIDE {
    detach_callback_.Run();
  }

 private:
  base::Closure attach_callback_;
  base::Closure detach_callback_;

  DISALLOW_COPY_AND_ASSIGN(InterstitialObserver);
};

}  // namespace

// This class intercepts media access request from the embedder. The request
// should be triggered only if the embedder API (from tests) allows the request
// in Javascript.
// We do not issue the actual media request; the fact that the request reached
// embedder's WebContents is good enough for our tests. This is also to make
// the test run successfully on trybots.
class MockWebContentsDelegate : public content::WebContentsDelegate {
 public:
  MockWebContentsDelegate() : requested_(false) {}
  virtual ~MockWebContentsDelegate() {}

  virtual void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) OVERRIDE {
    requested_ = true;
    if (message_loop_runner_.get())
      message_loop_runner_->Quit();
  }

  void WaitForSetMediaPermission() {
    if (requested_)
      return;
    message_loop_runner_ = new content::MessageLoopRunner;
    message_loop_runner_->Run();
  }

 private:
  bool requested_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(MockWebContentsDelegate);
};

// This class intercepts download request from the guest.
class MockDownloadWebContentsDelegate : public content::WebContentsDelegate {
 public:
  explicit MockDownloadWebContentsDelegate(
      content::WebContentsDelegate* orig_delegate)
      : orig_delegate_(orig_delegate),
        waiting_for_decision_(false),
        expect_allow_(false),
        decision_made_(false),
        last_download_allowed_(false) {}
  virtual ~MockDownloadWebContentsDelegate() {}

  virtual void CanDownload(
      content::RenderViewHost* render_view_host,
      int request_id,
      const std::string& request_method,
      const base::Callback<void(bool)>& callback) OVERRIDE {
    orig_delegate_->CanDownload(
        render_view_host, request_id, request_method,
        base::Bind(&MockDownloadWebContentsDelegate::DownloadDecided,
                   base::Unretained(this)));
  }

  void WaitForCanDownload(bool expect_allow) {
    EXPECT_FALSE(waiting_for_decision_);
    waiting_for_decision_ = true;

    if (decision_made_) {
      EXPECT_EQ(expect_allow, last_download_allowed_);
      return;
    }

    expect_allow_ = expect_allow;
    message_loop_runner_ = new content::MessageLoopRunner;
    message_loop_runner_->Run();
  }

  void DownloadDecided(bool allow) {
    EXPECT_FALSE(decision_made_);
    decision_made_ = true;

    if (waiting_for_decision_) {
      EXPECT_EQ(expect_allow_, allow);
      if (message_loop_runner_.get())
        message_loop_runner_->Quit();
      return;
    }
    last_download_allowed_ = allow;
  }

  void Reset() {
    waiting_for_decision_ = false;
    decision_made_ = false;
  }

 private:
  content::WebContentsDelegate* orig_delegate_;
  bool waiting_for_decision_;
  bool expect_allow_;
  bool decision_made_;
  bool last_download_allowed_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(MockDownloadWebContentsDelegate);
};

class WebViewTest : public extensions::PlatformAppBrowserTest {
 protected:
  virtual void SetUp() OVERRIDE {
    if (UsesFakeSpeech()) {
      // SpeechRecognition test specific SetUp.
      fake_speech_recognition_manager_.reset(
          new content::FakeSpeechRecognitionManager());
      fake_speech_recognition_manager_->set_should_send_fake_response(true);
      // Inject the fake manager factory so that the test result is returned to
      // the web page.
      content::SpeechRecognitionManager::SetManagerForTesting(
          fake_speech_recognition_manager_.get());
    }
    extensions::PlatformAppBrowserTest::SetUp();
  }

  virtual void TearDown() OVERRIDE {
    if (UsesFakeSpeech()) {
      // SpeechRecognition test specific TearDown.
      content::SpeechRecognitionManager::SetManagerForTesting(NULL);
    }

    extensions::PlatformAppBrowserTest::TearDown();
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    // Mock out geolocation for geolocation specific tests.
    if (!strncmp(test_info->name(), "GeolocationAPI",
            strlen("GeolocationAPI"))) {
      ui_test_utils::OverrideGeolocation(10, 20);
    }
  }

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();

    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, "--expose-gc");

    // Force SW rendering to check autosize bug.
    if (!strncmp(test_info->name(), "AutoSizeSW", strlen("AutosizeSW")))
      command_line->AppendSwitch(switches::kDisableForceCompositingMode);

    extensions::PlatformAppBrowserTest::SetUpCommandLine(command_line);
  }

  // This method is responsible for initializing a packaged app, which contains
  // multiple webview tags. The tags have different partition identifiers and
  // their WebContent objects are returned as output. The method also verifies
  // the expected process allocation and storage partition assignment.
  // The |navigate_to_url| parameter is used to navigate the main browser
  // window.
  //
  // TODO(ajwong): This function is getting to be too large. Either refactor it
  // so the test can specify a configuration of WebView tags that we will
  // dynamically inject JS to generate, or move this test wholesale into
  // something that RunPlatformAppTest() can execute purely in Javascript. This
  // won't let us do a white-box examination of the StoragePartition equivalence
  // directly, but we will be able to view the black box effects which is good
  // enough.  http://crbug.com/160361
  void NavigateAndOpenAppForIsolation(
      GURL navigate_to_url,
      content::WebContents** default_tag_contents1,
      content::WebContents** default_tag_contents2,
      content::WebContents** named_partition_contents1,
      content::WebContents** named_partition_contents2,
      content::WebContents** persistent_partition_contents1,
      content::WebContents** persistent_partition_contents2,
      content::WebContents** persistent_partition_contents3) {
    GURL::Replacements replace_host;
    std::string host_str("localhost");  // Must stay in scope with replace_host.
    replace_host.SetHostStr(host_str);

    navigate_to_url = navigate_to_url.ReplaceComponents(replace_host);

    GURL tag_url1 = embedded_test_server()->GetURL(
        "/extensions/platform_apps/web_view/isolation/cookie.html");
    tag_url1 = tag_url1.ReplaceComponents(replace_host);
    GURL tag_url2 = embedded_test_server()->GetURL(
        "/extensions/platform_apps/web_view/isolation/cookie2.html");
    tag_url2 = tag_url2.ReplaceComponents(replace_host);
    GURL tag_url3 = embedded_test_server()->GetURL(
        "/extensions/platform_apps/web_view/isolation/storage1.html");
    tag_url3 = tag_url3.ReplaceComponents(replace_host);
    GURL tag_url4 = embedded_test_server()->GetURL(
        "/extensions/platform_apps/web_view/isolation/storage2.html");
    tag_url4 = tag_url4.ReplaceComponents(replace_host);
    GURL tag_url5 = embedded_test_server()->GetURL(
        "/extensions/platform_apps/web_view/isolation/storage1.html#p1");
    tag_url5 = tag_url5.ReplaceComponents(replace_host);
    GURL tag_url6 = embedded_test_server()->GetURL(
        "/extensions/platform_apps/web_view/isolation/storage1.html#p2");
    tag_url6 = tag_url6.ReplaceComponents(replace_host);
    GURL tag_url7 = embedded_test_server()->GetURL(
        "/extensions/platform_apps/web_view/isolation/storage1.html#p3");
    tag_url7 = tag_url7.ReplaceComponents(replace_host);

    ui_test_utils::NavigateToURLWithDisposition(
        browser(), navigate_to_url, CURRENT_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    ui_test_utils::UrlLoadObserver observer1(
        tag_url1, content::NotificationService::AllSources());
    ui_test_utils::UrlLoadObserver observer2(
        tag_url2, content::NotificationService::AllSources());
    ui_test_utils::UrlLoadObserver observer3(
        tag_url3, content::NotificationService::AllSources());
    ui_test_utils::UrlLoadObserver observer4(
        tag_url4, content::NotificationService::AllSources());
    ui_test_utils::UrlLoadObserver observer5(
        tag_url5, content::NotificationService::AllSources());
    ui_test_utils::UrlLoadObserver observer6(
        tag_url6, content::NotificationService::AllSources());
    ui_test_utils::UrlLoadObserver observer7(
        tag_url7, content::NotificationService::AllSources());
    LoadAndLaunchPlatformApp("web_view/isolation");
    observer1.Wait();
    observer2.Wait();
    observer3.Wait();
    observer4.Wait();
    observer5.Wait();
    observer6.Wait();
    observer7.Wait();

    content::Source<content::NavigationController> source1 = observer1.source();
    EXPECT_TRUE(source1->GetWebContents()->GetRenderProcessHost()->IsGuest());
    content::Source<content::NavigationController> source2 = observer2.source();
    EXPECT_TRUE(source2->GetWebContents()->GetRenderProcessHost()->IsGuest());
    content::Source<content::NavigationController> source3 = observer3.source();
    EXPECT_TRUE(source3->GetWebContents()->GetRenderProcessHost()->IsGuest());
    content::Source<content::NavigationController> source4 = observer4.source();
    EXPECT_TRUE(source4->GetWebContents()->GetRenderProcessHost()->IsGuest());
    content::Source<content::NavigationController> source5 = observer5.source();
    EXPECT_TRUE(source5->GetWebContents()->GetRenderProcessHost()->IsGuest());
    content::Source<content::NavigationController> source6 = observer6.source();
    EXPECT_TRUE(source6->GetWebContents()->GetRenderProcessHost()->IsGuest());
    content::Source<content::NavigationController> source7 = observer7.source();
    EXPECT_TRUE(source7->GetWebContents()->GetRenderProcessHost()->IsGuest());

    // Check that the first two tags use the same process and it is different
    // than the process used by the other two.
    EXPECT_EQ(source1->GetWebContents()->GetRenderProcessHost()->GetID(),
              source2->GetWebContents()->GetRenderProcessHost()->GetID());
    EXPECT_EQ(source3->GetWebContents()->GetRenderProcessHost()->GetID(),
              source4->GetWebContents()->GetRenderProcessHost()->GetID());
    EXPECT_NE(source1->GetWebContents()->GetRenderProcessHost()->GetID(),
              source3->GetWebContents()->GetRenderProcessHost()->GetID());

    // The two sets of tags should also be isolated from the main browser.
    EXPECT_NE(source1->GetWebContents()->GetRenderProcessHost()->GetID(),
              browser()->tab_strip_model()->GetWebContentsAt(0)->
                  GetRenderProcessHost()->GetID());
    EXPECT_NE(source3->GetWebContents()->GetRenderProcessHost()->GetID(),
              browser()->tab_strip_model()->GetWebContentsAt(0)->
                  GetRenderProcessHost()->GetID());

    // Check that the storage partitions of the first two tags match and are
    // different than the other two.
    EXPECT_EQ(
        source1->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition(),
        source2->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition());
    EXPECT_EQ(
        source3->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition(),
        source4->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition());
    EXPECT_NE(
        source1->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition(),
        source3->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition());

    // Ensure the persistent storage partitions are different.
    EXPECT_EQ(
        source5->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition(),
        source6->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition());
    EXPECT_NE(
        source5->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition(),
        source7->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition());
    EXPECT_NE(
        source1->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition(),
        source5->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition());
    EXPECT_NE(
        source1->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition(),
        source7->GetWebContents()->GetRenderProcessHost()->
            GetStoragePartition());

    *default_tag_contents1 = source1->GetWebContents();
    *default_tag_contents2 = source2->GetWebContents();
    *named_partition_contents1 = source3->GetWebContents();
    *named_partition_contents2 = source4->GetWebContents();
    if (persistent_partition_contents1) {
      *persistent_partition_contents1 = source5->GetWebContents();
    }
    if (persistent_partition_contents2) {
      *persistent_partition_contents2 = source6->GetWebContents();
    }
    if (persistent_partition_contents3) {
      *persistent_partition_contents3 = source7->GetWebContents();
    }
  }

  void ExecuteScriptWaitForTitle(content::WebContents* web_contents,
                                 const char* script,
                                 const char* title) {
    base::string16 expected_title(base::ASCIIToUTF16(title));
    base::string16 error_title(base::ASCIIToUTF16("error"));

    content::TitleWatcher title_watcher(web_contents, expected_title);
    title_watcher.AlsoWaitForTitle(error_title);
    EXPECT_TRUE(content::ExecuteScript(web_contents, script));
    EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());
  }

  // Handles |request| by serving a redirect response.
  static scoped_ptr<net::test_server::HttpResponse> RedirectResponseHandler(
      const std::string& path,
      const GURL& redirect_target,
      const net::test_server::HttpRequest& request) {
    if (!StartsWithASCII(path, request.relative_url, true))
      return scoped_ptr<net::test_server::HttpResponse>();

    scoped_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);
    http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
    http_response->AddCustomHeader("Location", redirect_target.spec());
    return http_response.PassAs<net::test_server::HttpResponse>();
  }

  // Handles |request| by serving an empty response.
  static scoped_ptr<net::test_server::HttpResponse> EmptyResponseHandler(
      const std::string& path,
      const net::test_server::HttpRequest& request) {
    if (StartsWithASCII(path, request.relative_url, true)) {
      return scoped_ptr<net::test_server::HttpResponse>(
          new EmptyHttpResponse);
    }

    return scoped_ptr<net::test_server::HttpResponse>();
  }

  enum TestServer {
    NEEDS_TEST_SERVER,
    NO_TEST_SERVER
  };

  void TestHelper(const std::string& test_name,
                  const std::string& app_location,
                  TestServer test_server) {
    // For serving guest pages.
    if (test_server == NEEDS_TEST_SERVER) {
      if (!StartEmbeddedTestServer()) {
        LOG(ERROR) << "FAILED TO START TEST SERVER.";
        return;
      }
      embedded_test_server()->RegisterRequestHandler(
          base::Bind(&WebViewTest::RedirectResponseHandler,
                    kRedirectResponsePath,
                    embedded_test_server()->GetURL(kRedirectResponseFullPath)));

      embedded_test_server()->RegisterRequestHandler(
          base::Bind(&WebViewTest::EmptyResponseHandler, kEmptyResponsePath));
    }

    ExtensionTestMessageListener launched_listener("Launched", false);
    LoadAndLaunchPlatformApp(app_location.c_str());
    if (!launched_listener.WaitUntilSatisfied()) {
      LOG(ERROR) << "TEST DID NOT LAUNCH.";
      return;
    }

    // Flush any pending events to make sure we start with a clean slate.
    content::RunAllPendingInMessageLoop();

    content::WebContents* embedder_web_contents =
        GetFirstShellWindowWebContents();
    if (!embedder_web_contents) {
      LOG(ERROR) << "UNABLE TO FIND EMBEDDER WEB CONTENTS.";
      return;
    }

    ExtensionTestMessageListener done_listener("TEST_PASSED", false);
    done_listener.AlsoListenForFailureMessage("TEST_FAILED");
    if (!content::ExecuteScript(
            embedder_web_contents,
            base::StringPrintf("runTest('%s')", test_name.c_str()))) {
      LOG(ERROR) << "UNABLE TO START TEST.";
      return;
    }
    ASSERT_TRUE(done_listener.WaitUntilSatisfied());
  }

  content::WebContents* LoadGuest(const std::string& guest_path,
                                  const std::string& app_path) {
    GURL::Replacements replace_host;
    std::string host_str("localhost");  // Must stay in scope with replace_host.
    replace_host.SetHostStr(host_str);

    GURL guest_url = embedded_test_server()->GetURL(guest_path);
    guest_url = guest_url.ReplaceComponents(replace_host);

    ui_test_utils::UrlLoadObserver guest_observer(
        guest_url, content::NotificationService::AllSources());

    ExtensionTestMessageListener guest_loaded_listener("guest-loaded", false);
    LoadAndLaunchPlatformApp(app_path.c_str());
    guest_observer.Wait();

    content::Source<content::NavigationController> source =
        guest_observer.source();
    EXPECT_TRUE(source->GetWebContents()->GetRenderProcessHost()->IsGuest());

    bool satisfied = guest_loaded_listener.WaitUntilSatisfied();
    if (!satisfied)
      return NULL;

    content::WebContents* guest_web_contents = source->GetWebContents();
    return guest_web_contents;
  }

  // Runs media_access/allow tests.
  void MediaAccessAPIAllowTestHelper(const std::string& test_name);

  // Runs media_access/deny tests, each of them are run separately otherwise
  // they timeout (mostly on Windows).
  void MediaAccessAPIDenyTestHelper(const std::string& test_name) {
    ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.
    ExtensionTestMessageListener loaded_listener("loaded", false);
    LoadAndLaunchPlatformApp("web_view/media_access/deny");
    ASSERT_TRUE(loaded_listener.WaitUntilSatisfied());

    content::WebContents* embedder_web_contents =
        GetFirstShellWindowWebContents();
    ASSERT_TRUE(embedder_web_contents);

    ExtensionTestMessageListener test_run_listener("PASSED", false);
    test_run_listener.AlsoListenForFailureMessage("FAILED");
    EXPECT_TRUE(
        content::ExecuteScript(
            embedder_web_contents,
            base::StringPrintf("startDenyTest('%s')", test_name.c_str())));
    ASSERT_TRUE(test_run_listener.WaitUntilSatisfied());
  }

  void WaitForInterstitial(content::WebContents* web_contents) {
    scoped_refptr<content::MessageLoopRunner> loop_runner(
        new content::MessageLoopRunner);
    InterstitialObserver observer(web_contents,
                                  loop_runner->QuitClosure(),
                                  base::Closure());
    if (!content::InterstitialPage::GetInterstitialPage(web_contents))
      loop_runner->Run();
  }

 private:
  bool UsesFakeSpeech() {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();

    // SpeechRecognition test specific SetUp.
    return !strcmp(test_info->name(), "SpeechRecognition") ||
           !strcmp(test_info->name(),
                   "SpeechRecognitionAPI_HasPermissionAllow");
  }

  scoped_ptr<content::FakeSpeechRecognitionManager>
      fake_speech_recognition_manager_;
};

// This test ensures JavaScript errors ("Cannot redefine property") do not
// happen when a <webview> is removed from DOM and added back.
IN_PROC_BROWSER_TEST_F(WebViewTest,
                       AddRemoveWebView_AddRemoveWebView) {
  ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/web_view/addremove"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, AutoSize) {
#if defined(OS_WIN)
  // Flaky on XP bot http://crbug.com/299507
  if (base::win::GetVersion() <= base::win::VERSION_XP)
    return;
#endif

  ASSERT_TRUE(RunPlatformAppTest("platform_apps/web_view/autosize"))
      << message_;
}

#if !defined(OS_CHROMEOS)
// This test ensures <webview> doesn't crash in SW rendering when autosize is
// turned on.
// Flaky on Windows http://crbug.com/299507
#if defined(OS_WIN) || defined(OS_MACOSX)
#define MAYBE_AutoSizeSW DISABLED_AutoSizeSW
#else
#define MAYBE_AutoSizeSW AutoSizeSW
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_AutoSizeSW) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/web_view/autosize"))
      << message_;
}
#endif

// http://crbug.com/326332
IN_PROC_BROWSER_TEST_F(WebViewTest, DISABLED_Shim_TestAutosizeAfterNavigation) {
  TestHelper("testAutosizeAfterNavigation", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestAutosizeBeforeNavigation) {
  TestHelper("testAutosizeBeforeNavigation", "web_view/shim", NO_TEST_SERVER);
}
IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestAutosizeRemoveAttributes) {
  TestHelper("testAutosizeRemoveAttributes", "web_view/shim", NO_TEST_SERVER);
}

// This test is disabled due to being flaky. http://crbug.com/282116
#if defined(OS_WIN)
#define MAYBE_Shim_TestAutosizeWithPartialAttributes \
    DISABLED_Shim_TestAutosizeWithPartialAttributes
#else
#define MAYBE_Shim_TestAutosizeWithPartialAttributes \
    Shim_TestAutosizeWithPartialAttributes
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest,
                       MAYBE_Shim_TestAutosizeWithPartialAttributes) {
  TestHelper("testAutosizeWithPartialAttributes",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestAPIMethodExistence) {
  TestHelper("testAPIMethodExistence", "web_view/shim", NO_TEST_SERVER);
}

// Tests the existence of WebRequest API event objects on the request
// object, on the webview element, and hanging directly off webview.
IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestWebRequestAPIExistence) {
  TestHelper("testWebRequestAPIExistence", "web_view/shim", NO_TEST_SERVER);
}

// http://crbug.com/315920
#if defined(GOOGLE_CHROME_BUILD) && (defined(OS_WIN) || defined(OS_LINUX))
#define MAYBE_Shim_TestChromeExtensionURL DISABLED_Shim_TestChromeExtensionURL
#else
#define MAYBE_Shim_TestChromeExtensionURL Shim_TestChromeExtensionURL
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_Shim_TestChromeExtensionURL) {
  TestHelper("testChromeExtensionURL", "web_view/shim", NO_TEST_SERVER);
}

// http://crbug.com/315920
#if defined(GOOGLE_CHROME_BUILD) && (defined(OS_WIN) || defined(OS_LINUX))
#define MAYBE_Shim_TestChromeExtensionRelativePath \
    DISABLED_Shim_TestChromeExtensionRelativePath
#else
#define MAYBE_Shim_TestChromeExtensionRelativePath \
    Shim_TestChromeExtensionRelativePath
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest,
                       MAYBE_Shim_TestChromeExtensionRelativePath) {
  TestHelper("testChromeExtensionRelativePath",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestInvalidChromeExtensionURL) {
  TestHelper("testInvalidChromeExtensionURL", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestEventName) {
  TestHelper("testEventName", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestOnEventProperty) {
  TestHelper("testOnEventProperties", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestLoadProgressEvent) {
  TestHelper("testLoadProgressEvent", "web_view/shim", NO_TEST_SERVER);
}

// WebViewTest.Shim_TestDestroyOnEventListener is flaky, so disable it.
// http://crbug.com/255106
IN_PROC_BROWSER_TEST_F(WebViewTest, DISABLED_Shim_TestDestroyOnEventListener) {
  TestHelper("testDestroyOnEventListener", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestCannotMutateEventName) {
  TestHelper("testCannotMutateEventName", "web_view/shim", NO_TEST_SERVER);
}

// http://crbug.com/267304
#if defined(OS_WIN)
#define MAYBE_Shim_TestPartitionRaisesException \
  DISABLED_Shim_TestPartitionRaisesException
#else
#define MAYBE_Shim_TestPartitionRaisesException \
  Shim_TestPartitionRaisesException
#endif

IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_Shim_TestPartitionRaisesException) {
  TestHelper("testPartitionRaisesException",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestExecuteScriptFail) {
#if defined(OS_WIN)
  // Flaky on XP bot http://crbug.com/266185
  if (base::win::GetVersion() <= base::win::VERSION_XP)
    return;
#endif

  TestHelper("testExecuteScriptFail", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestExecuteScript) {
  TestHelper("testExecuteScript", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestTerminateAfterExit) {
  TestHelper("testTerminateAfterExit", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestAssignSrcAfterCrash) {
  TestHelper("testAssignSrcAfterCrash", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest,
                       Shim_TestNavOnConsecutiveSrcAttributeChanges) {
  TestHelper("testNavOnConsecutiveSrcAttributeChanges",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestNavOnSrcAttributeChange) {
  TestHelper("testNavOnSrcAttributeChange", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestRemoveSrcAttribute) {
  TestHelper("testRemoveSrcAttribute", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestReassignSrcAttribute) {
  TestHelper("testReassignSrcAttribute", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestBrowserPluginNotAllowed) {
#if defined(OS_WIN)
  // Flaky on XP bots. http://crbug.com/267300
  if (base::win::GetVersion() <= base::win::VERSION_XP)
    return;
#endif

  TestHelper("testBrowserPluginNotAllowed", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestNewWindow) {
  TestHelper("testNewWindow", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestNewWindowTwoListeners) {
  TestHelper("testNewWindowTwoListeners", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestNewWindowNoPreventDefault) {
  TestHelper("testNewWindowNoPreventDefault",
             "web_view/shim",
             NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestNewWindowNoReferrerLink) {
  TestHelper("testNewWindowNoReferrerLink", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestContentLoadEvent) {
  TestHelper("testContentLoadEvent", "web_view/shim", NO_TEST_SERVER);
}

// http://crbug.com/326330
IN_PROC_BROWSER_TEST_F(WebViewTest,
                       DISABLED_Shim_TestDeclarativeWebRequestAPI) {
  TestHelper("testDeclarativeWebRequestAPI",
             "web_view/shim",
             NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestWebRequestAPI) {
  TestHelper("testWebRequestAPI", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestWebRequestAPIGoogleProperty) {
  TestHelper("testWebRequestAPIGoogleProperty",
             "web_view/shim",
             NO_TEST_SERVER);
}

// This test is disabled due to being flaky. http://crbug.com/309451
#if defined(OS_WIN)
#define MAYBE_Shim_TestWebRequestListenerSurvivesReparenting \
    DISABLED_Shim_TestWebRequestListenerSurvivesReparenting
#else
#define MAYBE_Shim_TestWebRequestListenerSurvivesReparenting \
    Shim_TestWebRequestListenerSurvivesReparenting
#endif
IN_PROC_BROWSER_TEST_F(
    WebViewTest,
    MAYBE_Shim_TestWebRequestListenerSurvivesReparenting) {
  TestHelper("testWebRequestListenerSurvivesReparenting",
             "web_view/shim",
             NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestLoadStartLoadRedirect) {
  TestHelper("testLoadStartLoadRedirect", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest,
                       Shim_TestLoadAbortChromeExtensionURLWrongPartition) {
  TestHelper("testLoadAbortChromeExtensionURLWrongPartition",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestLoadAbortEmptyResponse) {
  TestHelper("testLoadAbortEmptyResponse", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestLoadAbortIllegalChromeURL) {
  TestHelper("testLoadAbortIllegalChromeURL",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestLoadAbortIllegalFileURL) {
  TestHelper("testLoadAbortIllegalFileURL", "web_view/shim", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestLoadAbortIllegalJavaScriptURL) {
  TestHelper("testLoadAbortIllegalJavaScriptURL",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestReload) {
  TestHelper("testReload", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestGetProcessId) {
  TestHelper("testGetProcessId", "web_view/shim", NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestRemoveWebviewOnExit) {
  ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.

  // Launch the app and wait until it's ready to load a test.
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("web_view/shim");
  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());

  content::WebContents* embedder_web_contents =
      GetFirstShellWindowWebContents();
  ASSERT_TRUE(embedder_web_contents);

  GURL::Replacements replace_host;
  std::string host_str("localhost");  // Must stay in scope with replace_host.
  replace_host.SetHostStr(host_str);

  std::string guest_path(
      "/extensions/platform_apps/web_view/shim/empty_guest.html");
  GURL guest_url = embedded_test_server()->GetURL(guest_path);
  guest_url = guest_url.ReplaceComponents(replace_host);

  ui_test_utils::UrlLoadObserver guest_observer(
      guest_url, content::NotificationService::AllSources());

  // Run the test and wait until the guest WebContents is available and has
  // finished loading.
  ExtensionTestMessageListener guest_loaded_listener("guest-loaded", false);
  EXPECT_TRUE(content::ExecuteScript(
                  embedder_web_contents,
                  "runTest('testRemoveWebviewOnExit')"));
  guest_observer.Wait();

  content::Source<content::NavigationController> source =
      guest_observer.source();
  EXPECT_TRUE(source->GetWebContents()->GetRenderProcessHost()->IsGuest());

  ASSERT_TRUE(guest_loaded_listener.WaitUntilSatisfied());

  content::WebContentsDestroyedWatcher destroyed_watcher(
      source->GetWebContents());

  // Tell the embedder to kill the guest.
  EXPECT_TRUE(content::ExecuteScript(
                  embedder_web_contents,
                  "removeWebviewOnExitDoCrash();"));

  // Wait until the guest WebContents is destroyed.
  destroyed_watcher.Wait();
}

// Remove <webview> immediately after navigating it.
// This is a regression test for http://crbug.com/276023.
IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestRemoveWebviewAfterNavigation) {
  TestHelper("testRemoveWebviewAfterNavigation",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestNavigationToExternalProtocol) {
  TestHelper("testNavigationToExternalProtocol",
             "web_view/shim",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Shim_TestResizeWebviewResizesContent) {
  TestHelper("testResizeWebviewResizesContent",
             "web_view/shim",
             NO_TEST_SERVER);
}

// This test makes sure we do not crash if app is closed while interstitial
// page is being shown in guest.
IN_PROC_BROWSER_TEST_F(WebViewTest, InterstitialTeardown) {
#if defined(OS_WIN)
  // Flaky on XP bot http://crbug.com/297014
  if (base::win::GetVersion() <= base::win::VERSION_XP)
    return;
#endif

  // Start a HTTPS server so we can load an interstitial page inside guest.
  net::SpawnedTestServer::SSLOptions ssl_options;
  ssl_options.server_certificate =
      net::SpawnedTestServer::SSLOptions::CERT_MISMATCHED_NAME;
  net::SpawnedTestServer https_server(
      net::SpawnedTestServer::TYPE_HTTPS, ssl_options,
      base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
  ASSERT_TRUE(https_server.Start());

  net::HostPortPair host_and_port = https_server.host_port_pair();

  ExtensionTestMessageListener embedder_loaded_listener("EmbedderLoaded",
                                                        false);
  LoadAndLaunchPlatformApp("web_view/interstitial_teardown");
  ASSERT_TRUE(embedder_loaded_listener.WaitUntilSatisfied());

  GuestContentBrowserClient new_client;
  content::ContentBrowserClient* old_client =
      SetBrowserClientForTesting(&new_client);

  // Now load the guest.
  content::WebContents* embedder_web_contents =
      GetFirstShellWindowWebContents();
  ExtensionTestMessageListener second("GuestAddedToDom", false);
  EXPECT_TRUE(content::ExecuteScript(
      embedder_web_contents,
      base::StringPrintf("loadGuest(%d);\n", host_and_port.port())));
  ASSERT_TRUE(second.WaitUntilSatisfied());

  // Wait for interstitial page to be shown in guest.
  content::WebContents* guest_web_contents = new_client.WaitForGuestCreated();
  SetBrowserClientForTesting(old_client);
  ASSERT_TRUE(guest_web_contents->GetRenderProcessHost()->IsGuest());
  WaitForInterstitial(guest_web_contents);

  // Now close the app while interstitial page being shown in guest.
  apps::ShellWindow* window = GetFirstShellWindow();
  window->GetBaseWindow()->Close();
}

IN_PROC_BROWSER_TEST_F(WebViewTest, ShimSrcAttribute) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/web_view/src_attribute"))
      << message_;
}

// This test verifies that prerendering has been disabled inside <webview>.
// This test is here rather than in PrerenderBrowserTest for testing convenience
// only. If it breaks then this is a bug in the prerenderer.
IN_PROC_BROWSER_TEST_F(WebViewTest, NoPrerenderer) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  content::WebContents* guest_web_contents =
      LoadGuest(
          "/extensions/platform_apps/web_view/noprerenderer/guest.html",
          "web_view/noprerenderer");
  ASSERT_TRUE(guest_web_contents != NULL);

  PrerenderLinkManager* prerender_link_manager =
      PrerenderLinkManagerFactory::GetForProfile(
          Profile::FromBrowserContext(guest_web_contents->GetBrowserContext()));
  ASSERT_TRUE(prerender_link_manager != NULL);
  EXPECT_TRUE(prerender_link_manager->IsEmpty());
}

// This tests cookie isolation for packaged apps with webview tags. It navigates
// the main browser window to a page that sets a cookie and loads an app with
// multiple webview tags. Each tag sets a cookie and the test checks the proper
// storage isolation is enforced.
// This test is disabled due to being flaky. http://crbug.com/294196
#if defined(OS_WIN)
#define MAYBE_CookieIsolation DISABLED_CookieIsolation
#else
#define MAYBE_CookieIsolation CookieIsolation
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_CookieIsolation) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  const std::string kExpire =
      "var expire = new Date(Date.now() + 24 * 60 * 60 * 1000);";
  std::string cookie_script1(kExpire);
  cookie_script1.append(
      "document.cookie = 'guest1=true; path=/; expires=' + expire + ';';");
  std::string cookie_script2(kExpire);
  cookie_script2.append(
      "document.cookie = 'guest2=true; path=/; expires=' + expire + ';';");

  GURL::Replacements replace_host;
  std::string host_str("localhost");  // Must stay in scope with replace_host.
  replace_host.SetHostStr(host_str);

  GURL set_cookie_url = embedded_test_server()->GetURL(
      "/extensions/platform_apps/isolation/set_cookie.html");
  set_cookie_url = set_cookie_url.ReplaceComponents(replace_host);

  // The first two partitions will be used to set cookies and ensure they are
  // shared. The named partition is used to ensure that cookies are isolated
  // between partitions within the same app.
  content::WebContents* cookie_contents1;
  content::WebContents* cookie_contents2;
  content::WebContents* named_partition_contents1;
  content::WebContents* named_partition_contents2;

  NavigateAndOpenAppForIsolation(set_cookie_url, &cookie_contents1,
                                 &cookie_contents2, &named_partition_contents1,
                                 &named_partition_contents2, NULL, NULL, NULL);

  EXPECT_TRUE(content::ExecuteScript(cookie_contents1, cookie_script1));
  EXPECT_TRUE(content::ExecuteScript(cookie_contents2, cookie_script2));

  int cookie_size;
  std::string cookie_value;

  // Test the regular browser context to ensure we have only one cookie.
  automation_util::GetCookies(GURL("http://localhost"),
                              browser()->tab_strip_model()->GetWebContentsAt(0),
                              &cookie_size, &cookie_value);
  EXPECT_EQ("testCookie=1", cookie_value);

  // The default behavior is to combine webview tags with no explicit partition
  // declaration into the same in-memory partition. Test the webview tags to
  // ensure we have properly set the cookies and we have both cookies in both
  // tags.
  automation_util::GetCookies(GURL("http://localhost"),
                              cookie_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("guest1=true; guest2=true", cookie_value);

  automation_util::GetCookies(GURL("http://localhost"),
                              cookie_contents2,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("guest1=true; guest2=true", cookie_value);

  // The third tag should not have any cookies as it is in a separate partition.
  automation_util::GetCookies(GURL("http://localhost"),
                              named_partition_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("", cookie_value);
}

// This tests that in-memory storage partitions are reset on browser restart,
// but persistent ones maintain state for cookies and HTML5 storage.
IN_PROC_BROWSER_TEST_F(WebViewTest, PRE_StoragePersistence) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  const std::string kExpire =
      "var expire = new Date(Date.now() + 24 * 60 * 60 * 1000);";
  std::string cookie_script1(kExpire);
  cookie_script1.append(
      "document.cookie = 'inmemory=true; path=/; expires=' + expire + ';';");
  std::string cookie_script2(kExpire);
  cookie_script2.append(
      "document.cookie = 'persist1=true; path=/; expires=' + expire + ';';");
  std::string cookie_script3(kExpire);
  cookie_script3.append(
      "document.cookie = 'persist2=true; path=/; expires=' + expire + ';';");

  // We don't care where the main browser is on this test.
  GURL blank_url("about:blank");

  // The first two partitions will be used to set cookies and ensure they are
  // shared. The named partition is used to ensure that cookies are isolated
  // between partitions within the same app.
  content::WebContents* cookie_contents1;
  content::WebContents* cookie_contents2;
  content::WebContents* named_partition_contents1;
  content::WebContents* named_partition_contents2;
  content::WebContents* persistent_partition_contents1;
  content::WebContents* persistent_partition_contents2;
  content::WebContents* persistent_partition_contents3;
  NavigateAndOpenAppForIsolation(blank_url, &cookie_contents1,
                                 &cookie_contents2, &named_partition_contents1,
                                 &named_partition_contents2,
                                 &persistent_partition_contents1,
                                 &persistent_partition_contents2,
                                 &persistent_partition_contents3);

  // Set the inmemory=true cookie for tags with inmemory partitions.
  EXPECT_TRUE(content::ExecuteScript(cookie_contents1, cookie_script1));
  EXPECT_TRUE(content::ExecuteScript(named_partition_contents1,
                                     cookie_script1));

  // For the two different persistent storage partitions, set the
  // two different cookies so we can check that they aren't comingled below.
  EXPECT_TRUE(content::ExecuteScript(persistent_partition_contents1,
                                     cookie_script2));

  EXPECT_TRUE(content::ExecuteScript(persistent_partition_contents3,
                                     cookie_script3));

  int cookie_size;
  std::string cookie_value;

  // Check that all in-memory partitions have a cookie set.
  automation_util::GetCookies(GURL("http://localhost"),
                              cookie_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("inmemory=true", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              cookie_contents2,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("inmemory=true", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              named_partition_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("inmemory=true", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              named_partition_contents2,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("inmemory=true", cookie_value);

  // Check that all persistent partitions kept their state.
  automation_util::GetCookies(GURL("http://localhost"),
                              persistent_partition_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("persist1=true", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              persistent_partition_contents2,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("persist1=true", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              persistent_partition_contents3,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("persist2=true", cookie_value);
}

// This is the post-reset portion of the StoragePersistence test.  See
// PRE_StoragePersistence for main comment.
IN_PROC_BROWSER_TEST_F(WebViewTest, DISABLED_StoragePersistence) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  // We don't care where the main browser is on this test.
  GURL blank_url("about:blank");

  // The first two partitions will be used to set cookies and ensure they are
  // shared. The named partition is used to ensure that cookies are isolated
  // between partitions within the same app.
  content::WebContents* cookie_contents1;
  content::WebContents* cookie_contents2;
  content::WebContents* named_partition_contents1;
  content::WebContents* named_partition_contents2;
  content::WebContents* persistent_partition_contents1;
  content::WebContents* persistent_partition_contents2;
  content::WebContents* persistent_partition_contents3;
  NavigateAndOpenAppForIsolation(blank_url, &cookie_contents1,
                                 &cookie_contents2, &named_partition_contents1,
                                 &named_partition_contents2,
                                 &persistent_partition_contents1,
                                 &persistent_partition_contents2,
                                 &persistent_partition_contents3);

  int cookie_size;
  std::string cookie_value;

  // Check that all in-memory partitions lost their state.
  automation_util::GetCookies(GURL("http://localhost"),
                              cookie_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              cookie_contents2,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              named_partition_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              named_partition_contents2,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("", cookie_value);

  // Check that all persistent partitions kept their state.
  automation_util::GetCookies(GURL("http://localhost"),
                              persistent_partition_contents1,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("persist1=true", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              persistent_partition_contents2,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("persist1=true", cookie_value);
  automation_util::GetCookies(GURL("http://localhost"),
                              persistent_partition_contents3,
                              &cookie_size, &cookie_value);
  EXPECT_EQ("persist2=true", cookie_value);
}

#if defined(OS_WIN)
// This test is very flaky on Win Aura, Win XP, Win 7. http://crbug.com/248873
#define MAYBE_DOMStorageIsolation DISABLED_DOMStorageIsolation
#else
#define MAYBE_DOMStorageIsolation DOMStorageIsolation
#endif

// This tests DOM storage isolation for packaged apps with webview tags. It
// loads an app with multiple webview tags and each tag sets DOM storage
// entries, which the test checks to ensure proper storage isolation is
// enforced.
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_DOMStorageIsolation) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  GURL regular_url = embedded_test_server()->GetURL("/title1.html");

  std::string output;
  std::string get_local_storage("window.domAutomationController.send("
      "window.localStorage.getItem('foo') || 'badval')");
  std::string get_session_storage("window.domAutomationController.send("
      "window.sessionStorage.getItem('bar') || 'badval')");

  content::WebContents* default_tag_contents1;
  content::WebContents* default_tag_contents2;
  content::WebContents* storage_contents1;
  content::WebContents* storage_contents2;

  NavigateAndOpenAppForIsolation(regular_url, &default_tag_contents1,
                                 &default_tag_contents2, &storage_contents1,
                                 &storage_contents2, NULL, NULL, NULL);

  // Initialize the storage for the first of the two tags that share a storage
  // partition.
  EXPECT_TRUE(content::ExecuteScript(storage_contents1,
                                     "initDomStorage('page1')"));

  // Let's test that the expected values are present in the first tag, as they
  // will be overwritten once we call the initDomStorage on the second tag.
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents1,
                                            get_local_storage.c_str(),
                                            &output));
  EXPECT_STREQ("local-page1", output.c_str());
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents1,
                                            get_session_storage.c_str(),
                                            &output));
  EXPECT_STREQ("session-page1", output.c_str());

  // Now, init the storage in the second tag in the same storage partition,
  // which will overwrite the shared localStorage.
  EXPECT_TRUE(content::ExecuteScript(storage_contents2,
                                     "initDomStorage('page2')"));

  // The localStorage value now should reflect the one written through the
  // second tag.
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents1,
                                            get_local_storage.c_str(),
                                            &output));
  EXPECT_STREQ("local-page2", output.c_str());
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents2,
                                            get_local_storage.c_str(),
                                            &output));
  EXPECT_STREQ("local-page2", output.c_str());

  // Session storage is not shared though, as each webview tag has separate
  // instance, even if they are in the same storage partition.
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents1,
                                            get_session_storage.c_str(),
                                            &output));
  EXPECT_STREQ("session-page1", output.c_str());
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents2,
                                            get_session_storage.c_str(),
                                            &output));
  EXPECT_STREQ("session-page2", output.c_str());

  // Also, let's check that the main browser and another tag that doesn't share
  // the same partition don't have those values stored.
  EXPECT_TRUE(ExecuteScriptAndExtractString(
      browser()->tab_strip_model()->GetWebContentsAt(0),
      get_local_storage.c_str(),
      &output));
  EXPECT_STREQ("badval", output.c_str());
  EXPECT_TRUE(ExecuteScriptAndExtractString(
      browser()->tab_strip_model()->GetWebContentsAt(0),
      get_session_storage.c_str(),
      &output));
  EXPECT_STREQ("badval", output.c_str());
  EXPECT_TRUE(ExecuteScriptAndExtractString(default_tag_contents1,
                                            get_local_storage.c_str(),
                                            &output));
  EXPECT_STREQ("badval", output.c_str());
  EXPECT_TRUE(ExecuteScriptAndExtractString(default_tag_contents1,
                                            get_session_storage.c_str(),
                                            &output));
  EXPECT_STREQ("badval", output.c_str());
}

// See crbug.com/248500
#if defined(OS_WIN)
#define MAYBE_IndexedDBIsolation DISABLED_IndexedDBIsolation
#else
#define MAYBE_IndexedDBIsolation IndexedDBIsolation
#endif

// This tests IndexedDB isolation for packaged apps with webview tags. It loads
// an app with multiple webview tags and each tag creates an IndexedDB record,
// which the test checks to ensure proper storage isolation is enforced.
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_IndexedDBIsolation) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  GURL regular_url = embedded_test_server()->GetURL("/title1.html");

  content::WebContents* default_tag_contents1;
  content::WebContents* default_tag_contents2;
  content::WebContents* storage_contents1;
  content::WebContents* storage_contents2;

  NavigateAndOpenAppForIsolation(regular_url, &default_tag_contents1,
                                 &default_tag_contents2, &storage_contents1,
                                 &storage_contents2, NULL, NULL, NULL);

  // Initialize the storage for the first of the two tags that share a storage
  // partition.
  ExecuteScriptWaitForTitle(storage_contents1, "initIDB()", "idb created");
  ExecuteScriptWaitForTitle(storage_contents1, "addItemIDB(7, 'page1')",
                            "addItemIDB complete");
  ExecuteScriptWaitForTitle(storage_contents1, "readItemIDB(7)",
                            "readItemIDB complete");

  std::string output;
  std::string get_value(
      "window.domAutomationController.send(getValueIDB() || 'badval')");

  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents1,
                                            get_value.c_str(), &output));
  EXPECT_STREQ("page1", output.c_str());

  // Initialize the db in the second tag.
  ExecuteScriptWaitForTitle(storage_contents2, "initIDB()", "idb open");

  // Since we share a partition, reading the value should return the existing
  // one.
  ExecuteScriptWaitForTitle(storage_contents2, "readItemIDB(7)",
                            "readItemIDB complete");
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents2,
                                            get_value.c_str(), &output));
  EXPECT_STREQ("page1", output.c_str());

  // Now write through the second tag and read it back.
  ExecuteScriptWaitForTitle(storage_contents2, "addItemIDB(7, 'page2')",
                            "addItemIDB complete");
  ExecuteScriptWaitForTitle(storage_contents2, "readItemIDB(7)",
                            "readItemIDB complete");
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents2,
                                            get_value.c_str(), &output));
  EXPECT_STREQ("page2", output.c_str());

  // Reset the document title, otherwise the next call will not see a change and
  // will hang waiting for it.
  EXPECT_TRUE(content::ExecuteScript(storage_contents1,
                                     "document.title = 'foo'"));

  // Read through the first tag to ensure we have the second value.
  ExecuteScriptWaitForTitle(storage_contents1, "readItemIDB(7)",
                            "readItemIDB complete");
  EXPECT_TRUE(ExecuteScriptAndExtractString(storage_contents1,
                                            get_value.c_str(), &output));
  EXPECT_STREQ("page2", output.c_str());

  // Now, let's confirm there is no database in the main browser and another
  // tag that doesn't share the same partition. Due to the IndexedDB API design,
  // open will succeed, but the version will be 1, since it creates the database
  // if it is not found. The two tags use database version 3, so we avoid
  // ambiguity.
  const char* script =
      "indexedDB.open('isolation').onsuccess = function(e) {"
      "  if (e.target.result.version == 1)"
      "    document.title = 'db not found';"
      "  else "
      "    document.title = 'error';"
      "}";
  ExecuteScriptWaitForTitle(browser()->tab_strip_model()->GetWebContentsAt(0),
                            script, "db not found");
  ExecuteScriptWaitForTitle(default_tag_contents1, script, "db not found");
}

// This test ensures that closing app window on 'loadcommit' does not crash.
// The test launches an app with guest and closes the window on loadcommit. It
// then launches the app window again. The process is repeated 3 times.
// http://crbug.com/291278
#if defined(OS_WIN)
#define MAYBE_CloseOnLoadcommit DISABLED_CloseOnLoadcommit
#else
#define MAYBE_CloseOnLoadcommit CloseOnLoadcommit
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_CloseOnLoadcommit) {
  ExtensionTestMessageListener done_test_listener(
      "done-close-on-loadcommit", false);
  LoadAndLaunchPlatformApp("web_view/close_on_loadcommit");
  ASSERT_TRUE(done_test_listener.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(WebViewTest, MediaAccessAPIDeny_TestDeny) {
  MediaAccessAPIDenyTestHelper("testDeny");
}

IN_PROC_BROWSER_TEST_F(WebViewTest,
                       MediaAccessAPIDeny_TestDenyThenAllowThrows) {
  MediaAccessAPIDenyTestHelper("testDenyThenAllowThrows");

}

IN_PROC_BROWSER_TEST_F(WebViewTest,
                       MediaAccessAPIDeny_TestDenyWithPreventDefault) {
  MediaAccessAPIDenyTestHelper("testDenyWithPreventDefault");
}

IN_PROC_BROWSER_TEST_F(WebViewTest,
                       MediaAccessAPIDeny_TestNoListenersImplyDeny) {
  MediaAccessAPIDenyTestHelper("testNoListenersImplyDeny");
}

IN_PROC_BROWSER_TEST_F(WebViewTest,
                       MediaAccessAPIDeny_TestNoPreventDefaultImpliesDeny) {
  MediaAccessAPIDenyTestHelper("testNoPreventDefaultImpliesDeny");
}

void WebViewTest::MediaAccessAPIAllowTestHelper(const std::string& test_name) {
  ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("web_view/media_access/allow");
  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());

  content::WebContents* embedder_web_contents =
      GetFirstShellWindowWebContents();
  ASSERT_TRUE(embedder_web_contents);
  MockWebContentsDelegate* mock = new MockWebContentsDelegate;
  embedder_web_contents->SetDelegate(mock);

  ExtensionTestMessageListener done_listener("TEST_PASSED", false);
  done_listener.AlsoListenForFailureMessage("TEST_FAILED");
  EXPECT_TRUE(
      content::ExecuteScript(
          embedder_web_contents,
          base::StringPrintf("startAllowTest('%s')",
                             test_name.c_str())));
  ASSERT_TRUE(done_listener.WaitUntilSatisfied());

  mock->WaitForSetMediaPermission();
}

IN_PROC_BROWSER_TEST_F(WebViewTest, MediaAccessAPIAllow_TestAllow) {
  MediaAccessAPIAllowTestHelper("testAllow");
}

IN_PROC_BROWSER_TEST_F(WebViewTest, MediaAccessAPIAllow_TestAllowAndThenDeny) {
  MediaAccessAPIAllowTestHelper("testAllowAndThenDeny");
}

IN_PROC_BROWSER_TEST_F(WebViewTest, MediaAccessAPIAllow_TestAllowTwice) {
  MediaAccessAPIAllowTestHelper("testAllowTwice");
}

IN_PROC_BROWSER_TEST_F(WebViewTest, MediaAccessAPIAllow_TestAllowAsync) {
  MediaAccessAPIAllowTestHelper("testAllowAsync");
}

// Checks that window.screenX/screenY/screenLeft/screenTop works correctly for
// guests.
IN_PROC_BROWSER_TEST_F(WebViewTest, ScreenCoordinates) {
  ASSERT_TRUE(RunPlatformAppTestWithArg(
      "platform_apps/web_view/common", "screen_coordinates"))
          << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, SpeechRecognition) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  content::WebContents* guest_web_contents = LoadGuest(
      "/extensions/platform_apps/web_view/speech/guest.html",
      "web_view/speech");
  ASSERT_TRUE(guest_web_contents);

  // Click on the guest (center of the WebContents), the guest is rendered in a
  // way that this will trigger clicking on speech recognition input mic.
  SimulateMouseClick(guest_web_contents, 0, blink::WebMouseEvent::ButtonLeft);

  base::string16 expected_title(base::ASCIIToUTF16("PASSED"));
  base::string16 error_title(base::ASCIIToUTF16("FAILED"));
  content::TitleWatcher title_watcher(guest_web_contents, expected_title);
  title_watcher.AlsoWaitForTitle(error_title);
  EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());
}

// Flaky on Windows. http://crbug.com/303966
#if defined(OS_WIN)
#define MAYBE_TearDownTest DISABLED_TearDownTest
#else
#define MAYBE_TearDownTest TearDownTest
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_TearDownTest) {
  ExtensionTestMessageListener first_loaded_listener("guest-loaded", false);
  const extensions::Extension* extension =
      LoadAndLaunchPlatformApp("web_view/teardown");
  ASSERT_TRUE(first_loaded_listener.WaitUntilSatisfied());
  apps::ShellWindow* window = NULL;
  if (!GetShellWindowCount())
    window = CreateShellWindow(extension);
  else
    window = GetFirstShellWindow();
  CloseShellWindow(window);

  // Load the app again.
  ExtensionTestMessageListener second_loaded_listener("guest-loaded", false);
  LoadAndLaunchPlatformApp("web_view/teardown");
  ASSERT_TRUE(second_loaded_listener.WaitUntilSatisfied());
}

// In following GeolocationAPIEmbedderHasNoAccess* tests, embedder (i.e. the
// platform app) does not have geolocation permission for this test.
// No matter what the API does, geolocation permission would be denied.
// Note that the test name prefix must be "GeolocationAPI".
IN_PROC_BROWSER_TEST_F(WebViewTest, GeolocationAPIEmbedderHasNoAccessAllow) {
  TestHelper("testDenyDenies",
             "web_view/geolocation/embedder_has_no_permission",
             NEEDS_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, GeolocationAPIEmbedderHasNoAccessDeny) {
  TestHelper("testDenyDenies",
             "web_view/geolocation/embedder_has_no_permission",
             NEEDS_TEST_SERVER);
}

// In following GeolocationAPIEmbedderHasAccess* tests, embedder (i.e. the
// platform app) has geolocation permission
//
// Note that these test names must be "GeolocationAPI" prefixed (b/c we mock out
// geolocation in this case).
//
// Also note that these are run separately because OverrideGeolocation() doesn't
// mock out geolocation for multiple navigator.geolocation calls properly and
// the tests become flaky.
// GeolocationAPI* test 1 of 3.
IN_PROC_BROWSER_TEST_F(WebViewTest, GeolocationAPIEmbedderHasAccessAllow) {
  TestHelper("testAllow",
             "web_view/geolocation/embedder_has_permission",
             NEEDS_TEST_SERVER);
}

// GeolocationAPI* test 2 of 3.
IN_PROC_BROWSER_TEST_F(WebViewTest, GeolocationAPIEmbedderHasAccessDeny) {
  TestHelper("testDeny",
             "web_view/geolocation/embedder_has_permission",
             NEEDS_TEST_SERVER);
}

// GeolocationAPI* test 3 of 3.
IN_PROC_BROWSER_TEST_F(WebViewTest,
                       GeolocationAPIEmbedderHasAccessMultipleBridgeIdAllow) {
  TestHelper("testMultipleBridgeIdAllow",
             "web_view/geolocation/embedder_has_permission",
             NEEDS_TEST_SERVER);
}

// Tests that
// BrowserPluginGeolocationPermissionContext::CancelGeolocationPermissionRequest
// is handled correctly (and does not crash).
IN_PROC_BROWSER_TEST_F(WebViewTest, GeolocationAPICancelGeolocation) {
  ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.
  ASSERT_TRUE(RunPlatformAppTest(
        "platform_apps/web_view/geolocation/cancel_request")) << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, DISABLED_GeolocationRequestGone) {
  ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.
  ASSERT_TRUE(RunPlatformAppTest(
        "platform_apps/web_view/geolocation/geolocation_request_gone"))
            << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, ClearData) {
#if defined(OS_WIN)
  // Flaky on XP bot http://crbug.com/282674
  if (base::win::GetVersion() <= base::win::VERSION_XP)
    return;
#endif

  ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.
  ASSERT_TRUE(RunPlatformAppTestWithArg(
      "platform_apps/web_view/common", "cleardata"))
          << message_;
}

// This test is disabled on Win due to being flaky. http://crbug.com/294592
#if defined(OS_WIN)
#define MAYBE_ConsoleMessage DISABLED_ConsoleMessage
#else
#define MAYBE_ConsoleMessage ConsoleMessage
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_ConsoleMessage) {
  ASSERT_TRUE(RunPlatformAppTestWithArg(
      "platform_apps/web_view/common", "console_messages"))
          << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, DownloadPermission) {
  ASSERT_TRUE(StartEmbeddedTestServer());  // For serving guest pages.
  content::WebContents* guest_web_contents =
      LoadGuest("/extensions/platform_apps/web_view/download/guest.html",
                "web_view/download");
  ASSERT_TRUE(guest_web_contents);

  // Replace WebContentsDelegate with mock version so we can intercept download
  // requests.
  content::WebContentsDelegate* delegate = guest_web_contents->GetDelegate();
  MockDownloadWebContentsDelegate* mock_delegate =
      new MockDownloadWebContentsDelegate(delegate);
  guest_web_contents->SetDelegate(mock_delegate);

  // Start test.
  // 1. Guest requests a download that its embedder denies.
  EXPECT_TRUE(content::ExecuteScript(guest_web_contents,
                                     "startDownload('download-link-1')"));
  mock_delegate->WaitForCanDownload(false); // Expect to not allow.
  mock_delegate->Reset();

  // 2. Guest requests a download that its embedder allows.
  EXPECT_TRUE(content::ExecuteScript(guest_web_contents,
                                     "startDownload('download-link-2')"));
  mock_delegate->WaitForCanDownload(true); // Expect to allow.
  mock_delegate->Reset();

  // 3. Guest requests a download that its embedder ignores, this implies deny.
  EXPECT_TRUE(content::ExecuteScript(guest_web_contents,
                                     "startDownload('download-link-3')"));
  mock_delegate->WaitForCanDownload(false); // Expect to not allow.
}

// This test makes sure loading <webview> does not crash when there is an
// extension which has content script whitelisted/forced.
IN_PROC_BROWSER_TEST_F(WebViewTest, WhitelistedContentScript) {
  // Whitelist the extension for running content script we are going to load.
  extensions::ExtensionsClient::ScriptingWhitelist whitelist;
  const std::string extension_id = "imeongpbjoodlnmlakaldhlcmijmhpbb";
  whitelist.push_back(extension_id);
  extensions::ExtensionsClient::Get()->SetScriptingWhitelist(whitelist);

  // Load the extension.
  const extensions::Extension* content_script_whitelisted_extension =
      LoadExtension(test_data_dir_.AppendASCII(
                        "platform_apps/web_view/extension_api/content_script"));
  ASSERT_TRUE(content_script_whitelisted_extension);
  ASSERT_EQ(extension_id, content_script_whitelisted_extension->id());

  // Now load an app with <webview>.
  ExtensionTestMessageListener done_listener("TEST_PASSED", false);
  LoadAndLaunchPlatformApp("web_view/content_script_whitelisted");
  ASSERT_TRUE(done_listener.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(WebViewTest, SetPropertyOnDocumentReady) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/web_view/document_ready"))
                  << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, SetPropertyOnDocumentInteractive) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/web_view/document_interactive"))
                  << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, SpeechRecognitionAPI_HasPermissionAllow) {
  ASSERT_TRUE(
      RunPlatformAppTestWithArg("platform_apps/web_view/speech_recognition_api",
                                "allowTest"))
          << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, SpeechRecognitionAPI_HasPermissionDeny) {
  ASSERT_TRUE(
      RunPlatformAppTestWithArg("platform_apps/web_view/speech_recognition_api",
                                "denyTest"))
          << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, SpeechRecognitionAPI_NoPermission) {
  ASSERT_TRUE(
      RunPlatformAppTestWithArg("platform_apps/web_view/common",
                                "speech_recognition_api_no_permission"))
          << message_;
}

// Tests overriding user agent.
IN_PROC_BROWSER_TEST_F(WebViewTest, UserAgent) {
  ASSERT_TRUE(RunPlatformAppTestWithArg(
              "platform_apps/web_view/common", "useragent")) << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, UserAgent_NewWindow) {
  ASSERT_TRUE(RunPlatformAppTestWithArg(
              "platform_apps/web_view/common",
              "useragent_newwindow")) << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, NoPermission) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/web_view/nopermission"))
                  << message_;
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Dialog_TestAlertDialog) {
  TestHelper("testAlertDialog", "web_view/dialog", NO_TEST_SERVER);
}

// Fails on official Windows and Linux builds. See http://crbug.com/313868
#if defined(GOOGLE_CHROME_BUILD) && (defined(OS_WIN) || defined(OS_LINUX))
#define MAYBE_Dialog_TestConfirmDialog DISABLED_Dialog_TestConfirmDialog
#else
#define MAYBE_Dialog_TestConfirmDialog Dialog_TestConfirmDialog
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_Dialog_TestConfirmDialog) {
  TestHelper("testConfirmDialog", "web_view/dialog", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Dialog_TestConfirmDialogCancel) {
  TestHelper("testConfirmDialogCancel", "web_view/dialog", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Dialog_TestConfirmDialogDefaultCancel) {
  TestHelper("testConfirmDialogDefaultCancel",
             "web_view/dialog",
             NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, Dialog_TestConfirmDialogDefaultGCCancel) {
  TestHelper("testConfirmDialogDefaultGCCancel",
             "web_view/dialog",
             NO_TEST_SERVER);
}

// Fails on official Windows and Linux builds. See http://crbug.com/313868
#if defined(GOOGLE_CHROME_BUILD) && (defined(OS_WIN) || defined(OS_LINUX))
#define MAYBE_Dialog_TestPromptDialog DISABLED_Dialog_TestPromptDialog
#else
#define MAYBE_Dialog_TestPromptDialog Dialog_TestPromptDialog
#endif
IN_PROC_BROWSER_TEST_F(WebViewTest, MAYBE_Dialog_TestPromptDialog) {
  TestHelper("testPromptDialog", "web_view/dialog", NO_TEST_SERVER);
}

IN_PROC_BROWSER_TEST_F(WebViewTest, NoContentSettingsAPI) {
  // Load the extension.
  const extensions::Extension* content_settings_extension =
      LoadExtension(
          test_data_dir_.AppendASCII(
              "platform_apps/web_view/extension_api/content_settings"));
  ASSERT_TRUE(content_settings_extension);
  TestHelper("testPostMessageCommChannel", "web_view/shim", NO_TEST_SERVER);
}

#if defined(ENABLE_PLUGINS)
class WebViewPluginTest : public WebViewTest {
 protected:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    WebViewTest::SetUpCommandLine(command_line);

    // Append the switch to register the pepper plugin.
    // library name = <out dir>/<test_name>.<library_extension>
    // MIME type = application/x-ppapi-<test_name>
    base::FilePath plugin_dir;
    EXPECT_TRUE(PathService::Get(base::DIR_MODULE, &plugin_dir));

    base::FilePath plugin_lib = plugin_dir.Append(library_name);
    EXPECT_TRUE(base::PathExists(plugin_lib));
    base::FilePath::StringType pepper_plugin = plugin_lib.value();
    pepper_plugin.append(FILE_PATH_LITERAL(";application/x-ppapi-tests"));
    command_line->AppendSwitchNative(switches::kRegisterPepperPlugins,
                                     pepper_plugin);
  }
};

IN_PROC_BROWSER_TEST_F(WebViewPluginTest, TestLoadPluginEvent) {
  TestHelper("testPluginLoadPermission", "web_view/shim", NO_TEST_SERVER);
}
#endif  // defined(ENABLE_PLUGINS)

class WebViewCaptureTest : public WebViewTest,
  public testing::WithParamInterface<std::string> {
 public:
  WebViewCaptureTest() {}
  virtual ~WebViewCaptureTest() {}
  virtual void SetUp() OVERRIDE {
    EnablePixelOutput();
    WebViewTest::SetUp();
  }
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    command_line->AppendSwitch(GetParam());
    // http://crbug.com/327035
    command_line->AppendSwitch(switches::kDisableDelegatedRenderer);
    WebViewTest::SetUpCommandLine(command_line);
  }
};

// <webview> screenshot capture fails with ubercomp.
// See http://crbug.com/327035.
IN_PROC_BROWSER_TEST_P(WebViewCaptureTest,
                       DISABLED_Shim_ScreenshotCapture) {
  TestHelper("testScreenshotCapture", "web_view/shim", NO_TEST_SERVER);
}

INSTANTIATE_TEST_CASE_P(WithoutThreadedCompositor,
    WebViewCaptureTest,
    ::testing::Values(std::string(switches::kDisableThreadedCompositing)));

// http://crbug.com/171744
#if !defined(OS_MACOSX)
INSTANTIATE_TEST_CASE_P(WithThreadedCompositor,
    WebViewCaptureTest,
    ::testing::Values(std::string(switches::kEnableThreadedCompositing)));
#endif
