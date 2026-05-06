// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/serialization/base64/decode.hpp"
#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

using namespace Validation;

Test::Harness SerializationBase64 = {.name = "Serialization::Base64"};

PERIMORTEM_UNIT_TEST(SerializationBase64, decode) {
  Allocator::Arena arena;
  const auto source = "QmFzZTY0IHRlc3Qgc3RyaW5nIGZvciBQZXJpbW9ydGVtLg=="_view;
  const auto decoded_bytes = Base64::decode(arena, source);

  EXPECT_TEXT(decoded_bytes, "Base64 test string for Perimortem."_view);
}

// PERIMORTEM_UNIT_TEST(SerializationBase64, vectorized_decode) {
//   Allocator::Arena arena;
//   auto source = load_text("perimortem/tests/base64/source.ttx");
//   auto base64 = load_text("perimortem/tests/base64/source.base64");
//   const auto decoded_bytes = Base64::decode(arena, source);
//   Arena arena;
//   Decoded decode(arena, View::Byte(base64));

//   EXPECT_EQ(decode.get_view(), View::Byte(source));
// }

// #include "perimortem/serialization/base64/decode.hpp"

// extern auto load_text(const std::filesystem::path& p) -> std::string;

// struct Base64Tests : public ::testing::Test {
//  protected:
//   static void SetUpTestSuite() {}

//   virtual void SetUp() {}

//   virtual void TearDown() {}
// };

// using namespace Perimortem::Memory;
// using namespace Perimortem::Storage::Base64;

// TEST_F(Base64Tests, decode) {
//   Arena arena;
//   Decoded decode(arena,
//                  View::Byte("QmFzZTY0IHRlc3Qgc3RyaW5nIGZvciBQZXJpbW9ydGVtLg=="));

//   EXPECT_EQ(decode.get_view(), View::Byte("Base64 test string for
//   Perimortem."));
// }

// TEST_F(Base64Tests, vectorize_decode) {
//   auto source = load_text("perimortem/tests/base64/source.ttx");
//   auto base64 = load_text("perimortem/tests/base64/source.base64");
//   Arena arena;
//   Decoded decode(arena, View::Byte(base64));

//   EXPECT_EQ(decode.get_view(), View::Byte(source));
// }