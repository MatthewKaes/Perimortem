// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include <filesystem>
#include <string_view>

#include "storage/formats/base64.hpp"

extern auto load_text(const std::filesystem::path& p) -> std::string;

struct Base64Tests : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  virtual void SetUp() {}

  virtual void TearDown() {}
};

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Base64;

TEST_F(Base64Tests, decode) {
  Arena arena;
  Decoded decode(arena,
                 View::Byte("QmFzZTY0IHRlc3Qgc3RyaW5nIGZvciBQZXJpbW9ydGVtLg=="));

  EXPECT_EQ(decode.get_view(), View::Byte("Base64 test string for Perimortem."));
}

TEST_F(Base64Tests, vectorize_decode) {
  auto source = load_text("core/tests/base64/source.ttx");
  auto base64 = load_text("core/tests/base64/source.base64");
  Arena arena;
  Decoded decode(arena, View::Byte(base64));

  EXPECT_EQ(decode.get_view(), View::Byte(source));
}