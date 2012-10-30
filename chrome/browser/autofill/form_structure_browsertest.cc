// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autofill/autofill_manager.h"
#include "chrome/browser/autofill/data_driven_test.h"
#include "chrome/browser/autofill/form_structure.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "googleurl/src/gurl.h"

namespace {

const FilePath::CharType kTestName[] = FILE_PATH_LITERAL("heuristics");

// Convert the |html| snippet to a data URI.
GURL HTMLToDataURI(const std::string& html) {
  return GURL(std::string("data:text/html;charset=utf-8,") + html);
}

}  // namespace

// A data-driven test for verifying Autofill heuristics. Each input is an HTML
// file that contains one or more forms. The corresponding output file lists the
// heuristically detected type for eachfield.
class FormStructureBrowserTest : public InProcessBrowserTest,
                                 public DataDrivenTest {
 protected:
  FormStructureBrowserTest();
  virtual ~FormStructureBrowserTest();

  // InProcessBrowserTest:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE;

  // DataDrivenTest:
  virtual void GenerateResults(const std::string& input,
                               std::string* output) OVERRIDE;

  // Serializes the given |forms| into a string.
  std::string FormStructuresToString(const std::vector<FormStructure*>& forms);

 private:
  DISALLOW_COPY_AND_ASSIGN(FormStructureBrowserTest);
};

FormStructureBrowserTest::FormStructureBrowserTest() {
}

FormStructureBrowserTest::~FormStructureBrowserTest() {
}

void FormStructureBrowserTest::SetUpCommandLine(CommandLine* command_line) {
  // Include new field types and heuristics in the regression test.
  command_line->AppendSwitch(switches::kEnableNewAutofillHeuristics);
}

void FormStructureBrowserTest::GenerateResults(const std::string& input,
                                               std::string* output) {
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
                                                       HTMLToDataURI(input)));

  AutofillManager* autofill_manager =
      AutofillManager::FromWebContents(chrome::GetActiveWebContents(browser()));
  ASSERT_NE(static_cast<AutofillManager*>(NULL), autofill_manager);
  std::vector<FormStructure*> forms = autofill_manager->form_structures_.get();
  *output = FormStructureBrowserTest::FormStructuresToString(forms);
}

std::string FormStructureBrowserTest::FormStructuresToString(
    const std::vector<FormStructure*>& forms) {
  std::string forms_string;
  for (std::vector<FormStructure*>::const_iterator iter = forms.begin();
       iter != forms.end();
       ++iter) {

    for (std::vector<AutofillField*>::const_iterator field_iter =
            (*iter)->begin();
         field_iter != (*iter)->end();
         ++field_iter) {
      forms_string += AutofillType::FieldTypeToString((*field_iter)->type());
      forms_string += " | " + UTF16ToUTF8((*field_iter)->name);
      forms_string += " | " + UTF16ToUTF8((*field_iter)->label);
      forms_string += " | " + UTF16ToUTF8((*field_iter)->value);
      forms_string += "\n";
    }
  }
  return forms_string;
}

// Heuristics tests timeout on Windows.  See http://crbug.com/85276
#if defined(OS_WIN)
#define MAYBE_DataDrivenHeuristics(n) DISABLED_DataDrivenHeuristics##n
#else
#define MAYBE_DataDrivenHeuristics(n) DataDrivenHeuristics##n
#endif
IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest, DataDrivenHeuristics00) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("00_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest, DataDrivenHeuristics01) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("01_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(02)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("02_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(03)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("03_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(04)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("04_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(05)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("05_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(06)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("06_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

// Has been failing on Mac since http://trac.webkit.org/changeset/105029
// (WebKit roll http://crrev.com/117847).
IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    DISABLED_DataDrivenHeuristics07) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("07_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(08)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("08_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(09)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("09_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(10)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("10_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(11)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("11_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(12)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("12_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(13)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("13_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(14)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("14_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(15)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("15_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(16)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("16_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(17)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("17_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}

IN_PROC_BROWSER_TEST_F(FormStructureBrowserTest,
    MAYBE_DataDrivenHeuristics(20)) {
  const FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("20_*.html");
  RunDataDrivenTest(GetInputDirectory(kTestName),
                    GetOutputDirectory(kTestName),
                    kFileNamePattern);
}
