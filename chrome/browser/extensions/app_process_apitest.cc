// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/blocked_content/popup_blocker_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/test_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/install_flag.h"
#include "extensions/browser/process_map.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "extensions/common/switches.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "sync/api/string_ordinal.h"

using content::NavigationController;
using content::RenderViewHost;
using content::SiteInstance;
using content::WebContents;
using extensions::Extension;

class AppApiTest : public ExtensionApiTest {
 protected:
  // Gets the base URL for files for a specific test, making sure that it uses
  // "localhost" as the hostname, since that is what the extent is declared
  // as in the test apps manifests.
  GURL GetTestBaseURL(const std::string& test_directory) {
    GURL::Replacements replace_host;
    replace_host.SetHostStr("localhost");
    GURL base_url = embedded_test_server()->GetURL(
        "/extensions/api_test/" + test_directory + "/");
    return base_url.ReplaceComponents(replace_host);
  }

  // Pass flags to make testing apps easier.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kDisablePopupBlocking);
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        extensions::switches::kAllowHTTPBackgroundPage);
  }

  // Helper function to test that independent tabs of the named app are loaded
  // into separate processes.
  void TestAppInstancesHelper(const std::string& app_name) {
    LOG(INFO) << "Start of test.";

    extensions::ProcessMap* process_map =
        extensions::ProcessMap::Get(browser()->profile());

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

    ASSERT_TRUE(LoadExtension(
        test_data_dir_.AppendASCII(app_name)));
    const Extension* extension = GetSingleLoadedExtension();

    // Open two tabs in the app, one outside it.
    GURL base_url = GetTestBaseURL(app_name);

    // Test both opening a URL in a new tab, and opening a tab and then
    // navigating it.  Either way, app tabs should be considered extension
    // processes, but they have no elevated privileges and thus should not
    // have WebUI bindings.
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), base_url.Resolve("path1/empty.html"), NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
    LOG(INFO) << "Nav 1.";
    EXPECT_TRUE(process_map->Contains(
        browser()->tab_strip_model()->GetWebContentsAt(1)->
            GetRenderProcessHost()->GetID()));
    EXPECT_FALSE(browser()->tab_strip_model()->GetWebContentsAt(1)->GetWebUI());

    content::WindowedNotificationObserver tab_added_observer(
        chrome::NOTIFICATION_TAB_ADDED,
        content::NotificationService::AllSources());
    chrome::NewTab(browser());
    tab_added_observer.Wait();
    LOG(INFO) << "New tab.";
    ui_test_utils::NavigateToURL(browser(),
                                 base_url.Resolve("path2/empty.html"));
    LOG(INFO) << "Nav 2.";
    EXPECT_TRUE(process_map->Contains(
        browser()->tab_strip_model()->GetWebContentsAt(2)->
            GetRenderProcessHost()->GetID()));
    EXPECT_FALSE(browser()->tab_strip_model()->GetWebContentsAt(2)->GetWebUI());

    // We should have opened 2 new extension tabs. Including the original blank
    // tab, we now have 3 tabs. The two app tabs should not be in the same
    // process, since they do not have the background permission.  (Thus, we
    // want to separate them to improve responsiveness.)
    ASSERT_EQ(3, browser()->tab_strip_model()->count());
    WebContents* tab1 = browser()->tab_strip_model()->GetWebContentsAt(1);
    WebContents* tab2 = browser()->tab_strip_model()->GetWebContentsAt(2);
    EXPECT_NE(tab1->GetRenderProcessHost(), tab2->GetRenderProcessHost());

    // Opening tabs with window.open should keep the page in the opener's
    // process.
    ASSERT_EQ(1u, chrome::GetBrowserCount(browser()->profile(),
                                          browser()->host_desktop_type()));
    OpenWindow(tab1, base_url.Resolve("path1/empty.html"), true, NULL);
    LOG(INFO) << "WindowOpenHelper 1.";
    OpenWindow(tab2, base_url.Resolve("path2/empty.html"), true, NULL);
    LOG(INFO) << "End of test.";
    UnloadExtension(extension->id());
  }
};

// Omits the disable-popup-blocking flag so we can cover that case.
class BlockedAppApiTest : public AppApiTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        extensions::switches::kAllowHTTPBackgroundPage);
  }
};

