// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/diff_encoder.h"

#include <vector>

#include <gtest/gtest.h>

#include "bsdiff/fake_patch_writer.h"
#include "bsdiff/test_utils.h"

namespace {

// Generated with:
// echo 'Hello World' | hexdump -v -e '"    " 12/1 "0x%02x, " "\n"'
const uint8_t kHelloWorld[] = {
    0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x0a,
};

}  // namespace

namespace bsdiff {

class DiffEncoderTest : public testing::Test {
 protected:
  void SetUp() { EXPECT_TRUE(fake_patch_.Init()); }

  FakePatchWriter fake_patch_;
  std::unique_ptr<DiffEncoder> diff_encoder_;
};

TEST_F(DiffEncoderTest, CreateEmptyPatchTest) {
  diff_encoder_.reset(new DiffEncoder(&fake_patch_, nullptr, 0, nullptr, 0));
  EXPECT_TRUE(diff_encoder_->Close());

  // Both diff and extra stream must be empty stream, and not control entries.
  EXPECT_EQ(0U, fake_patch_.entries().size());
  EXPECT_TRUE(fake_patch_.diff_stream().empty());
  EXPECT_TRUE(fake_patch_.extra_stream().empty());
}

TEST_F(DiffEncoderTest, AllInExtraStreamTest) {
  diff_encoder_.reset(new DiffEncoder(&fake_patch_, nullptr, 0, kHelloWorld,
                                      sizeof(kHelloWorld)));

  // Write to the extra stream in two parts: first 5 bytes, then the rest.
  EXPECT_TRUE(diff_encoder_->AddControlEntry(ControlEntry(0, 5, 0)));
  EXPECT_TRUE(diff_encoder_->AddControlEntry(
      ControlEntry(0, sizeof(kHelloWorld) - 5, 0)));
  EXPECT_TRUE(diff_encoder_->Close());

  EXPECT_EQ(2U, fake_patch_.entries().size());
  EXPECT_TRUE(fake_patch_.diff_stream().empty());
  std::vector<uint8_t> hello_world(kHelloWorld,
                                   kHelloWorld + sizeof(kHelloWorld));
  EXPECT_EQ(hello_world, fake_patch_.extra_stream());
}

TEST_F(DiffEncoderTest, AllInDiffStreamTest) {
  diff_encoder_.reset(new DiffEncoder(&fake_patch_, kHelloWorld,
                                      sizeof(kHelloWorld), kHelloWorld,
                                      sizeof(kHelloWorld)));
  EXPECT_TRUE(
      diff_encoder_->AddControlEntry(ControlEntry(sizeof(kHelloWorld), 0, 0)));
  EXPECT_TRUE(diff_encoder_->Close());

  EXPECT_EQ(std::vector<uint8_t>(sizeof(kHelloWorld), 0),
            fake_patch_.diff_stream());
  EXPECT_TRUE(fake_patch_.extra_stream().empty());
}

}  // namespace bsdiff
