// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/dom_distiller/content/browser/web_contents_main_frame_observer.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/content_browser_test.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Helper class to know how far in the loading process the current WebContents
// has come. It will call the callback after DocumentLoadedInFrame is called for
// the main frame.
class WebContentsMainFrameHelper : public content::WebContentsObserver {
 public:
  WebContentsMainFrameHelper(content::WebContents* web_contents,
                             const base::Closure& callback)
      : WebContentsObserver(web_contents), callback_(callback) {}

  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override {
    if (!render_frame_host->GetParent())
      callback_.Run();
  }

 private:
  base::Closure callback_;
};

}  // namespace

namespace dom_distiller {

const char* kExternalTestResourcesPath =
    "third_party/dom_distiller_js/dist/test/data";
// TODO(wychen) Remove filter when crbug.com/471854 is fixed.
const char* kTestFilePath =
    "/war/test.html?console_log=0&filter=-*.SchemaOrgParserAccessorTest.*";
const char* kRunJsTestsJs =
    "(function() {return org.chromium.distiller.JsTestEntry.run();})();";

class DomDistillerJsTest : public content::ContentBrowserTest {
 public:
  DomDistillerJsTest() : result_(NULL) {}

  // content::ContentBrowserTest:
  void SetUpOnMainThread() override {
    AddComponentsResources();
    SetUpTestServer();
    content::ContentBrowserTest::SetUpOnMainThread();
  }

  void OnJsTestExecutionDone(const base::Value* value) {
    result_ = value->DeepCopy();
    js_test_execution_done_callback_.Run();
  }

 protected:
  base::Closure js_test_execution_done_callback_;
  const base::Value* result_;

 private:
  void AddComponentsResources() {
    base::FilePath pak_file;
    base::FilePath pak_dir;
#if defined(OS_ANDROID)
    CHECK(PathService::Get(base::DIR_ANDROID_APP_DATA, &pak_dir));
    pak_dir = pak_dir.Append(FILE_PATH_LITERAL("paks"));
#else
    PathService::Get(base::DIR_MODULE, &pak_dir);
#endif  // OS_ANDROID
    pak_file =
        pak_dir.Append(FILE_PATH_LITERAL("components_tests_resources.pak"));
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        pak_file, ui::SCALE_FACTOR_NONE);
  }

  void SetUpTestServer() {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII(kExternalTestResourcesPath);
    embedded_test_server()->ServeFilesFromDirectory(path);
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  }
};

IN_PROC_BROWSER_TEST_F(DomDistillerJsTest, RunJsTests) {
  // Load the test file in content shell and wait until it has fully loaded.
  content::WebContents* web_contents = shell()->web_contents();
  dom_distiller::WebContentsMainFrameObserver::CreateForWebContents(
      web_contents);
  base::RunLoop url_loaded_runner;
  WebContentsMainFrameHelper main_frame_loaded(web_contents,
                                               url_loaded_runner.QuitClosure());
  web_contents->GetController().LoadURL(
      embedded_test_server()->GetURL(kTestFilePath),
      content::Referrer(),
      ui::PAGE_TRANSITION_TYPED,
      std::string());
  url_loaded_runner.Run();

  // Execute the JS to run the tests, and wait until it has finished.
  base::RunLoop run_loop;
  js_test_execution_done_callback_ = run_loop.QuitClosure();
  // Add timeout in case JS Test execution fails. It is safe to call the
  // QuitClosure multiple times.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::TimeDelta::FromSeconds(15));
  web_contents->GetMainFrame()->ExecuteJavaScriptForTests(
      base::UTF8ToUTF16(kRunJsTestsJs),
      base::Bind(&DomDistillerJsTest::OnJsTestExecutionDone,
                 base::Unretained(this)));
  run_loop.Run();

  // By now either the timeout has triggered, or there should be a result.
  ASSERT_TRUE(result_ != NULL) << "No result found. Timeout?";

  // Convert to dictionary and parse the results.
  const base::DictionaryValue* dict;
  result_->GetAsDictionary(&dict);
  ASSERT_TRUE(result_->GetAsDictionary(&dict));

  ASSERT_TRUE(dict->HasKey("success"));
  bool success;
  ASSERT_TRUE(dict->GetBoolean("success", &success));

  ASSERT_TRUE(dict->HasKey("numTests"));
  int num_tests;
  ASSERT_TRUE(dict->GetInteger("numTests", &num_tests));

  ASSERT_TRUE(dict->HasKey("failed"));
  int failed;
  ASSERT_TRUE(dict->GetInteger("failed", &failed));

  ASSERT_TRUE(dict->HasKey("skipped"));
  int skipped;
  ASSERT_TRUE(dict->GetInteger("skipped", &skipped));

  VLOG(0) << "Ran " << num_tests << " tests. failed = " << failed
          << " skipped = " << skipped;
  // Ensure that running the tests succeeded.
  EXPECT_TRUE(success);

  // Only print the log if there was an error.
  if (!success) {
    ASSERT_TRUE(dict->HasKey("log"));
    std::string console_log;
    ASSERT_TRUE(dict->GetString("log", &console_log));
    VLOG(0) << "Console log:\n" << console_log;
  }
}

}  // namespace dom_distiller
