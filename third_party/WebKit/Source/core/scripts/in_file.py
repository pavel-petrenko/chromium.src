# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
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

import copy
import os

# NOTE: This has only been used to parse
# core/page/RuntimeEnabledFeatures.in and may not be capable
# of parsing other .in files correctly.

# .in file format is:
# // comment
# name1 arg=value, arg2=value2, arg2=value3
#
# InFile must be passed a dictionary of default values
# with which to validate arguments against known names.
# Sequence types as default values will produce sequences
# as parse results.
# Bare arguments (no '=') are treated as names with value True.
# The first field will always be labeled 'name'.
#
# InFile.load_from_path('file.in', {'arg': None, 'arg2': []})
#
# Parsing produces an array of dictionaries:
# [ { 'name' : 'name1', 'arg' :' value', arg2=['value2', 'value3'] }
class InFile(object):
    def __init__(self, lines, defaults):
        lines = map(str.strip, lines)
        lines = filter(lambda line: line and not line.startswith("//"), lines)
        self.name_dictionaries = [self._parse_line(line, defaults) for line in lines]

    @classmethod
    def load_from_path(self, path, defaults):
        with open(os.path.abspath(path)) as in_file:
            return InFile(in_file.readlines(), defaults)

    def _is_sequence(self, arg):
        return (not hasattr(arg, "strip")
                and hasattr(arg, "__getitem__")
                or hasattr(arg, "__iter__"))

    def _parse_line(self, line, defaults):
        args = copy.deepcopy(defaults)
        parts = line.split(' ')
        args['name'] = parts[0]
        # re-join the rest of the line and split on ','
        args_list = ' '.join(parts[1:]).strip().split(',')
        for arg_string in args_list:
            arg_string = arg_string.strip()
            if not arg_string: # Ignore empty args
                continue
            if '=' in arg_string:
                arg_name, arg_value = arg_string.split('=')
            else:
                arg_name, arg_value = arg_string, True
            if arg_name not in defaults:
                # FIXME: This should probably raise instead of exit(1)
                print "Unknown argument: '%s' in line:\n%s\nKnown arguments: %s" % (arg_name, line, defaults.keys())
                exit(1)
            if self._is_sequence(args[arg_name]):
                args[arg_name].append(arg_value)
            else:
                args[arg_name] = arg_value
        return args