// Tests that hosted apps with the background permission get a process-per-app
// model, since all pages need to be able to script the background page.
// http://crbug.com/172750
IN_PROC_BROWSER_TEST_F(AppApiTest, DISABLED_AppProcess) {
  LOG(INFO) << "Start of test.";

  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("app_process")));

  LOG(INFO) << "Loaded extension.";

  // Open two tabs in the app, one outside it.
  GURL base_url = GetTestBaseURL("app_process");

  // Test both opening a URL in a new tab, and opening a tab and then navigating
  // it.  Either way, app tabs should be considered extension processes, but
  // they have no elevated privileges and thus should not have WebUI bindings.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), base_url.Resolve("path1/empty.html"), NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_TRUE(process_map->Contains(
      browser()->tab_strip_model()->GetWebContentsAt(1)->
          GetRenderProcessHost()->GetID()));
  EXPECT_FALSE(browser()->tab_strip_model()->GetWebContentsAt(1)->GetWebUI());
  LOG(INFO) << "Nav 1.";

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), base_url.Resolve("path2/empty.html"), NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_TRUE(process_map->Contains(
      browser()->tab_strip_model()->GetWebContentsAt(2)->
          GetRenderProcessHost()->GetID()));
  EXPECT_FALSE(browser()->tab_strip_model()->GetWebContentsAt(2)->GetWebUI());
  LOG(INFO) << "Nav 2.";

  content::WindowedNotificationObserver tab_added_observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::NotificationService::AllSources());
  chrome::NewTab(browser());
  tab_added_observer.Wait();
  LOG(INFO) << "New tab.";
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path3/empty.html"));
  LOG(INFO) << "Nav 3.";
  EXPECT_FALSE(process_map->Contains(
      browser()->tab_strip_model()->GetWebContentsAt(3)->
          GetRenderProcessHost()->GetID()));
  EXPECT_FALSE(browser()->tab_strip_model()->GetWebContentsAt(3)->GetWebUI());

  // We should have opened 3 new extension tabs. Including the original blank
  // tab, we now have 4 tabs. Because the app_process app has the background
  // permission, all of its instances are in the same process.  Thus two tabs
  // should be part of the extension app and grouped in the same process.
  ASSERT_EQ(4, browser()->tab_strip_model()->count());
  WebContents* tab = browser()->tab_strip_model()->GetWebContentsAt(1);

  EXPECT_EQ(tab->GetRenderProcessHost(),
            browser()->tab_strip_model()->GetWebContentsAt(2)->
                GetRenderProcessHost());
  EXPECT_NE(tab->GetRenderProcessHost(),
            browser()->tab_strip_model()->GetWebContentsAt(3)->
                GetRenderProcessHost());

  // Now let's do the same using window.open. The same should happen.
  ASSERT_EQ(1u, chrome::GetBrowserCount(browser()->profile(),
                                        browser()->host_desktop_type()));
  OpenWindow(tab, base_url.Resolve("path1/empty.html"), true, NULL);
  LOG(INFO) << "WindowOpenHelper 1.";
  OpenWindow(tab, base_url.Resolve("path2/empty.html"), true, NULL);
  LOG(INFO) << "WindowOpenHelper 2.";
  // TODO(creis): This should open in a new process (i.e., false for the last
  // argument), but we temporarily avoid swapping processes away from a hosted
  // app if it has an opener, because some OAuth providers make script calls
  // between non-app popups and non-app iframes in the app process.
  // See crbug.com/59285.
  OpenWindow(tab, base_url.Resolve("path3/empty.html"), true, NULL);
  LOG(INFO) << "WindowOpenHelper 3.";

  // Now let's have these pages navigate, into or out of the extension web
  // extent. They should switch processes.
  const GURL& app_url(base_url.Resolve("path1/empty.html"));
  const GURL& non_app_url(base_url.Resolve("path3/empty.html"));
  NavigateInRenderer(browser()->tab_strip_model()->GetWebContentsAt(2),
                     non_app_url);
  LOG(INFO) << "NavigateTabHelper 1.";
  NavigateInRenderer(browser()->tab_strip_model()->GetWebContentsAt(3),
                     app_url);
  LOG(INFO) << "NavigateTabHelper 2.";
  EXPECT_NE(tab->GetRenderProcessHost(),
            browser()->tab_strip_model()->GetWebContentsAt(2)->
                GetRenderProcessHost());
  EXPECT_EQ(tab->GetRenderProcessHost(),
            browser()->tab_strip_model()->GetWebContentsAt(3)->
                GetRenderProcessHost());

  // If one of the popup tabs navigates back to the app, window.opener should
  // be valid.
  NavigateInRenderer(browser()->tab_strip_model()->GetWebContentsAt(6),
                     app_url);
  LOG(INFO) << "NavigateTabHelper 3.";
  EXPECT_EQ(tab->GetRenderProcessHost(),
            browser()->tab_strip_model()->GetWebContentsAt(6)->
                GetRenderProcessHost());
  bool windowOpenerValid = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetWebContentsAt(6),
      "window.domAutomationController.send(window.opener != null)",
      &windowOpenerValid));
  ASSERT_TRUE(windowOpenerValid);

  LOG(INFO) << "End of test.";
}

