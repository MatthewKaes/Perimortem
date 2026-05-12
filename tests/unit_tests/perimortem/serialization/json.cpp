// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/json/node.hpp"

#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::System;

using namespace Validation;

Test::Harness SerializationJson = {.name = "Serialization::Json"};

PERIMORTEM_UNIT_TEST(SerializationJson, empty_node) {
  auto start_requests = Bibliotheca::check_out_requests();
  Json::Node empty;

  EXPECT_EQ(empty.is_null(), true);

  EXPECT_EQ(empty.get_flag(), false);
  EXPECT_EQ(empty.get_number(), 0);
  EXPECT_EQ(empty.get_real(), 0.0);
  EXPECT_EQ(empty.get_size(), 0);
  EXPECT_TEXT(empty.get_string(), View::Bytes());

  // Node should never affect the check out state for these tests.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SerializationJson, value_node) {
  auto start_requests = Bibliotheca::check_out_requests();
  Json::Node value;
  EXPECT_EQ(value.is_null(), true);

  value = True;
  EXPECT_EQ(value.get_flag(), true);
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 0.0);
  EXPECT_TEXT(value.get_string(), View::Bytes());
  EXPECT_EQ(value.get_size(), 0);

  value = 142ll;
  EXPECT_EQ(value.get_flag(), 0);
  EXPECT_EQ(value.get_number(), 142ll);
  EXPECT_EQ(value.get_real(), 0.0);
  EXPECT_TEXT(value.get_string(), View::Bytes());
  EXPECT_EQ(value.get_size(), 0);

  value = 12.12;
  EXPECT_EQ(value.get_flag(), 0);
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 12.12);
  EXPECT_TEXT(value.get_string(), View::Bytes());
  EXPECT_EQ(value.get_size(), 0);

  value = "Test View"_view;
  EXPECT_EQ(value.get_flag(), 0);
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 0);
  EXPECT_TEXT(value.get_string(), "Test View"_view);
  EXPECT_EQ(value.get_size(), 9);

  // Node should never affect the check out state for these tests.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SerializationJson, access_never_faults) {
  auto start_requests = Bibliotheca::check_out_requests();
  Json::Node value;
  EXPECT_EQ(value.is_null(), true);

  value = True;
  EXPECT_EQ(value["invalid"_view].is_null(), true);
  EXPECT_EQ(value["invalid"_view]["access"_view].is_null(), true);

  value = value["invalid"_view]["access"_view];

  EXPECT_EQ(value.get_flag(), false);
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 0.0);
  EXPECT_EQ(value.get_size(), 0);
  EXPECT_TEXT(value.get_string(), View::Bytes());

  // Node should never affect the check out state for these tests.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SerializationJson, access_is_value_type) {
  auto start_requests = Bibliotheca::check_out_requests();
  Json::Node value;
  EXPECT_EQ(value.is_null(), true);

  auto new_node = value["invalid"_view];
  EXPECT_EQ(new_node.is_null(), true);

  new_node = "valid"_view;
  EXPECT_EQ(new_node.is_null(), false);
  EXPECT_TEXT(new_node.get_string(), "valid"_view);

  // Original value node should remain unchanged.
  EXPECT_EQ(value.is_null(), true);
  EXPECT_EQ(value["invalid"_view].is_null(), true);

  // Value type nodes should never allocate.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_values) {
  Allocator::Arena arena;
  Json::Node value;
  value.parse(arena, ""_view);
  EXPECT_EQ(value.is_null(), true);

  value.parse(arena, "true"_view);
  EXPECT_EQ(value.get_flag(), true);

  value.parse(arena, "false"_view);
  EXPECT_EQ(value.get_flag(), false);

  value.parse(arena, "12"_view);
  EXPECT_EQ(value.get_number(), 12);

  value.parse(arena, "-9000"_view);
  EXPECT_EQ(value.get_number(), -9000);

  value.parse(arena, "12.06"_view);
  EXPECT_EQ(value.get_real(), 12.06);

  value.parse(arena, "-130.4"_view);
  EXPECT_EQ(value.get_real(), -130.4);

  value.parse(arena, "\"Sub View\""_view);
  EXPECT_TEXT(value.get_string(), "Sub View"_view);

  value.parse(arena, "\"\""_view);
  EXPECT_TEXT(value.get_string(), ""_view);
}

