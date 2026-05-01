// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/test/test.hpp"

#include "perimortem/memory/static/table.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;

using namespace Validation;

template <typename key_type, typename value_type>
struct Pair {
  static key_type key;
  static value_type value;
};


static constexpr Type::Pair<View::Amorphous, Int> data[] = {
    {"as"_view, 1},       {"if"_view, 2},       {"for"_view, 3},
    {"new"_view, 4},      {"else"_view, 5},     {"func"_view, 6},
    {"init"_view, 7},     {"self"_view, 8},     {"true"_view, 9},
    {"alias"_view, 10},   {"debug"_view, 11},   {"error"_view, 12},
    {"false"_view, 13},   {"using"_view, 14},   {"while"_view, 15},
    {"entity"_view, 16},  {"object"_view, 17},  {"return"_view, 18},
    {"struct"_view, 19},  {"library"_view, 20}, {"on_load"_view, 21},
    {"package"_view, 22}, {"warning"_view, 23},
};

static constexpr View::Structured<
              Type::Pair<View::Amorphous, Int>> value(data);

using keyword_table = Static::Table<Int, value>;

// constexpr auto mem_comp = keyword_table::get_memory_consumption();

Test::Harness StaticMap = {.name = "Keyword Lookup"};