// Test that hosted apps without the background permission use a process per app
// instance model, such that separate instances are in separate processes.
// Flaky on Windows. http://crbug.com/248047
#if defined(OS_WIN)
#define MAYBE_AppProcessInstances DISABLED_AppProcessInstances
#else
#define MAYBE_AppProcessInstances AppProcessInstances
#endif
IN_PROC_BROWSER_TEST_F(AppApiTest, MAYBE_AppProcessInstances) {
  TestAppInstancesHelper("app_process_instances");
}

// Test that hosted apps with the background permission but that set
// allow_js_access to false also use a process per app instance model.
// Separate instances should be in separate processes.
// Flaky on XP: http://crbug.com/165834
#if defined(OS_WIN)
#define MAYBE_AppProcessBackgroundInstances \
    DISABLED_AppProcessBackgroundInstances
#else
#define MAYBE_AppProcessBackgroundInstances AppProcessBackgroundInstances
#endif
IN_PROC_BROWSER_TEST_F(AppApiTest, MAYBE_AppProcessBackgroundInstances) {
  TestAppInstancesHelper("app_process_background_instances");
}

// Tests that bookmark apps do not use the app process model and are treated
// like normal web pages instead.  http://crbug.com/104636.
// Timing out on Windows. http://crbug.com/238777
#if defined(OS_WIN)
#define MAYBE_BookmarkAppGetsNormalProcess DISABLED_BookmarkAppGetsNormalProcess
#else
#define MAYBE_BookmarkAppGetsNormalProcess BookmarkAppGetsNormalProcess
#endif
IN_PROC_BROWSER_TEST_F(AppApiTest, MAYBE_BookmarkAppGetsNormalProcess) {
  ExtensionService* service = extensions::ExtensionSystem::Get(
      browser()->profile())->extension_service();
  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  GURL base_url = GetTestBaseURL("app_process");

  // Load an app as a bookmark app.
  std::string error;
  scoped_refptr<const Extension> extension(extensions::file_util::LoadExtension(
      test_data_dir_.AppendASCII("app_process"),
      extensions::Manifest::UNPACKED,
      Extension::FROM_BOOKMARK,
      &error));
  service->OnExtensionInstalled(extension.get(),
                                syncer::StringOrdinal::CreateInitialOrdinal(),
                                extensions::kInstallFlagInstallImmediately);
  ASSERT_TRUE(extension.get());
  ASSERT_TRUE(extension->from_bookmark());

  // Test both opening a URL in a new tab, and opening a tab and then navigating
  // it.  Either way, bookmark app tabs should be considered normal processes
  // with no elevated privileges and no WebUI bindings.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), base_url.Resolve("path1/empty.html"), NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_FALSE(process_map->Contains(
      browser()->tab_strip_model()->GetWebContentsAt(1)->
          GetRenderProcessHost()->GetID()));
  EXPECT_FALSE(browser()->tab_strip_model()->GetWebContentsAt(1)->GetWebUI());

  content::WindowedNotificationObserver tab_added_observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::NotificationService::AllSources());
  chrome::NewTab(browser());
  tab_added_observer.Wait();
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path2/empty.html"));
  EXPECT_FALSE(process_map->Contains(
      browser()->tab_strip_model()->GetWebContentsAt(2)->
          GetRenderProcessHost()->GetID()));
  EXPECT_FALSE(browser()->tab_strip_model()->GetWebContentsAt(2)->GetWebUI());

  // We should have opened 2 new bookmark app tabs. Including the original blank
  // tab, we now have 3 tabs.  Because normal pages use the
  // process-per-site-instance model, each should be in its own process.
  ASSERT_EQ(3, browser()->tab_strip_model()->count());
  WebContents* tab = browser()->tab_strip_model()->GetWebContentsAt(1);
  EXPECT_NE(tab->GetRenderProcessHost(),
            browser()->tab_strip_model()->GetWebContentsAt(2)->
                GetRenderProcessHost());

  // Now let's do the same using window.open. The same should happen.
  ASSERT_EQ(1u, chrome::GetBrowserCount(browser()->profile(),
                                        browser()->host_desktop_type()));
  OpenWindow(tab, base_url.Resolve("path1/empty.html"), true, NULL);
  OpenWindow(tab, base_url.Resolve("path2/empty.html"), true, NULL);

  // Now let's have a tab navigate out of and back into the app's web
  // extent. Neither navigation should switch processes.
  const GURL& app_url(base_url.Resolve("path1/empty.html"));
  const GURL& non_app_url(base_url.Resolve("path3/empty.html"));
  RenderViewHost* host2 =
      browser()->tab_strip_model()->GetWebContentsAt(2)->GetRenderViewHost();
  NavigateInRenderer(browser()->tab_strip_model()->GetWebContentsAt(2),
                     non_app_url);
  EXPECT_EQ(host2->GetProcess(),
            browser()->tab_strip_model()->GetWebContentsAt(2)->
                GetRenderProcessHost());
  NavigateInRenderer(browser()->tab_strip_model()->GetWebContentsAt(2),
                     app_url);
  EXPECT_EQ(host2->GetProcess(),
            browser()->tab_strip_model()->GetWebContentsAt(2)->
                GetRenderProcessHost());
}

