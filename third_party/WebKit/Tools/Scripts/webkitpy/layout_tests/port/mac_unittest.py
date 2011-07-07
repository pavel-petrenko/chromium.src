# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import StringIO
import sys
import unittest

from webkitpy.layout_tests.port.mac import MacPort
from webkitpy.layout_tests.port import port_testcase
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.tool.mocktool import MockOptions, MockUser, MockExecutive


class MacTest(port_testcase.PortTestCase):
    def port_maker(self, platform):
        if platform != 'darwin':
            return None
        return MacPort

    def assert_skipped_file_search_paths(self, port_name, expected_paths):
        port = MacPort(port_name=port_name, filesystem=MockFileSystem(), user=MockUser(), executive=MockExecutive())
        self.assertEqual(port._skipped_file_search_paths(), expected_paths)

    def test_skipped_file_search_paths(self):
        # FIXME: This should no longer be necessary, but I'm not brave enough to remove it.
        if sys.platform == 'win32':
            return None

        self.assert_skipped_file_search_paths('mac-snowleopard', set(['mac-snowleopard', 'mac']))
        self.assert_skipped_file_search_paths('mac-leopard', set(['mac-leopard', 'mac']))
        # We cannot test just "mac" here as the MacPort constructor automatically fills in the version from the running OS.
        # self.assert_skipped_file_search_paths('mac', ['mac'])


    example_skipped_file = u"""
# <rdar://problem/5647952> fast/events/mouseout-on-window.html needs mac DRT to issue mouse out events
fast/events/mouseout-on-window.html

# <rdar://problem/5643675> window.scrollTo scrolls a window with no scrollbars
fast/events/attempt-scroll-with-no-scrollbars.html

# see bug <rdar://problem/5646437> REGRESSION (r28015): svg/batik/text/smallFonts fails
svg/batik/text/smallFonts.svg
"""
    example_skipped_tests = [
        "fast/events/mouseout-on-window.html",
        "fast/events/attempt-scroll-with-no-scrollbars.html",
        "svg/batik/text/smallFonts.svg",
    ]

    def test_tests_from_skipped_file_contents(self):
        port = MacPort(filesystem=MockFileSystem(), user=MockUser(), executive=MockExecutive())
        self.assertEqual(port._tests_from_skipped_file_contents(self.example_skipped_file), self.example_skipped_tests)

    def assert_name(self, port_name, os_version_string, expected):
        port = MacPort(port_name=port_name, os_version_string=os_version_string, filesystem=MockFileSystem(), user=MockUser(), executive=MockExecutive())
        self.assertEquals(expected, port.name())

    def test_tests_for_other_platforms(self):
        platforms = ['mac', 'chromium-linux', 'mac-snowleopard']
        port = MacPort(port_name='mac-snowleopard', filesystem=MockFileSystem(), user=MockUser(), executive=MockExecutive())
        platform_dir_paths = map(port._webkit_baseline_path, platforms)
        # Replace our empty mock file system with one which has our expected platform directories.
        port._filesystem = MockFileSystem(dirs=platform_dir_paths)

        dirs_to_skip = port._tests_for_other_platforms()
        self.assertTrue('platform/chromium-linux' in dirs_to_skip)
        self.assertFalse('platform/mac' in dirs_to_skip)
        self.assertFalse('platform/mac-snowleopard' in dirs_to_skip)

    def test_version(self):
        port = MacPort(filesystem=MockFileSystem(), user=MockUser(), executive=MockExecutive())
        self.assertTrue(port.version())

    def test_versions(self):
        port = self.make_port()
        if port:
            self.assertTrue(port.name() in ('mac-leopard', 'mac-snowleopard', 'mac-future'))

        self.assert_name(None, '10.5.3', 'mac-leopard')
        self.assert_name('mac', '10.5.3', 'mac-leopard')
        self.assert_name('mac-leopard', '10.4.8', 'mac-leopard')
        self.assert_name('mac-leopard', '10.5.3', 'mac-leopard')
        self.assert_name('mac-leopard', '10.6.3', 'mac-leopard')

        self.assert_name(None, '10.6.3', 'mac-snowleopard')
        self.assert_name('mac', '10.6.3', 'mac-snowleopard')
        self.assert_name('mac-snowleopard', '10.4.3', 'mac-snowleopard')
        self.assert_name('mac-snowleopard', '10.5.3', 'mac-snowleopard')
        self.assert_name('mac-snowleopard', '10.6.3', 'mac-snowleopard')

        self.assert_name(None, '10.7', 'mac-future')
        self.assert_name(None, '10.7.3', 'mac-future')
        self.assert_name(None, '10.8', 'mac-future')
        self.assert_name('mac', '10.7.3', 'mac-future')
        self.assert_name('mac-future', '10.4.3', 'mac-future')
        self.assert_name('mac-future', '10.5.3', 'mac-future')
        self.assert_name('mac-future', '10.6.3', 'mac-future')
        self.assert_name('mac-future', '10.7.3', 'mac-future')

        self.assertRaises(AssertionError, self.assert_name, None, '10.3.1', 'should-raise-assertion-so-this-value-does-not-matter')

    def _assert_search_path(self, search_paths, version, use_webkit2=False):
        # FIXME: Port constructors should not "parse" the port name, but
        # rather be passed components (directly or via setters).  Once
        # we fix that, this method will need a re-write.
        port = MacPort('mac-%s' % version, options=MockOptions(webkit_test_runner=use_webkit2), filesystem=MockFileSystem(), user=MockUser(), executive=MockExecutive())
        absolute_search_paths = map(port._webkit_baseline_path, search_paths)
        self.assertEquals(port.baseline_search_path(), absolute_search_paths)

    def test_baseline_search_path(self):
        # FIXME: Is this really right?  Should mac-leopard fallback to mac-snowleopard?
        self._assert_search_path(['mac-leopard', 'mac-snowleopard', 'mac'], 'leopard')
        self._assert_search_path(['mac-snowleopard', 'mac'], 'snowleopard')

        self._assert_search_path(['mac-wk2', 'mac-leopard', 'mac-snowleopard', 'mac'], 'leopard', use_webkit2=True)
        self._assert_search_path(['mac-wk2', 'mac-snowleopard', 'mac'], 'snowleopard', use_webkit2=True)


if __name__ == '__main__':
    port_testcase.main()
