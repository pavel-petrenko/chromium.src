// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// TODO(nona): Add more tests.

#include "chromeos/dbus/ibus/ibus_text.h"

#include <string>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "chromeos/dbus/ibus/ibus_object.h"
#include "dbus/message.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
// TODO(nona): Remove ibus namespace after complete libibus removal.
namespace ibus {

TEST(IBusTextTest, WriteReadTest) {
  const char kSampleText[] = "Sample Text";
  const IBusText::UnderlineAttribute kSampleUnderlineAttribute1 = {
    IBusText::IBUS_TEXT_UNDERLINE_SINGLE, 10, 20};

  const IBusText::UnderlineAttribute kSampleUnderlineAttribute2 = {
    IBusText::IBUS_TEXT_UNDERLINE_DOUBLE, 11, 21};

  const IBusText::UnderlineAttribute kSampleUnderlineAttribute3 = {
    IBusText::IBUS_TEXT_UNDERLINE_ERROR, 12, 22};

  const IBusText::SelectionAttribute kSampleSelectionAttribute = {30, 40};

  // Make IBusText
  IBusText text;
  text.set_text(kSampleText);
  std::vector<IBusText::UnderlineAttribute>* underline_attributes =
      text.mutable_underline_attributes();
  underline_attributes->push_back(kSampleUnderlineAttribute1);
  underline_attributes->push_back(kSampleUnderlineAttribute2);
  underline_attributes->push_back(kSampleUnderlineAttribute3);
  std::vector<IBusText::SelectionAttribute>* selection_attributes =
      text.mutable_selection_attributes();
  selection_attributes->push_back(kSampleSelectionAttribute);

  // Write to Response object.
  scoped_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter writer(response.get());
  AppendIBusText(text, &writer);

  // Read from Response object.
  dbus::MessageReader reader(response.get());
  IBusText expected_text;
  ASSERT_TRUE(PopIBusText(&reader, &expected_text));
  EXPECT_EQ(expected_text.text(), kSampleText);
  EXPECT_EQ(3U, expected_text.underline_attributes().size());
  EXPECT_EQ(1U, expected_text.selection_attributes().size());
}

TEST(IBusTextTest, StringAsIBusTextTest) {
  const char kSampleText[] = "Sample Text";

  // Write to Response object.
  scoped_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter writer(response.get());
  AppendStringAsIBusText(kSampleText, &writer);

  // Read from Response object.
  dbus::MessageReader reader(response.get());
  IBusText ibus_text;
  ASSERT_TRUE(PopIBusText(&reader, &ibus_text));
  EXPECT_EQ(kSampleText, ibus_text.text());
  EXPECT_TRUE(ibus_text.underline_attributes().empty());
  EXPECT_TRUE(ibus_text.selection_attributes().empty());
}

TEST(IBusTextTest, PopStringFromIBusTextTest) {
  const char kSampleText[] = "Sample Text";

  // Write to Response object.
  scoped_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter writer(response.get());
  AppendStringAsIBusText(kSampleText, &writer);

  // Read from Response object.
  dbus::MessageReader reader(response.get());
  std::string result;
  ASSERT_TRUE(PopStringFromIBusText(&reader, &result));
  EXPECT_EQ(kSampleText, result);
}

TEST(IBusTextTest, ReadAnnotationFieldTest) {
  const char kSampleText[] = "Sample Text";
  const char kSampleAnnotationText[] = "Sample Annotation";

  // Create IBusLookupTable.
  scoped_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter writer(response.get());
  dbus::MessageWriter top_variant_writer(NULL);
  writer.OpenVariant("(sa{sv}sv)", &top_variant_writer);
  dbus::MessageWriter contents_writer(NULL);
  top_variant_writer.OpenStruct(&contents_writer);
  contents_writer.AppendString("IBusText");

  // Write attachment field.
  dbus::MessageWriter attachment_array_writer(NULL);
  contents_writer.OpenArray("{sv}", &attachment_array_writer);
  dbus::MessageWriter entry_writer(NULL);
  attachment_array_writer.OpenDictEntry(&entry_writer);
  entry_writer.AppendString("annotation");
  dbus::MessageWriter variant_writer(NULL);
  entry_writer.OpenVariant("v", &variant_writer);
  dbus::MessageWriter sub_variant_writer(NULL);
  variant_writer.OpenVariant("s", &sub_variant_writer);
  sub_variant_writer.AppendString(kSampleAnnotationText);

  // Close attachment field container.
  variant_writer.CloseContainer(&sub_variant_writer);
  entry_writer.CloseContainer(&variant_writer);
  attachment_array_writer.CloseContainer(&entry_writer);
  contents_writer.CloseContainer(&attachment_array_writer);

  // Write IBusText contents.
  contents_writer.AppendString(kSampleText);
  IBusObjectWriter ibus_attr_list_writer("IBusAttrList", "av",
                                         &contents_writer);
  dbus::MessageWriter attribute_array_writer(NULL);
  ibus_attr_list_writer.OpenArray("v", &attribute_array_writer);
  ibus_attr_list_writer.CloseContainer(&attribute_array_writer);
  ibus_attr_list_writer.CloseAll();

  // Close all containers.
  top_variant_writer.CloseContainer(&contents_writer);
  writer.CloseContainer(&top_variant_writer);

  // Read IBusText.
  IBusText ibus_text;
  dbus::MessageReader reader(response.get());
  PopIBusText(&reader, &ibus_text);

  // Check values.
  EXPECT_EQ(kSampleText, ibus_text.text());
  EXPECT_EQ(kSampleAnnotationText, ibus_text.annotation());
}

TEST(IBusTextTest, ReadDescriptionFieldTest) {
  const char kSampleText[] = "Sample Text";
  const char kSampleDescriptionText[] = "Sample Description";

  // Create IBusLookupTable.
  scoped_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter writer(response.get());
  dbus::MessageWriter top_variant_writer(NULL);
  writer.OpenVariant("(sa{sv}sv)", &top_variant_writer);
  dbus::MessageWriter contents_writer(NULL);
  top_variant_writer.OpenStruct(&contents_writer);
  contents_writer.AppendString("IBusText");

  // Write attachment field.
  dbus::MessageWriter attachment_array_writer(NULL);
  contents_writer.OpenArray("{sv}", &attachment_array_writer);
  dbus::MessageWriter entry_writer(NULL);
  attachment_array_writer.OpenDictEntry(&entry_writer);
  entry_writer.AppendString("description");
  dbus::MessageWriter variant_writer(NULL);
  entry_writer.OpenVariant("v", &variant_writer);
  dbus::MessageWriter sub_variant_writer(NULL);
  variant_writer.OpenVariant("s", &sub_variant_writer);
  sub_variant_writer.AppendString(kSampleDescriptionText);

  // Close attachment field container.
  variant_writer.CloseContainer(&sub_variant_writer);
  entry_writer.CloseContainer(&variant_writer);
  attachment_array_writer.CloseContainer(&entry_writer);
  contents_writer.CloseContainer(&attachment_array_writer);

  // Write IBusText contents.
  contents_writer.AppendString(kSampleText);
  IBusObjectWriter ibus_attr_list_writer("IBusAttrList", "av",
                                         &contents_writer);
  dbus::MessageWriter attribute_array_writer(NULL);
  ibus_attr_list_writer.OpenArray("v", &attribute_array_writer);
  ibus_attr_list_writer.CloseContainer(&attribute_array_writer);
  ibus_attr_list_writer.CloseAll();

  // Close all containers.
  top_variant_writer.CloseContainer(&contents_writer);
  writer.CloseContainer(&top_variant_writer);

  // Read IBusText.
  IBusText ibus_text;
  dbus::MessageReader reader(response.get());
  PopIBusText(&reader, &ibus_text);

  // Check values.
  EXPECT_EQ(kSampleText, ibus_text.text());
  EXPECT_EQ(kSampleDescriptionText, ibus_text.description());
}

}  // namespace ibus
}  // namespace chromeos
