// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/view/bytes.hpp"

#include "ttx_generated/ttx_tests.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxAbi = {
  .name = "TTX::ABI"_view,
};

static View::Bytes printed[4];
static Count printed_count = 0;

extern "C" void print(View::Bytes data) {
  if (printed_count < 4) {
    printed[printed_count++] = data;
  }
}

PERIMORTEM_UNIT_TEST(TtxAbi, round_trip) {
  printed_count = 0;

  Ttx::TTX_source_tetrodotoxin_test("Round trip text!"_view);

  ASSERT(printed_count == 2);
  EXPECT_TEXT(printed[0], "Compiler Test"_view);
  EXPECT_TEXT(printed[1], "Round trip text!"_view);
}
