# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs perf tests.

Our buildbot infrastructure requires each slave to run steps serially.
This is sub-optimal for android, where these steps can run independently on
multiple connected devices.

The buildbots will run this script multiple times per cycle:
- First: all steps listed in --steps in will be executed in parallel using all
connected devices. Step results will be pickled to disk. Each step has a unique
name. The result code will be ignored if the step name is listed in
--flaky-steps.
The buildbot will treat this step as a regular step, and will not process any
graph data.

- Then, with -print-step STEP_NAME: at this stage, we'll simply print the file
with the step results previously saved. The buildbot will then process the graph
data accordingly.

The JSON steps file contains a dictionary in the format:
{ "version": int,
  "steps": {
    "foo": {
      "device_affinity": int,
      "cmd": "script_to_execute foo"
    },
    "bar": {
      "device_affinity": int,
      "cmd": "script_to_execute bar"
    }
  }
}

The JSON flaky steps file contains a list with step names which results should
be ignored:
[
  "step_name_foo",
  "step_name_bar"
]

Note that script_to_execute necessarily have to take at least the following
option:
  --device: the serial number to be passed to all adb commands.
"""

import collections
import json
import logging
import os
import pickle
import shutil
import sys
import tempfile
import threading
import time

from devil.android import battery_utils
from devil.android import device_errors
from devil.utils import cmd_helper
from pylib import constants
from pylib import forwarder
from pylib.base import base_test_result
from pylib.base import base_test_runner


def GetPersistedResult(test_name):
  file_name = os.path.join(constants.PERF_OUTPUT_DIR, test_name)
  if not os.path.exists(file_name):
    logging.error('File not found %s', file_name)
    return None

  with file(file_name, 'r') as f:
    return pickle.loads(f.read())


def OutputJsonList(json_input, json_output):
  with file(json_input, 'r') as i:
    all_steps = json.load(i)

  step_values = []
  for k, v in all_steps['steps'].iteritems():
    data = {'test': k, 'device_affinity': v['device_affinity']}

    persisted_result = GetPersistedResult(k)
    if persisted_result:
      data['start_time'] = persisted_result['start_time']
      data['end_time'] = persisted_result['end_time']
      data['total_time'] = persisted_result['total_time']
    step_values.append(data)

  with file(json_output, 'w') as o:
    o.write(json.dumps(step_values))
  return 0


def PrintTestOutput(test_name, json_file_name=None):
  """Helper method to print the output of previously executed test_name.

  Args:
    test_name: name of the test that has been previously executed.
    json_file_name: name of the file to output chartjson data to.

  Returns:
    exit code generated by the test step.
  """
  persisted_result = GetPersistedResult(test_name)
  if not persisted_result:
    return 1
  logging.info('*' * 80)
  logging.info('Output from:')
  logging.info(persisted_result['cmd'])
  logging.info('*' * 80)

  output_formatted = ''
  persisted_outputs = persisted_result['output']
  for i in xrange(len(persisted_outputs)):
    output_formatted += '\n\nOutput from run #%d:\n\n%s' % (
        i, persisted_outputs[i])
  print output_formatted

  if json_file_name:
    with file(json_file_name, 'w') as f:
      f.write(persisted_result['chartjson'])

  return persisted_result['exit_code']


def PrintSummary(test_names):
  logging.info('*' * 80)
  logging.info('Sharding summary')
  device_total_time = collections.defaultdict(int)
  for test_name in test_names:
    file_name = os.path.join(constants.PERF_OUTPUT_DIR, test_name)
    if not os.path.exists(file_name):
      logging.info('%s : No status file found', test_name)
      continue
    with file(file_name, 'r') as f:
      result = pickle.loads(f.read())
    logging.info('%s : exit_code=%d in %d secs at %s',
                 result['name'], result['exit_code'], result['total_time'],
                 result['device'])
    device_total_time[result['device']] += result['total_time']
  for device, device_time in device_total_time.iteritems():
    logging.info('Total for device %s : %d secs', device, device_time)
  logging.info('Total steps time: %d secs', sum(device_total_time.values()))


class _HeartBeatLogger(object):
  # How often to print the heartbeat on flush().
  _PRINT_INTERVAL = 30.0

  def __init__(self):
    """A file-like class for keeping the buildbot alive."""
    self._len = 0
    self._tick = time.time()
    self._stopped = threading.Event()
    self._timer = threading.Thread(target=self._runner)
    self._timer.start()

  def _runner(self):
    while not self._stopped.is_set():
      self.flush()
      self._stopped.wait(_HeartBeatLogger._PRINT_INTERVAL)

  def write(self, data):
    self._len += len(data)

  def flush(self):
    now = time.time()
    if now - self._tick >= _HeartBeatLogger._PRINT_INTERVAL:
      self._tick = now
      print '--single-step output length %d' % self._len
      sys.stdout.flush()

  def stop(self):
    self._stopped.set()


class TestRunner(base_test_runner.BaseTestRunner):
  def __init__(self, test_options, device, shard_index, max_shard, tests,
      flaky_tests):
    """A TestRunner instance runs a perf test on a single device.

    Args:
      test_options: A PerfOptions object.
      device: Device to run the tests.
      shard_index: the index of this device.
      max_shards: the maximum shard index.
      tests: a dict mapping test_name to command.
      flaky_tests: a list of flaky test_name.
    """
    super(TestRunner, self).__init__(device, None)
    self._options = test_options
    self._shard_index = shard_index
    self._max_shard = max_shard
    self._tests = tests
    self._flaky_tests = flaky_tests
    self._output_dir = None
    self._device_battery = battery_utils.BatteryUtils(self.device)

  @staticmethod
  def _SaveResult(result):
    pickled = os.path.join(constants.PERF_OUTPUT_DIR, result['name'])
    if os.path.exists(pickled):
      with file(pickled, 'r') as f:
        previous = pickle.loads(f.read())
        result['output'] = previous['output'] + result['output']

    with file(pickled, 'w') as f:
      f.write(pickle.dumps(result))

  def _CheckDeviceAffinity(self, test_name):
    """Returns True if test_name has affinity for this shard."""
    affinity = (self._tests['steps'][test_name]['device_affinity'] %
                self._max_shard)
    if self._shard_index == affinity:
      return True
    logging.info('Skipping %s on %s (affinity is %s, device is %s)',
                 test_name, self.device_serial, affinity, self._shard_index)
    return False

  def _CleanupOutputDirectory(self):
    if self._output_dir:
      shutil.rmtree(self._output_dir, ignore_errors=True)
      self._output_dir = None

  def _ReadChartjsonOutput(self):
    if not self._output_dir:
      return ''

    json_output_path = os.path.join(self._output_dir, 'results-chart.json')
    try:
      with open(json_output_path) as f:
        return f.read()
    except IOError:
      logging.exception('Exception when reading chartjson.')
      logging.error('This usually means that telemetry did not run, so it could'
                    ' not generate the file. Please check the device running'
                    ' the test.')
      return ''

  def _LaunchPerfTest(self, test_name):
    """Runs a perf test.

    Args:
      test_name: the name of the test to be executed.

    Returns:
      A tuple containing (Output, base_test_result.ResultType)
    """
    if not self._CheckDeviceAffinity(test_name):
      return '', base_test_result.ResultType.PASS

    try:
      logging.warning('Unmapping device ports')
      forwarder.Forwarder.UnmapAllDevicePorts(self.device)
      self.device.RestartAdbd()
    except Exception as e: # pylint: disable=broad-except
      logging.error('Exception when tearing down device %s', e)

    cmd = ('%s --device %s' %
           (self._tests['steps'][test_name]['cmd'],
            self.device_serial))

    if self._options.collect_chartjson_data:
      self._output_dir = tempfile.mkdtemp()
      cmd = cmd + ' --output-dir=%s' % self._output_dir

    logging.info(
        'temperature: %s (0.1 C)',
        str(self._device_battery.GetBatteryInfo().get('temperature')))
    if self._options.max_battery_temp:
      self._device_battery.LetBatteryCoolToTemperature(
          self._options.max_battery_temp)

    logging.info('Charge level: %s%%',
        str(self._device_battery.GetBatteryInfo().get('level')))
    if self._options.min_battery_level:
      self._device_battery.ChargeDeviceToLevel(
          self._options.min_battery_level)

    logging.info('%s : %s', test_name, cmd)
    start_time = time.time()

    timeout = self._tests['steps'][test_name].get('timeout', 3600)
    if self._options.no_timeout:
      timeout = None
    logging.info('Timeout for %s test: %s', test_name, timeout)
    full_cmd = cmd
    if self._options.dry_run:
      full_cmd = 'echo %s' % cmd

    logfile = sys.stdout
    if self._options.single_step:
      # Just print a heart-beat so that the outer buildbot scripts won't timeout
      # without response.
      logfile = _HeartBeatLogger()
    cwd = os.path.abspath(constants.DIR_SOURCE_ROOT)
    if full_cmd.startswith('src/'):
      cwd = os.path.abspath(os.path.join(constants.DIR_SOURCE_ROOT, os.pardir))
    try:
      exit_code, output = cmd_helper.GetCmdStatusAndOutputWithTimeout(
          full_cmd, timeout, cwd=cwd, shell=True, logfile=logfile)
      json_output = self._ReadChartjsonOutput()
    except cmd_helper.TimeoutError as e:
      exit_code = -1
      output = e.output
      json_output = ''
    finally:
      self._CleanupOutputDirectory()
      if self._options.single_step:
        logfile.stop()
    end_time = time.time()
    if exit_code is None:
      exit_code = -1
    logging.info('%s : exit_code=%d in %d secs at %s',
                 test_name, exit_code, end_time - start_time,
                 self.device_serial)

    if exit_code == 0:
      result_type = base_test_result.ResultType.PASS
    else:
      result_type = base_test_result.ResultType.FAIL
      # Since perf tests use device affinity, give the device a chance to
      # recover if it is offline after a failure. Otherwise, the master sharder
      # will remove it from the pool and future tests on this device will fail.
      try:
        self.device.WaitUntilFullyBooted(timeout=120)
      except device_errors.CommandTimeoutError as e:
        logging.error('Device failed to return after %s: %s', test_name, e)

    actual_exit_code = exit_code
    if test_name in self._flaky_tests:
      # The exit_code is used at the second stage when printing the
      # test output. If the test is flaky, force to "0" to get that step green
      # whilst still gathering data to the perf dashboards.
      # The result_type is used by the test_dispatcher to retry the test.
      exit_code = 0

    persisted_result = {
        'name': test_name,
        'output': [output],
        'chartjson': json_output,
        'exit_code': exit_code,
        'actual_exit_code': actual_exit_code,
        'result_type': result_type,
        'start_time': start_time,
        'end_time': end_time,
        'total_time': end_time - start_time,
        'device': self.device_serial,
        'cmd': cmd,
    }
    self._SaveResult(persisted_result)

    return (output, result_type)

  def RunTest(self, test_name):
    """Run a perf test on the device.

    Args:
      test_name: String to use for logging the test result.

    Returns:
      A tuple of (TestRunResults, retry).
    """
    _, result_type = self._LaunchPerfTest(test_name)
    results = base_test_result.TestRunResults()
    results.AddResult(base_test_result.BaseTestResult(test_name, result_type))
    retry = None
    if not results.DidRunPass():
      retry = test_name
    return results, retry