// Tests that app process switching works properly in the following scenario:
// 1. navigate to a page1 in the app
// 2. page1 redirects to a page2 outside the app extent (ie, "/server-redirect")
// 3. page2 redirects back to a page in the app
// The final navigation should end up in the app process.
// See http://crbug.com/61757
// Flaky.  http://crbug.com/341898
IN_PROC_BROWSER_TEST_F(AppApiTest, DISABLED_AppProcessRedirectBack) {
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("app_process")));

  // Open two tabs in the app.
  GURL base_url = GetTestBaseURL("app_process");

  chrome::NewTab(browser());
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path1/empty.html"));
  chrome::NewTab(browser());
  // Wait until the second tab finishes its redirect train (2 hops).
  // 1. We navigate to redirect.html
  // 2. Renderer navigates and finishes, counting as a load stop.
  // 3. Renderer issues the meta refresh to navigate to server-redirect.
  // 4. Renderer is now in a "provisional load", waiting for navigation to
  //    complete.
  // 5. Browser sees a redirect response from server-redirect to empty.html, and
  //    transfers that to a new navigation, using RequestTransferURL.
  // 6. Renderer navigates to empty.html, and finishes loading, counting as the
  //    second load stop
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
      browser(), base_url.Resolve("path1/redirect.html"), 2);

  // 3 tabs, including the initial about:blank. The last 2 should be the same
  // process.
  ASSERT_EQ(3, browser()->tab_strip_model()->count());
  EXPECT_EQ("/extensions/api_test/app_process/path1/empty.html",
            browser()->tab_strip_model()->GetWebContentsAt(2)->
                GetController().GetLastCommittedEntry()->GetURL().path());
  EXPECT_EQ(browser()->tab_strip_model()->GetWebContentsAt(1)->
                GetRenderProcessHost(),
            browser()->tab_strip_model()->GetWebContentsAt(2)->
                GetRenderProcessHost());
}

