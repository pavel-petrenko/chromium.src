#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs a single test case.

On success returns 0. On failure returns a negative value. See
websocket_handler.py for possible return values.

To use, run ./run_test.py test_case_name timeout_in_secs. Note, that all of the
Chrome windows will be killed.

The test runner runs a testing package of the Camera app within a new instance
of chrome, and starts a websocket server. The Camera app tries to connect to it
as soon as possible, and waits for the test case name to be run. After that,
messages are sent to the test runner. Possible messages are: SUCCESS, FAILURE,
INFO and COMMAND. The FAILURE one will cause tests fail. At least one SUCCESS
is required to pass the test. COMMAND messages are used to control the hardware,
eg. detach and reattach the USB camera device.

Note, that the Websocket implementation is trivial, and just a subset of the
protocol. However, it is enough for the purpose of this test runner.
"""


import argparse
import glob
import os
import signal
import subprocess
import sys
import threading
import websocket_handler

# Location of the Camera app.
SELF_PATH = os.path.dirname(os.path.abspath(__file__))
CAMERA_PATH = os.path.join(SELF_PATH, '../build/tests')

# Port to be used to communicate with the Camera app. Keep in sync with
# src/js/test.js.
WEBSOCKET_PORT = 47552


def RunCommand(command):
  """Runs a command, waits and returns the error code."""
  process = subprocess.Popen(command, shell=False)
  process.wait()
  return process.returncode


class TestCaseRunner(object):
  """Test case runner."""

  def __init__(self, test_case, test_timeout, chrome_binary, chrome_args):
    self.__server = None
    self.__server_thread = None
    self.__closing = False
    self.__status = websocket_handler.STATUS_INTERNAL_ERROR
    self.__chrome_process = None
    self.__test_case = test_case
    self.__test_timeout = test_timeout
    self.__chrome_binary = chrome_binary
    self.__chrome_args = chrome_args

  def Close(self, returncode):
    """Closes the test runner process.

    Closes the run_task.py process by stopping the WebSocket server,
    terminating all of the Chrome windows, and returning the passed error code.

    Args:
      returncode: int; the return code.
    """

    if self.__closing:
      return
    self.__closing = True

    self.__status = returncode
    self.__server.Terminate()
    self.__chrome_process.kill()

  def Run(self):
    """Runs the test case with the specified timeout passed as arguments.

    Returns:
      The error code.

    Raises:
      Exception if called more than once per instance.
    """

    if self.__server:
      raise Exception('Unable to call Run() more than once.')

    # Step 1. Restart the camera module.
    if RunCommand(['sudo', os.path.join(SELF_PATH, 'camera_reset.py')]) != 0:
      print 'Failed to reload the camera kernel module.'
      return websocket_handler.STATUS_INTERNAL_ERROR

    # Step 2. Fetch camera devices.
    camera_devices = [os.path.basename(usb_id)
                      for usb_id in
                      glob.glob('/sys/bus/usb/drivers/uvcvideo/?-*')]
    if not camera_devices:
      print 'No cameras detected.'
      return websocket_handler.STATUS_INTERNAL_ERROR

    # Step 3. Set the timeout.
    def Timeout():
      print 'Timeout.'
      return websocket_handler.STATUS_INTERNAL_ERROR

    signal.signal(signal.SIGALRM, Timeout)
    signal.alarm(self.__test_timeout)

    # Step 4. Check if there is a camera attached.
    if not glob.glob('/dev/video*'):
      print 'Camera device is missing.'
      return websocket_handler.STATUS_INTERNAL_ERROR

    # Step 5. Start the websocket server for communication with the Camera app.
    def HandleWebsocketCommand(name):
      if name == 'detach':
        for camera_device in camera_devices:
          if not os.path.exists('/sys/bus/usb/drivers/uvcvideo/%s' %
                                camera_device):
            continue
          if RunCommand(['sudo', os.path.join(SELF_PATH, 'camera_ctl.py'),
                         camera_device, 'unbind']) != 0:
            print 'Failed to detach a camera.'
            return websocket_handler.STATUS_INTERNAL_ERROR
      if name == 'attach':
        for camera_device in camera_devices:
          if os.path.exists('/sys/bus/usb/drivers/uvcvideo/%s' % camera_device):
            continue
          if RunCommand(['sudo', os.path.join(SELF_PATH, 'camera_ctl.py'),
                         camera_device, 'bind']) != 0:
            print 'Failed to attach the camera.'
            return websocket_handler.STATUS_INTERNAL_ERROR

    self.__server = websocket_handler.Server(
        ('localhost', WEBSOCKET_PORT), websocket_handler.Handler, self.Close,
        HandleWebsocketCommand, self.__test_case)
    server_thread = threading.Thread(target=self.__server.serve_forever)
    server_thread.daemon = True
    server_thread.start()

    # Step 6. Install the Camera app.
    run_command = [
        self.__chrome_binary, '--verbose', '--enable-logging',
        '--load-and-launch-app=%s' % CAMERA_PATH] + self.__chrome_args
    self.__chrome_process = subprocess.Popen(run_command, shell=False)
    self.__chrome_process.wait()

    # Wait until the browser is closed.
    return self.__status


def main():
  """Starts the application."""
  parser = argparse.ArgumentParser(description='Runs a single test.')
  parser.add_argument('--chrome',
                      help='Path for the Chrome binary.',
                      default='google-chrome')
  parser.add_argument('--timeout',
                      help='Timeout in seconds.',
                      type=int,
                      default=30)
  parser.add_argument('test_case',
                      help='Test case name to be run.', nargs=1)
  args, chrome_args = parser.parse_known_args()

  test_runner = TestCaseRunner(args.test_case[0],
                               args.timeout,
                               args.chrome,
                               chrome_args or [])
  sys.exit(test_runner.Run())

if __name__ == '__main__':
  main()

