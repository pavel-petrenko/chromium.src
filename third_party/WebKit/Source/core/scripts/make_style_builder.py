#!/usr/bin/env python
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

import re
import sys

import in_generator


class StyleBuilderWriter(in_generator.Writer):
    class_name = 'StyleBuilder'

    valid_values = {
        'applytype': ['default', 'length'],
        'usenone': [True, False],
        'useintrinsic': [True, False],
        'useauto': [True, False],
    }
    defaults = {
        'condition': None,
        'applytype': 'default',
        'nameformethods': None,
# These depend on property name by default
        'typename': None,
        'getter': None,
        'setter': None,
        'initial': None,
# For the length apply type
        'usenone': False,
        'useintrinsic': False,
        'useauto': False,
    }

    def __init__(self, in_files, enabled_conditions):
        super(StyleBuilderWriter, self).__init__(in_files, enabled_conditions)
        self._properties = self.in_file.name_dictionaries

        def set_if_none(property, key, value):
            if property[key] is None:
                property[key] = value

        for property in self._properties:
            cc = self._camelcase_property_name(property["name"])
            property["propertyid"] = "CSSProperty" + cc
            cc = property["nameformethods"] or cc.replace("Webkit", "")
            set_if_none(property, "typename", "E" + cc)
            set_if_none(property, "getter", self._lower_first(cc))
            set_if_none(property, "setter", "set" + cc)
            set_if_none(property, "initial", "initial" + cc)

# FIXME: some of these might be better in a utils file
    @staticmethod
    def _camelcase_property_name(property_name):
        return re.sub(r'(^[^-])|-(.)', lambda match: (match.group(1) or match.group(2)).upper(), property_name)

    @staticmethod
    def _lower_first(s):
        return s[0].lower() + s[1:]

    @staticmethod
    def _upper_first(s):
        return s[0].upper() + s[1:]

    def generate_implementation(self):
        return {
            "properties": self._properties,
        }


if __name__ == "__main__":
    in_generator.Maker(StyleBuilderWriter).main(sys.argv)
