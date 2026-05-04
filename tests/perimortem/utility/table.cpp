// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/utility/null_terminated.hpp"
#include "perimortem/utility/table.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;

using namespace Validation;

constexpr Pair<View::Bytes, Int> keyword_source[] = {
    {"as"_view, 1},       {"if"_view, 2},       {"for"_view, 3},
    {"new"_view, 4},      {"else"_view, 5},     {"func"_view, 6},
    {"init"_view, 7},     {"self"_view, 8},     {"true"_view, 9},
    {"alias"_view, 10},   {"debug"_view, 11},   {"error"_view, 12},
    {"false"_view, 13},   {"using"_view, 14},   {"while"_view, 15},
    {"entity"_view, 16},  {"object"_view, 17},  {"return"_view, 18},
    {"struct"_view, 19},  {"library"_view, 20}, {"on_load"_view, 21},
    {"package"_view, 22}, {"warning"_view, 23},
};

static constexpr View::Vector table_data(keyword_source);
using keyword_table = Table<Int, table_data>;

Test::Harness StaticTable = {.name = "Static::Table"};

PERIMORTEM_UNIT_TEST(StaticTable, keyword_lookup) {
  EXPECT(keyword_table::get_memory_consumption() < 256);

  for (Count i = 0; i < Data::array_size(keyword_source); i++) {
    EXPECT_EQ(keyword_table::find_or_default(keyword_source[i].key, -1),
              keyword_source[i].value);
  }

  EXPECT_EQ(keyword_table::find_or_default("unknown"_view, -1), -1);
  EXPECT_EQ(keyword_table::find_or_default("a"_view, -1), -1);
  EXPECT_EQ(keyword_table::find_or_default("As"_view, -1), -1);
  EXPECT_EQ(keyword_table::find_or_default(""_view, -1), -1);
  EXPECT_EQ(keyword_table::find_or_default("rutern"_view, -1), -1);
  EXPECT_EQ(keyword_table::find_or_default("errrr"_view, -1), -1);
}