// Ensure that re-navigating to a URL after installing or uninstalling it as an
// app correctly swaps the tab to the app process.  (http://crbug.com/80621)
//
// Fails on Windows. http://crbug.com/238670
// Added logging to help diagnose the location of the problem.
IN_PROC_BROWSER_TEST_F(AppApiTest, NavigateIntoAppProcess) {
  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // The app under test acts on URLs whose host is "localhost",
  // so the URLs we navigate to must have host "localhost".
  GURL base_url = GetTestBaseURL("app_process");

  // Load an app URL before loading the app.
  LOG(INFO) << "Loading path1/empty.html.";
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path1/empty.html"));
  LOG(INFO) << "Loading path1/empty.html - done.";
  WebContents* contents = browser()->tab_strip_model()->GetWebContentsAt(0);
  EXPECT_FALSE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));

  // Load app and re-navigate to the page.
  LOG(INFO) << "Loading extension.";
  const Extension* app =
      LoadExtension(test_data_dir_.AppendASCII("app_process"));
  LOG(INFO) << "Loading extension - done.";
  ASSERT_TRUE(app);
  LOG(INFO) << "Loading path1/empty.html.";
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path1/empty.html"));
  LOG(INFO) << "Loading path1/empty.html - done.";
  EXPECT_TRUE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));

  // Disable app and re-navigate to the page.
  LOG(INFO) << "Disabling extension.";
  DisableExtension(app->id());
  LOG(INFO) << "Disabling extension - done.";
  LOG(INFO) << "Loading path1/empty.html.";
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path1/empty.html"));
  LOG(INFO) << "Loading path1/empty.html - done.";
  EXPECT_FALSE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));
}

// Ensure that reloading a URL after installing or uninstalling it as an app
// correctly swaps the tab to the app process.  (http://crbug.com/80621)
//
// Added logging to help diagnose the location of the problem.
// http://crbug.com/238670
IN_PROC_BROWSER_TEST_F(AppApiTest, ReloadIntoAppProcess) {
  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // The app under test acts on URLs whose host is "localhost",
  // so the URLs we navigate to must have host "localhost".
  GURL base_url = GetTestBaseURL("app_process");

  // Load app, disable it, and navigate to the page.
  LOG(INFO) << "Loading extension.";
  const Extension* app =
      LoadExtension(test_data_dir_.AppendASCII("app_process"));
  LOG(INFO) << "Loading extension - done.";
  ASSERT_TRUE(app);
  LOG(INFO) << "Disabling extension.";
  DisableExtension(app->id());
  LOG(INFO) << "Disabling extension - done.";
  LOG(INFO) << "Navigate to path1/empty.html.";
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path1/empty.html"));
  LOG(INFO) << "Navigate to path1/empty.html - done.";
  WebContents* contents = browser()->tab_strip_model()->GetWebContentsAt(0);
  EXPECT_FALSE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));

  // Enable app and reload the page.
  LOG(INFO) << "Enabling extension.";
  EnableExtension(app->id());
  LOG(INFO) << "Enabling extension - done.";
  content::WindowedNotificationObserver reload_observer(
      content::NOTIFICATION_LOAD_STOP,
      content::Source<NavigationController>(
          &browser()->tab_strip_model()->GetActiveWebContents()->
              GetController()));
  LOG(INFO) << "Reloading.";
  chrome::Reload(browser(), CURRENT_TAB);
  reload_observer.Wait();
  LOG(INFO) << "Reloading - done.";
  EXPECT_TRUE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));

  // Disable app and reload the page.
  LOG(INFO) << "Disabling extension.";
  DisableExtension(app->id());
  LOG(INFO) << "Disabling extension - done.";
  content::WindowedNotificationObserver reload_observer2(
      content::NOTIFICATION_LOAD_STOP,
      content::Source<NavigationController>(
          &browser()->tab_strip_model()->GetActiveWebContents()->
              GetController()));
  LOG(INFO) << "Reloading.";
  chrome::Reload(browser(), CURRENT_TAB);
  reload_observer2.Wait();
  LOG(INFO) << "Reloading - done.";
  EXPECT_FALSE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));
}

