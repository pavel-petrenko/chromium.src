# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import unittest
import websocket

class TestWebSocket(unittest.TestCase):
  def test_exports(self):
    self.assertNotEqual(websocket.create_connection, None)
