// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include <stdio.h>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/ttx_tests.hpp"

using namespace Validation;

using namespace Perimortem::Core;

static Harness TtxAbi = {
  .name = "TTX::ABI (C++ / TTX mixing)"_view,
};

extern "C" void print(View::Bytes data) {
  fflush(stdout);
  View::Bytes debug_text = "[TTX DEBUG] "_view;
  View::Bytes end_line = "\n"_view;
  fwrite(debug_text.get_data(), 1, debug_text.get_size(), stdout);
  fwrite(data.get_data(), 1, data.get_size(), stdout);
  fwrite(end_line.get_data(), 1, end_line.get_size(), stdout);
}

PERIMORTEM_UNIT_TEST(TtxAbi, round_trip) {
  Ttx::TTX_source_tetrodotoxin_test("Round trip text!"_view);
}