PERIMORTEM_UNIT_TEST(SerializationJson, greedy_parse) {
  Allocator::Arena arena;
  Json::Node value;

  value.parse(arena, "true?"_view);
  EXPECT_EQ(value.get_flag(), true);

  value.parse(arena, "false2"_view);
  EXPECT_EQ(value.get_flag(), false);

  value.parse(arena, "12a"_view);
  EXPECT_EQ(value.get_number(), 12);

  value.parse(arena, " -9000qwerty"_view);
  EXPECT_EQ(value.get_number(), -9000);

  value.parse(arena, " 12.06 "_view);
  EXPECT_EQ(value.get_real(), 12.06);

  value.parse(arena, " -130.a4"_view);
  EXPECT_EQ(value.get_real(), -130.0);

  value.parse(arena, "\"Sub View"_view);
  EXPECT_TEXT(value.get_string(), "Sub View"_view);

  constexpr auto long_view =
      "\"Very long string value used for testing vectorized scans"_view;
  value.parse(arena, long_view);
  EXPECT_TEXT(value.get_string(), long_view.slice(1));
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_arrays) {
  Allocator::Arena arena;
  Json::Node value;

  value.parse(arena, "[]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 0);

  value.parse(arena, "[1,2,3,4]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value[0].get_number(), 1);
  EXPECT_EQ(value[1].get_number(), 2);
  EXPECT_EQ(value[2].get_number(), 3);
  EXPECT_EQ(value[3].get_number(), 4);

  value.parse(arena, "[1,true,\"test\",4]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value[0].get_number(), 1);
  EXPECT_EQ(value[1].get_flag(), true);
  EXPECT_TEXT(value[2].get_string(), "test"_view);
  EXPECT_EQ(value[3].get_number(), 4);
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_nested_arrays) {
  Allocator::Arena arena;
  Json::Node value;

  value.parse(arena, "[]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 0);

  value.parse(arena, "[1,[2],[[true], 3, [ \"value\" ]],[[][[4][]]]]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value[0].get_number(), 1);

  EXPECT_EQ(value[1].get_size(), 1);
  EXPECT_EQ(value[1][0].get_number(), 2);

  EXPECT_EQ(value[2].get_size(), 3);
  EXPECT_EQ(value[2][0].get_size(), 1);
  EXPECT_EQ(value[2][1].get_size(), 0);
  EXPECT_EQ(value[2][2].get_size(), 1);
  EXPECT_EQ(value[2][0][0].get_flag(), true);
  EXPECT_EQ(value[2][1].get_number(), 3);
  EXPECT_TEXT(value[2][2][0].get_string(), "value"_view);

  EXPECT_EQ(value[3][1][0][0].get_number(), 4);
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_object) {
  Allocator::Arena arena;
  Json::Node value;

  value.parse(arena, "{}"_view);
  ASSERT(value.is_object());
  EXPECT_EQ(value.get_size(), 0);

  value.parse(
      arena,
      "{\"value\":1, \"test\":2, \"another member\":3, \"final\":4, }"_view);
  ASSERT(value.is_object());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value["value"_view].get_number(), 1);
  EXPECT_EQ(value["test"_view].get_number(), 2);
  EXPECT_EQ(value["another member"_view].get_number(), 3);
  EXPECT_EQ(value["final"_view].get_number(), 4);

  // Mixed and duplicate, Node should use the first valid member.
  value.parse(
      arena,
      "{\"number\":1,\"flag\":true,\"test\":\"test\",\"number\":-1}"_view);
  ASSERT(value.is_object());
  // We parsed 4 elements but only 3 are accessable via the view.
  // If the raw object is fetched the hidden duplicate can be read.
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value["number"_view].get_number(), 1);
  EXPECT_EQ(value["flag"_view].get_flag(), true);
  EXPECT_TEXT(value["test"_view].get_string(), "test"_view);

  auto members = value.get_object();
  EXPECT_EQ(members.get_size(), 4);
  EXPECT_TEXT(members[0].name, "number"_view);
  EXPECT_TEXT(members[1].name, "flag"_view);
  EXPECT_TEXT(members[2].name, "test"_view);
  EXPECT_TEXT(members[3].name, "number"_view);

  EXPECT_EQ(members[0].node.get_number(), 1);
  EXPECT_EQ(members[1].node.get_flag(), true);
  EXPECT_TEXT(members[2].node.get_string(), "test"_view);
  EXPECT_EQ(members[3].node.get_number(), -1);
}

PERIMORTEM_UNIT_TEST(SerializationJson, format) {
  constexpr auto expected =
      "{\"root\":1,\"flag\":true,\"config\":{\"version\":\"1.0.2\",\"sub_flag\":true}}"_view;
  Json::Node value = Json::Object({
    {"root"_view, Long(1)},
    {"flag"_view, True},
    {"config"_view, Json::Object({
                      {"version"_view, "1.0.2"_view},
                      {"sub_flag"_view, True},
                    })},
  });

  Allocator::Arena arena;
  auto formated = value.format(arena);
  ASSERT_TEXT(formated, expected);
}

PERIMORTEM_UNIT_TEST(SerializationJson, round_trip_init_rpc) {
  File source;
  ASSERT(source.read("tests/data/json/init_rpc.json"_view));

  Allocator::Arena arena;
  Json::Node value;
  value.parse(arena, source.get_view());
  auto formated = value.format(arena);

  ASSERT_EQ(formated.get_size(), source.get_size());
  ASSERT_TEXT(formated, source.get_view());
}
