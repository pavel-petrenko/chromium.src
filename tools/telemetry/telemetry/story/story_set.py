# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import inspect
import os

from telemetry.story import story as story_module
from telemetry.wpr import archive_info
from telemetry import decorators


class StorySet(object):
  """A collection of stories.

  A typical usage of StorySet would be to subclass it and then call
  AddStory for each Story.
  """

  def __init__(self, archive_data_file='', cloud_storage_bucket=None,
               base_dir=None, serving_dirs=None):
    """Creates a new StorySet.

    Args:
      archive_data_file: The path to Web Page Replay's archive data, relative
          to self.base_dir.
      cloud_storage_bucket: The cloud storage bucket used to download
          Web Page Replay's archive data. Valid values are: None,
          story.PUBLIC_BUCKET, story.PARTNER_BUCKET, or story.INTERNAL_BUCKET
          (defined in telemetry.util.cloud_storage).
      serving_dirs: A set of paths, relative to self.base_dir, to directories
          containing hash files for non-wpr archive data stored in cloud
          storage.
    """
    self.stories = []
    self._archive_data_file = archive_data_file
    self._wpr_archive_info = None
    archive_info.AssertValidCloudStorageBucket(cloud_storage_bucket)
    self._cloud_storage_bucket = cloud_storage_bucket
    if base_dir:
      if not os.path.isdir(base_dir):
        raise ValueError('Invalid directory path of base_dir: %s' % base_dir)
      self._base_dir = base_dir
    else:
      self._base_dir = os.path.dirname(inspect.getfile(self.__class__))
    # Convert any relative serving_dirs to absolute paths.
    self._serving_dirs = set(os.path.realpath(os.path.join(self.base_dir, d))
                             for d in serving_dirs or [])

  @property
  def allow_mixed_story_states(self):
    """True iff Stories are allowed to have different StoryState classes.

    There are no checks in place for determining if SharedStates are
    being assigned correctly to all Stories in a given StorySet. The
    majority of test cases should not need the ability to have multiple
    SharedStates, which usually implies you should be writing multiple
    benchmarks instead. We provide errors to avoid accidentally assigning
    or defaulting to the wrong SharedState.
    Override at your own risk. Here be dragons.
    """
    return False

  @property
  def file_path(self):
    return inspect.getfile(self.__class__).replace('.pyc', '.py')

  @property
  def base_dir(self):
    """The base directory to resolve archive_data_file.

    This defaults to the directory containing the StorySet instance's class.
    """
    return self._base_dir

  @property
  def serving_dirs(self):
    all_serving_dirs = self._serving_dirs.copy()
    for story in self.stories:
      if story.serving_dir:
        all_serving_dirs.add(story.serving_dir)
    return all_serving_dirs

  @property
  def archive_data_file(self):
    return self._archive_data_file

  @property
  def bucket(self):
    return self._cloud_storage_bucket

  @property
  def wpr_archive_info(self):
    """Lazily constructs wpr_archive_info if it's not set and returns it."""
    if self.archive_data_file and not self._wpr_archive_info:
      self._wpr_archive_info = archive_info.WprArchiveInfo.FromFile(
          os.path.join(self.base_dir, self.archive_data_file), self.bucket)
    return self._wpr_archive_info

  def AddStory(self, story):
    assert isinstance(story, story_module.Story)
    self.stories.append(story)

  def RemoveStory(self, story):
    """Removes a Story.

    Allows the stories to be filtered.
    """
    self.stories.remove(story)

  @decorators.Deprecated(
    2015, 7, 19, 'Please use AddStory instead. The user story concept is '
    'being renamed to story.')
  def AddUserStory(self, story):
    self.AddStory(story)

  @decorators.Deprecated(
    2015, 7, 19, 'Please use RemoveStory instead. The user story concept is '
    'being renamed to story.')
  def RemoveUserStory(self, story):
    self.RemoveStory(story)

  @classmethod
  def Name(cls):
    """Returns the string name of this StorySet.
    Note that this should be a classmethod so the benchmark_runner script can
    match the story class with its name specified in the run command:
    'Run <User story test name> <User story class name>'
    """
    return cls.__module__.split('.')[-1]

  @classmethod
  def Description(cls):
    """Return a string explaining in human-understandable terms what this
    story represents.
    Note that this should be a classmethod so the benchmark_runner script can
    display stories' names along with their descriptions in the list command.
    """
    if cls.__doc__:
      return cls.__doc__.splitlines()[0]
    else:
      return ''

  def WprFilePathForStory(self, story):
    """Convenient function to retrieve WPR archive file path.

    Args:
      story: The Story to look up.

    Returns:
      The WPR archive file path for the given Story, if found.
      Otherwise, None.
    """
    if not self.wpr_archive_info:
      return None
    return self.wpr_archive_info.WprFilePathForStory(story)

  @decorators.Deprecated(
    2015, 7, 19, 'Please use WprFilePathForStory instead. The user story '
    'concept is being renamed to story.')
  def WprFilePathForUserStory(self, story):
    return self.WprFilePathForStory(story)

  def __iter__(self):
    return self.stories.__iter__()

  def __len__(self):
    return len(self.stories)

  def __getitem__(self, key):
    return self.stories[key]

  def __setitem__(self, key, value):
    self.stories[key] = value