// Ensure that reloading a URL with JavaScript after installing or uninstalling
// it as an app correctly swaps the process.  (http://crbug.com/80621)
//
// Crashes on Windows and Mac. http://crbug.com/238670
// Added logging to help diagnose the location of the problem.
IN_PROC_BROWSER_TEST_F(AppApiTest, ReloadIntoAppProcessWithJavaScript) {
  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // The app under test acts on URLs whose host is "localhost",
  // so the URLs we navigate to must have host "localhost".
  GURL base_url = GetTestBaseURL("app_process");

  // Load app, disable it, and navigate to the page.
  LOG(INFO) << "Loading extension.";
  const Extension* app =
      LoadExtension(test_data_dir_.AppendASCII("app_process"));
  LOG(INFO) << "Loading extension - done.";
  ASSERT_TRUE(app);
  LOG(INFO) << "Disabling extension.";
  DisableExtension(app->id());
  LOG(INFO) << "Disabling extension - done.";
  LOG(INFO) << "Navigate to path1/empty.html.";
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path1/empty.html"));
  LOG(INFO) << "Navigate to path1/empty.html - done.";
  WebContents* contents = browser()->tab_strip_model()->GetWebContentsAt(0);
  EXPECT_FALSE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));

  // Enable app and reload via JavaScript.
  LOG(INFO) << "Enabling extension.";
  EnableExtension(app->id());
  LOG(INFO) << "Enabling extension - done.";
  content::WindowedNotificationObserver js_reload_observer(
      content::NOTIFICATION_LOAD_STOP,
      content::Source<NavigationController>(
          &browser()->tab_strip_model()->GetActiveWebContents()->
              GetController()));
  LOG(INFO) << "Executing location.reload().";
  ASSERT_TRUE(content::ExecuteScript(contents, "location.reload();"));
  js_reload_observer.Wait();
  LOG(INFO) << "Executing location.reload() - done.";
  EXPECT_TRUE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));

  // Disable app and reload via JavaScript.
  LOG(INFO) << "Disabling extension.";
  DisableExtension(app->id());
  LOG(INFO) << "Disabling extension - done.";
  content::WindowedNotificationObserver js_reload_observer2(
      content::NOTIFICATION_LOAD_STOP,
      content::Source<NavigationController>(
          &browser()->tab_strip_model()->GetActiveWebContents()->
              GetController()));
  LOG(INFO) << "Executing location = location.";
  ASSERT_TRUE(content::ExecuteScript(contents, "location = location;"));
  js_reload_observer2.Wait();
  LOG(INFO) << "Executing location = location - done.";
  EXPECT_FALSE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));
}

// Tests that if we have a non-app process (path3/container.html) that has an
// iframe with  a URL in the app's extent (path1/iframe.html), then opening a
// link from that iframe to a new window to a URL in the app's extent (path1/
// empty.html) results in the new window being in an app process. See
// http://crbug.com/89272 for more details.
IN_PROC_BROWSER_TEST_F(AppApiTest, OpenAppFromIframe) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL base_url = GetTestBaseURL("app_process");

  // Load app and start URL (not in the app).
  const Extension* app =
      LoadExtension(test_data_dir_.AppendASCII("app_process"));
  ASSERT_TRUE(app);

  ui_test_utils::NavigateToURL(browser(),
                               base_url.Resolve("path3/container.html"));
  EXPECT_FALSE(process_map->Contains(
      browser()->tab_strip_model()->GetWebContentsAt(0)->
          GetRenderProcessHost()->GetID()));

  const BrowserList* active_browser_list =
      BrowserList::GetInstance(chrome::GetActiveDesktop());
  EXPECT_EQ(2U, active_browser_list->size());
  content::WebContents* popup_contents =
      active_browser_list->get(1)->tab_strip_model()->GetActiveWebContents();
  content::WaitForLoadStop(popup_contents);

  // Popup window should be in the app's process.
  RenderViewHost* popup_host = popup_contents->GetRenderViewHost();
  EXPECT_TRUE(process_map->Contains(popup_host->GetProcess()->GetID()));
}

// Similar to the previous test, but ensure that popup blocking bypass
// isn't granted to the iframe.  See crbug.com/117446.
#if defined(OS_CHROMEOS)
// http://crbug.com/153513
#define MAYBE_OpenAppFromIframe DISABLED_OpenAppFromIframe
#else
#define MAYBE_OpenAppFromIframe OpenAppFromIframe
#endif
IN_PROC_BROWSER_TEST_F(BlockedAppApiTest, MAYBE_OpenAppFromIframe) {
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Load app and start URL (not in the app).
  const Extension* app =
      LoadExtension(test_data_dir_.AppendASCII("app_process"));
  ASSERT_TRUE(app);

  ui_test_utils::NavigateToURL(
      browser(), GetTestBaseURL("app_process").Resolve("path3/container.html"));

  WebContents* tab = browser()->tab_strip_model()->GetActiveWebContents();
  PopupBlockerTabHelper* popup_blocker_tab_helper =
      PopupBlockerTabHelper::FromWebContents(tab);
  if (!popup_blocker_tab_helper->GetBlockedPopupsCount()) {
    content::WindowedNotificationObserver observer(
        chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED,
        content::NotificationService::AllSources());
    observer.Wait();
  }

  EXPECT_EQ(1u, popup_blocker_tab_helper->GetBlockedPopupsCount());
}

