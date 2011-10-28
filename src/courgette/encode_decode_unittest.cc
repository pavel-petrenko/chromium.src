// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "courgette/base_test_unittest.h"
#include "courgette/courgette.h"
#include "courgette/streams.h"

class EncodeDecodeTest : public BaseTest {
 public:
  void TestExe(const char *) const;
};

void EncodeDecodeTest::TestExe(const char* file_name) const {
  // Test top-level Courgette API for converting an a file to a binary
  // assembly representation and back.
  std::string file1 = FileContents(file_name);

  const void* original_buffer = file1.c_str();
  size_t original_length = file1.size();

  courgette::AssemblyProgram* program = NULL;
  const courgette::Status parse_status =
      courgette::ParseDetectedExecutable(original_buffer, original_length,
                                         &program);
  EXPECT_EQ(courgette::C_OK, parse_status);

  courgette::EncodedProgram* encoded = NULL;

  const courgette::Status encode_status = Encode(program, &encoded);
  EXPECT_EQ(courgette::C_OK, encode_status);

  DeleteAssemblyProgram(program);

  courgette::SinkStreamSet sinks;
  const courgette::Status write_status = WriteEncodedProgram(encoded, &sinks);
  EXPECT_EQ(courgette::C_OK, write_status);

  DeleteEncodedProgram(encoded);

  courgette::SinkStream sink;
  bool can_collect = sinks.CopyTo(&sink);
  EXPECT_TRUE(can_collect);

  const void* buffer = sink.Buffer();
  size_t length = sink.Length();

  EXPECT_EQ(971850U, length);

  courgette::SourceStreamSet sources;
  bool can_get_source_streams = sources.Init(buffer, length);
  EXPECT_TRUE(can_get_source_streams);

  courgette::EncodedProgram *encoded2 = NULL;
  const courgette::Status read_status = ReadEncodedProgram(&sources, &encoded2);
  EXPECT_EQ(courgette::C_OK, read_status);

  courgette::SinkStream assembled;
  const courgette::Status assemble_status = Assemble(encoded2, &assembled);
  EXPECT_EQ(courgette::C_OK, assemble_status);

  const void* assembled_buffer = assembled.Buffer();
  size_t assembled_length = assembled.Length();

  EXPECT_EQ(original_length, assembled_length);
  EXPECT_EQ(0, memcmp(original_buffer, assembled_buffer, original_length));

  DeleteEncodedProgram(encoded2);
}


TEST_F(EncodeDecodeTest, All) {
  TestExe("setup1.exe");
}
