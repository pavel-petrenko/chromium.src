# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      # GN version: //extensions/shell/browser/api
      'target_name': 'shell_api_registration',
      'type': 'static_library',
      # TODO(jschuh): http://crbug.com/167187 size_t -> int
      'msvs_disabled_warnings': [ 4267 ],
      'includes': [
        '../../../../build/json_schema_bundle_registration_compile.gypi',
        '../../common/api/schemas.gypi',
      ],
      'dependencies': [
        '<(DEPTH)/extensions/shell/common/api/api.gyp:shell_api',
      ],
    },
  ],
}