// Tests that if an extension launches an app via chrome.tabs.create with an URL
// that's not in the app's extent but that server redirects to it, we still end
// up with an app process. See http://crbug.com/99349 for more details.
IN_PROC_BROWSER_TEST_F(AppApiTest, ServerRedirectToAppFromExtension) {
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(StartEmbeddedTestServer());

  LoadExtension(test_data_dir_.AppendASCII("app_process"));
  const Extension* launcher =
      LoadExtension(test_data_dir_.AppendASCII("app_launcher"));

  // There should be two navigations by the time the app page is loaded.
  // 1. The extension launcher page.
  // 2. The app's URL (which includes a server redirect).
  // Note that the server redirect does not generate a navigation event.
  content::TestNavigationObserver test_navigation_observer(
      browser()->tab_strip_model()->GetActiveWebContents(),
      2);
  test_navigation_observer.StartWatchingNewWebContents();

  // Load the launcher extension, which should launch the app.
  ui_test_utils::NavigateToURL(
      browser(), launcher->GetResourceURL("server_redirect.html"));

  // Wait for app tab to be created and loaded.
  test_navigation_observer.Wait();

  // App has loaded, and chrome.app.isInstalled should be true.
  bool is_installed = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "window.domAutomationController.send(chrome.app.isInstalled)",
      &is_installed));
  ASSERT_TRUE(is_installed);
}

// Tests that if an extension launches an app via chrome.tabs.create with an URL
// that's not in the app's extent but that client redirects to it, we still end
// up with an app process.
IN_PROC_BROWSER_TEST_F(AppApiTest, ClientRedirectToAppFromExtension) {
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(StartEmbeddedTestServer());

  LoadExtension(test_data_dir_.AppendASCII("app_process"));
  const Extension* launcher =
      LoadExtension(test_data_dir_.AppendASCII("app_launcher"));

  // There should be three navigations by the time the app page is loaded.
  // 1. The extension launcher page.
  // 2. The URL that the extension launches, which client redirects.
  // 3. The app's URL.
  content::TestNavigationObserver test_navigation_observer(
      browser()->tab_strip_model()->GetActiveWebContents(),
      3);
  test_navigation_observer.StartWatchingNewWebContents();

  // Load the launcher extension, which should launch the app.
  ui_test_utils::NavigateToURL(
      browser(), launcher->GetResourceURL("client_redirect.html"));

  // Wait for app tab to be created and loaded.
  test_navigation_observer.Wait();

  // App has loaded, and chrome.app.isInstalled should be true.
  bool is_installed = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "window.domAutomationController.send(chrome.app.isInstalled)",
      &is_installed));
  ASSERT_TRUE(is_installed);
}

// Tests that if we have an app process (path1/container.html) with a non-app
// iframe (path3/iframe.html), then opening a link from that iframe to a new
// window to a same-origin non-app URL (path3/empty.html) should keep the window
// in the app process.
// This is in contrast to OpenAppFromIframe, since here the popup will not be
// missing special permissions and should be scriptable from the iframe.
// See http://crbug.com/92669 for more details.
IN_PROC_BROWSER_TEST_F(AppApiTest, OpenWebPopupFromWebIframe) {
  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL base_url = GetTestBaseURL("app_process");

  // Load app and start URL (in the app).
  const Extension* app =
      LoadExtension(test_data_dir_.AppendASCII("app_process"));
  ASSERT_TRUE(app);

  ui_test_utils::NavigateToURL(browser(),
                               base_url.Resolve("path1/container.html"));
  content::RenderProcessHost* process =
      browser()->tab_strip_model()->GetWebContentsAt(0)->GetRenderProcessHost();
  EXPECT_TRUE(process_map->Contains(process->GetID()));

  // Popup window should be in the app's process if site isolation is off;
  // otherwise they should be in different processes.
  const BrowserList* active_browser_list =
      BrowserList::GetInstance(chrome::GetActiveDesktop());
  EXPECT_EQ(2U, active_browser_list->size());
  content::WebContents* popup_contents =
      active_browser_list->get(1)->tab_strip_model()->GetActiveWebContents();
  content::WaitForLoadStop(popup_contents);

  bool should_be_in_same_process = !content::AreAllSitesIsolatedForTesting();
  content::RenderProcessHost* popup_process =
      popup_contents->GetRenderProcessHost();
  EXPECT_EQ(should_be_in_same_process, process == popup_process);
  EXPECT_EQ(should_be_in_same_process,
            process_map->Contains(popup_process->GetID()));
}

