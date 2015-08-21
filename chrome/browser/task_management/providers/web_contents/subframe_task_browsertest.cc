// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/task_management/task_management_browsertest_util.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"

namespace task_management {

namespace {

// URL of a test page on a.com that has two cross-site iframes to b.com and
// c.com.
const char kCrossSitePageUrl[] = "/cross-site/a.com/iframe_cross_site.html";

// URL of a test page on a.com that has no cross-site iframes.
const char kSimplePageUrl[] = "/cross-site/a.com/title2.html";

base::string16 GetExpectedSubframeTitlePrefix() {
  return l10n_util::GetStringFUTF16(IDS_TASK_MANAGER_SUBFRAME_PREFIX,
                                    base::string16());
}

base::string16 PrefixExpectedTabTitle(const char* title) {
  return l10n_util::GetStringFUTF16(IDS_TASK_MANAGER_TAB_PREFIX,
                                    base::UTF8ToUTF16(title));
}

}  // namespace

// A test for OOPIFs and how they show up in the task manager as
// SubframeTasks.
class SubframeTaskBrowserTest : public InProcessBrowserTest {
 public:
  SubframeTaskBrowserTest() {}
  ~SubframeTaskBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    content::IsolateAllSitesForTesting(command_line);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
    content::SetupCrossSiteRedirector(embedded_test_server());
  }

  void NavigateTo(const char* page_url) const {
    ui_test_utils::NavigateToURL(browser(),
                                 embedded_test_server()->GetURL(page_url));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SubframeTaskBrowserTest);
};

// Makes sure that, if sites are isolated, the task manager will show the
// expected SubframeTasks, and they will be shown as running on different
// processes as expected.
IN_PROC_BROWSER_TEST_F(SubframeTaskBrowserTest, TaskManagerShowsSubframeTasks) {
  MockWebContentsTaskManager task_manager;
  EXPECT_TRUE(task_manager.tasks().empty());
  task_manager.StartObserving();

  // Currently only the about:blank page.
  ASSERT_EQ(1U, task_manager.tasks().size());
  const Task* about_blank_task = task_manager.tasks().front();
  EXPECT_EQ(Task::RENDERER, about_blank_task->GetType());
  EXPECT_EQ(PrefixExpectedTabTitle("about:blank"), about_blank_task->title());

  NavigateTo(kCrossSitePageUrl);

  // Whether sites are isolated or not, we expect to have at least one tab
  // contents task.
  ASSERT_GE(task_manager.tasks().size(), 1U);
  const Task* cross_site_task = task_manager.tasks().front();
  EXPECT_EQ(Task::RENDERER, cross_site_task->GetType());
  EXPECT_EQ(PrefixExpectedTabTitle("cross-site iframe test"),
            cross_site_task->title());

  if (!content::AreAllSitesIsolatedForTesting()) {
    // Sites are not isolated. No SubframeTasks are expected, just the above
    // task.
    ASSERT_EQ(1U, task_manager.tasks().size());
  } else {
    // Sites are isolated. We expect, in addition to the above task, two more
    // SubframeTasks, one for b.com and another for c.com.
    ASSERT_EQ(3U, task_manager.tasks().size());
    const Task* subframe_task_1 = task_manager.tasks()[1];
    const Task* subframe_task_2 = task_manager.tasks()[2];

    EXPECT_EQ(Task::RENDERER, subframe_task_1->GetType());
    EXPECT_EQ(Task::RENDERER, subframe_task_2->GetType());

    EXPECT_TRUE(base::StartsWith(subframe_task_1->title(),
                                 GetExpectedSubframeTitlePrefix(),
                                 base::CompareCase::INSENSITIVE_ASCII));
    EXPECT_TRUE(base::StartsWith(subframe_task_2->title(),
                                 GetExpectedSubframeTitlePrefix(),
                                 base::CompareCase::INSENSITIVE_ASCII));

    // All tasks must be running on different processes.
    EXPECT_NE(subframe_task_1->process_id(), subframe_task_2->process_id());
    EXPECT_NE(subframe_task_1->process_id(), cross_site_task->process_id());
    EXPECT_NE(subframe_task_2->process_id(), cross_site_task->process_id());
  }

  // If we navigate to the simple page on a.com which doesn't have cross-site
  // iframes, we expect not to have any SubframeTasks.
  NavigateTo(kSimplePageUrl);

  ASSERT_EQ(1U, task_manager.tasks().size());
  const Task* simple_page_task = task_manager.tasks().front();
  EXPECT_EQ(Task::RENDERER, simple_page_task->GetType());
  EXPECT_EQ(PrefixExpectedTabTitle("Title Of Awesomeness"),
            simple_page_task->title());
}

}  // namespace task_management
