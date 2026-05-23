// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/base64/decode.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::System;

using namespace Validation;

static Harness SerializationBase64 = {
  .name = "Serialization::Base64"_view,
};

PERIMORTEM_UNIT_TEST(SerializationBase64, decode_empty) {
  auto start_requests = Bibliotheca::check_out_requests();
  const auto source = ""_view;
  const auto decoded_bytes = Base64::decode(source);

  EXPECT_TEXT(decoded_bytes.get_view(), ""_view);

  // Checking empty should perform zero allocations.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SerializationBase64, decode_simple) {
  auto start_requests = Bibliotheca::check_out_requests();
  const auto source = "QmFzZTY0IHRlc3Qgc3RyaW5nIGZvciBQZXJpbW9ydGVtLg=="_view;
  const auto decoded_bytes = Base64::decode(source);

  EXPECT_TEXT(
      decoded_bytes.get_view(), "Base64 test string for Perimortem."_view);

  // Should only perform 1 allocations:
  // 1 decode
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests + 1);
}

PERIMORTEM_UNIT_TEST(SerializationBase64, decode_vectorized) {
  auto start_requests = Bibliotheca::check_out_requests();
  File source;
  ASSERT(source.read("validation/data/ttx/source.ttx"_view));
  File base64;
  ASSERT(base64.read("validation/data/base64/source.base64"_view));

  const auto decoded_bytes = Base64::decode(base64.get_view());
  EXPECT_TEXT(decoded_bytes.get_view(), source.get_view());

  // Should only perform 3 allocations:
  // 2 file reads + 1 decode
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests + 3);
}