// http://crbug.com/118502
#if defined(OS_MACOSX) || defined(OS_LINUX)
#define MAYBE_ReloadAppAfterCrash DISABLED_ReloadAppAfterCrash
#else
#define MAYBE_ReloadAppAfterCrash ReloadAppAfterCrash
#endif
IN_PROC_BROWSER_TEST_F(AppApiTest, MAYBE_ReloadAppAfterCrash) {
  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("app_process")));

  GURL base_url = GetTestBaseURL("app_process");

  // Load the app, chrome.app.isInstalled should be true.
  ui_test_utils::NavigateToURL(browser(), base_url.Resolve("path1/empty.html"));
  WebContents* contents = browser()->tab_strip_model()->GetWebContentsAt(0);
  EXPECT_TRUE(process_map->Contains(
      contents->GetRenderProcessHost()->GetID()));
  bool is_installed = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(chrome.app.isInstalled)",
      &is_installed));
  ASSERT_TRUE(is_installed);

  // Crash the tab and reload it, chrome.app.isInstalled should still be true.
  content::CrashTab(browser()->tab_strip_model()->GetActiveWebContents());
  content::WindowedNotificationObserver observer(
      content::NOTIFICATION_LOAD_STOP,
      content::Source<NavigationController>(
          &browser()->tab_strip_model()->GetActiveWebContents()->
              GetController()));
  chrome::Reload(browser(), CURRENT_TAB);
  observer.Wait();
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(chrome.app.isInstalled)",
      &is_installed));
  ASSERT_TRUE(is_installed);
}

// Test that a cross-process navigation away from a hosted app stays in the same
// BrowsingInstance, so that postMessage calls to the app's other windows still
// work.
IN_PROC_BROWSER_TEST_F(AppApiTest, SameBrowsingInstanceAfterSwap) {
  extensions::ProcessMap* process_map =
      extensions::ProcessMap::Get(browser()->profile());

  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL base_url = GetTestBaseURL("app_process");

  // Load app and start URL (in the app).
  const Extension* app =
      LoadExtension(test_data_dir_.AppendASCII("app_process"));
  ASSERT_TRUE(app);

  ui_test_utils::NavigateToURL(browser(),
                               base_url.Resolve("path1/iframe.html"));
  content::SiteInstance* app_instance =
      browser()->tab_strip_model()->GetWebContentsAt(0)->GetSiteInstance();
  EXPECT_TRUE(process_map->Contains(app_instance->GetProcess()->GetID()));

  // Popup window should be in the app's process.
  const BrowserList* active_browser_list =
      BrowserList::GetInstance(chrome::GetActiveDesktop());
  EXPECT_EQ(2U, active_browser_list->size());
  content::WebContents* popup_contents =
      active_browser_list->get(1)->tab_strip_model()->GetActiveWebContents();
  content::WaitForLoadStop(popup_contents);

  SiteInstance* popup_instance = popup_contents->GetSiteInstance();
  EXPECT_EQ(app_instance, popup_instance);

  // Navigate the popup to another process outside the app.
  GURL non_app_url(base_url.Resolve("path3/empty.html"));
  ui_test_utils::NavigateToURL(active_browser_list->get(1), non_app_url);
  SiteInstance* new_instance = popup_contents->GetSiteInstance();
  EXPECT_NE(app_instance, new_instance);

  // It should still be in the same BrowsingInstance, allowing postMessage to
  // work.
  EXPECT_TRUE(app_instance->IsRelatedSiteInstance(new_instance));
}
