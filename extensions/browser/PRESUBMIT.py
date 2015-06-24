# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Chromium presubmit script for src/extensions/browser.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into depot_tools.
"""

import sys

def _CreateHistogramValueChecker(input_api, output_api, path):
  original_sys_path = sys.path

  try:
    sys.path.append(input_api.os_path.join(
        input_api.PresubmitLocalPath(), '..', '..', 'tools',
        'strict_enum_value_checker'))
    from strict_enum_value_checker import StrictEnumValueChecker
  finally:
    sys.path = original_sys_path

  return StrictEnumValueChecker(input_api, output_api,
      start_marker='enum HistogramValue {', end_marker='  // Last entry:',
      path=path)


def _RunHistogramValueCheckers(input_api, output_api):
  results = []
  histogram_paths = ('extensions/browser/extension_event_histogram_value.h',
                     'extensions/browser/extension_function_histogram_value.h')
  for path in histogram_paths:
    results += _CreateHistogramValueChecker(input_api, output_api, path).Run()
  return results


def CheckChangeOnUpload(input_api, output_api):
  results = []
  # results += _RunHistogramValueCheckers(input_api, output_api)
  results += input_api.canned_checks.CheckPatchFormatted(input_api, output_api)
  return results


def CheckChangeOnCommit(input_api, output_api):
  return []  # _RunHistogramValueCheckers(input_api, output_api)
