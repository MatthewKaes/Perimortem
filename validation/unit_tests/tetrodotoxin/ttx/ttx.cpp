// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include <stdio.h>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx_generated/ttx_tests.hpp"

using namespace Validation;

using namespace Perimortem::Core;

static Harness TtxAbi = {
  .name = "TTX::ABI (C++ / TTX mixing)"_view,
};

static Bool received_structured_message = False;
static View::Bytes received_title;
static Bits_32 received_count = 0;
static Real_32 received_ratio = 0;
static Bool received_object_pair = False;
static View::Bytes received_left_title;
static Bits_32 received_left_count = 0;
static Real_32 received_left_ratio = 0;
static View::Bytes received_right_title;
static Bits_32 received_right_count = 0;
static Real_32 received_right_ratio = 0;

extern "C" void print(View::Bytes data) {
  fflush(stdout);
  View::Bytes debug_text = "[TTX DEBUG] "_view;
  View::Bytes end_line = "\n"_view;
  fwrite(debug_text.get_data(), 1, debug_text.get_size(), stdout);
  fwrite(data.get_data(), 1, data.get_size(), stdout);
  fwrite(end_line.get_data(), 1, end_line.get_size(), stdout);
}

extern "C" void receive_message(const Ttx::TTX_source_Message* message) {
  received_structured_message = True;
  received_title = message->title;
  received_count = message->count;
  received_ratio = message->ratio;
}

extern "C" void receive_pair(
    const Ttx::TTX_source_Message* left,
    const Ttx::TTX_source_Message* right) {
  received_object_pair = True;
  received_left_title = left->title;
  received_left_count = left->count;
  received_left_ratio = left->ratio;
  received_right_title = right->title;
  received_right_count = right->count;
  received_right_ratio = right->ratio;
}

PERIMORTEM_UNIT_TEST(TtxAbi, round_trip) {
  Ttx::TTX_source_tetrodotoxin_test("Round trip text!"_view);
}

PERIMORTEM_UNIT_TEST(TtxAbi, structured_object) {
  received_structured_message = False;
  received_title = View::Bytes();
  received_count = 0;
  received_ratio = 0;

  Ttx::TTX_source_structured_test();

  EXPECT(received_structured_message);
  EXPECT_TEXT(received_title, "Structured"_view);
  EXPECT_EQ(received_count, Bits_32(7));
  EXPECT(received_ratio == Real_32(-0.5f));
}

PERIMORTEM_UNIT_TEST(TtxAbi, object_pair) {
  received_object_pair = False;
  received_left_title = View::Bytes();
  received_left_count = 0;
  received_left_ratio = 0;
  received_right_title = View::Bytes();
  received_right_count = 0;
  received_right_ratio = 0;

  Ttx::TTX_source_object_pair_test();

  EXPECT(received_object_pair);
  EXPECT_TEXT(received_left_title, "Left"_view);
  EXPECT_EQ(received_left_count, Bits_32(2));
  EXPECT(received_left_ratio == Real_32(0.25f));
  EXPECT_TEXT(received_right_title, "Right"_view);
  EXPECT_EQ(received_right_count, Bits_32(3));
  EXPECT(received_right_ratio == Real_32(-0.75f));
}
