// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOFILL_AUTOFILL_FIELD_H_
#define CHROME_BROWSER_AUTOFILL_AUTOFILL_FIELD_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/string16.h"
#include "chrome/browser/autofill/field_types.h"
#include "webkit/glue/form_field.h"

class AutofillField : public webkit_glue::FormField {
 public:
  enum PhonePart {
    IGNORED = 0,
    PHONE_PREFIX = 1,
    PHONE_SUFFIX = 2,
  };

  AutofillField();
  AutofillField(const webkit_glue::FormField& field,
                const string16& unique_name);
  virtual ~AutofillField();

  const string16& unique_name() const { return unique_name_; }

  const string16& section() const { return section_; }
  AutofillFieldType heuristic_type() const { return heuristic_type_; }
  AutofillFieldType server_type() const { return server_type_; }
  const FieldTypeSet& possible_types() const { return possible_types_; }
  PhonePart phone_part() const { return phone_part_; }

  // Sets the heuristic type of this field, validating the input.
  void set_section(const string16& section) { section_ = section; }
  void set_heuristic_type(AutofillFieldType type);
  void set_server_type(AutofillFieldType type);
  void set_possible_types(const FieldTypeSet& possible_types) {
    possible_types_ = possible_types;
  }
  void set_phone_part(PhonePart part) { phone_part_ = part; }

  // This function automatically chooses between server and heuristic autofill
  // type, depending on the data available.
  AutofillFieldType type() const;

  // Returns true if the value of this field is empty.
  bool IsEmpty() const;

  // The unique signature of this field, composed of the field name and the html
  // input type in a 32-bit hash.
  std::string FieldSignature() const;

  // Returns true if the field type has been determined (without the text in the
  // field).
  bool IsFieldFillable() const;

 private:
  // The unique name of this field, generated by Autofill.
  string16 unique_name_;

  // The unique identifier for the section (e.g. billing vs. shipping address)
  // that this field belongs to.
  string16 section_;

  // The type of the field, as determined by the Autofill server.
  AutofillFieldType server_type_;

  // The type of the field, as determined by the local heuristics.
  AutofillFieldType heuristic_type_;

  // The set of possible types for this field.
  FieldTypeSet possible_types_;

  // Used to track whether this field is a phone prefix or suffix.
  PhonePart phone_part_;

  DISALLOW_COPY_AND_ASSIGN(AutofillField);
};

#endif  // CHROME_BROWSER_AUTOFILL_AUTOFILL_FIELD_H_
