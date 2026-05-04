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

constexpr Pair<View::Bytes, View::Bytes> word_source[] = {
    {"a"_view, "b"_view},
    {"b"_view, "test"_view},
    {"longer?"_view, "shorter"_view},
    {"possible?"_view, "maybe!"_view},
    {"happy"_view, "feeling glad"_view},
    {"sunshine"_view, "in a bag"_view},
    {"useless"_view, "not for long"_view},
    {"future"_view, "comming on"_view}};

static constexpr View::Vector keyword_data(keyword_source);
using keyword_table = Table<Int, keyword_data, Data::CacheAware::Disabled>;
using keyword_table_aligned =
    Table<Int, keyword_data, Data::CacheAware::Enabled>;

static constexpr View::Vector word_data(word_source);
using word_table = Table<View::Bytes, word_data, Data::CacheAware::Disabled>;
using word_table_aligned =
    Table<View::Bytes, word_data, Data::CacheAware::Enabled>;

Test::Harness StaticTable = {.name = "Utility::Table"};

PERIMORTEM_UNIT_TEST(StaticTable, keyword_table) {
  constexpr auto invalid = -1;

  for (Count i = 0; i < Data::array_size(keyword_source); i++) {
    EXPECT_EQ(keyword_table::find_or_default(keyword_source[i].key, invalid),
              keyword_source[i].value);
  }

  EXPECT_EQ(keyword_table::find_or_default("unknown"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("a"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("As"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default(""_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("rutern"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("errrr"_view, invalid), invalid);
}

PERIMORTEM_UNIT_TEST(StaticTable, keyword_table_aligned) {
  constexpr auto invalid = -1;

  for (Count i = 0; i < Data::array_size(keyword_source); i++) {
    EXPECT_EQ(keyword_table_aligned::find_or_default(keyword_source[i].key, invalid),
              keyword_source[i].value);
  }

  EXPECT_EQ(keyword_table_aligned::find_or_default("unknown"_view, invalid), invalid);
  EXPECT_EQ(keyword_table_aligned::find_or_default("a"_view, invalid), invalid);
  EXPECT_EQ(keyword_table_aligned::find_or_default("As"_view, invalid), invalid);
  EXPECT_EQ(keyword_table_aligned::find_or_default(""_view, invalid), invalid);
  EXPECT_EQ(keyword_table_aligned::find_or_default("rutern"_view, invalid), invalid);
  EXPECT_EQ(keyword_table_aligned::find_or_default("errrr"_view, invalid), invalid);
}

PERIMORTEM_UNIT_TEST(StaticTable, word_table) {
  constexpr auto invalid = "nope"_view;

  for (Count i = 0; i < Data::array_size(word_source); i++) {
    EXPECT_TEXT(word_table::find_or_default(word_source[i].key, invalid),
                word_source[i].value);
  }

  EXPECT_TEXT(word_table::find_or_default("unknown"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("c"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("As"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default(""_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("rutern"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("errrr"_view, invalid), invalid);
}


PERIMORTEM_UNIT_TEST(StaticTable, word_table_aligned) {
  constexpr auto invalid = "nope"_view;

  for (Count i = 0; i < Data::array_size(word_source); i++) {
    EXPECT_TEXT(word_table_aligned::find_or_default(word_source[i].key, invalid),
                word_source[i].value);
  }

  EXPECT_TEXT(word_table_aligned::find_or_default("unknown"_view, invalid), invalid);
  EXPECT_TEXT(word_table_aligned::find_or_default("c"_view, invalid), invalid);
  EXPECT_TEXT(word_table_aligned::find_or_default("As"_view, invalid), invalid);
  EXPECT_TEXT(word_table_aligned::find_or_default(""_view, invalid), invalid);
  EXPECT_TEXT(word_table_aligned::find_or_default("rutern"_view, invalid), invalid);
  EXPECT_TEXT(word_table_aligned::find_or_default("errrr"_view, invalid), invalid);
}