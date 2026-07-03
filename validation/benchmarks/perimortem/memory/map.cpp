// Perimortem Engine
// Copyright © Matt Kaes

#ifdef PERI_BENCH_CPP
#include <string_view>
#include <unordered_map>
#define PERI_SLOW_BENCH
#endif

#include "validation/benchmark.hpp"

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/map.hpp"

#include "perimortem/system/random.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Validation;

constexpr auto scalar = Dynamic::MapVectorization::Scalar;
constexpr auto partial = Dynamic::MapVectorization::Partial;
constexpr auto full = Dynamic::MapVectorization::Full;

constexpr Count max_key_count = 1 << 16;
static Static::Vector<Signed_32, max_key_count> lookup_keys;

auto populate_lookup_keys() -> void {
  static Bool populated = False;
  if (populated) {
    return;
  }
  for (Count i = 0; i < max_key_count; i++) {
    lookup_keys[i] = Signed_32(i);
  }
  populated = True;
}

static Harness MapSigned_32s = {
  .name = "Map Performance"_view,
  .init = populate_lookup_keys,
};

template <Dynamic::MapVectorization vector, Count values, Bool lookup>
auto map_test() -> void {
  Dynamic::Map<Signed_32, Signed_32, vector> local_map(values);
  for (Count i = 0; i < values; i++) {
    local_map.insert(lookup_keys[i], Signed_32(i));
  }
  Signed_32 accumulator = local_map.get_size();

  if constexpr (lookup) {
    Benchmark::start_time();
    accumulator = 0;
    for (Count i = 0; i < values; i++) {
      accumulator += local_map.at(lookup_keys[i % values]);
    }
  }
  Benchmark::end_time();
  Benchmark::prevent_optimization(accumulator);
}

// Usually Perimortem is anti macros but this just saves sooooooo much boiler
// plate code that it's not even funny.
#define MAP_INT_TEST(type, count, var)                               \
  PERIMORTEM_BENCHMARK(MapSigned_32s, var##_##count##_ints_##type) { \
    map_test<type, count, var>();                                    \
  }

#define MAP_INT_TEST_RANGE(count, lookup) \
  MAP_INT_TEST(scalar, count, lookup);    \
  MAP_INT_TEST(partial, count, lookup);   \
  MAP_INT_TEST(full, count, lookup);

constexpr auto lookup = True;
constexpr auto insert = False;

MAP_INT_TEST_RANGE(256, lookup);
MAP_INT_TEST_RANGE(1024, lookup);
#ifdef PERI_SLOW_BENCH
MAP_INT_TEST_RANGE(4096, lookup);
MAP_INT_TEST_RANGE(16384, lookup);
MAP_INT_TEST_RANGE(65536, lookup);
#endif

MAP_INT_TEST_RANGE(256, insert);
MAP_INT_TEST_RANGE(1024, insert);
#ifdef PERI_SLOW_BENCH
MAP_INT_TEST_RANGE(4096, insert);
MAP_INT_TEST_RANGE(16384, insert);
MAP_INT_TEST_RANGE(65536, insert);
#endif

static constexpr Pair<View::Bytes, Count> keyword_source[] = {
  {"as"_view, 0},         {"if"_view, 1},          {"for"_view, 2},
  {"new"_view, 3},        {"else"_view, 4},        {"func"_view, 5},
  {"init"_view, 6},       {"self"_view, 7},        {"true"_view, 8},
  {"alias"_view, 9},      {"debug"_view, 10},      {"error"_view, 11},
  {"false"_view, 12},     {"using"_view, 13},      {"while"_view, 14},
  {"entity"_view, 15},    {"object"_view, 16},     {"return"_view, 17},
  {"struct"_view, 18},    {"library"_view, 19},    {"on_load"_view, 20},
  {"package"_view, 21},   {"warning"_view, 22},    {"testing"_view, 24},
  {"insert_x"_view, 24},  {"deletion"_view, 25},   {"removals"_view, 26},
  {"log_print"_view, 27}, {"log_print2"_view, 28}, {"x"_view, 29},
  {"y"_view, 30},         {"main"_view, 31},
};

static constexpr Pair<View::Bytes, Count> non_keyword_source[] = {
  {"af"_view, 0},         {"1f"_view, 1},          {"f2r"_view, 2},
  {"naw"_view, 3},        {"el.e"_view, 4},        {"fu1c"_view, 5},
  {"ini?"_view, 6},       {"sel2"_view, 7},        {"t4ue"_view, 8},
  {"aliis"_view, 9},      {"d1bug"_view, 10},      {"err7r"_view, 11},
  {"aalse"_view, 12},     {"us0ng"_view, 13},      {"wzile"_view, 14},
  {"entnty"_view, 15},    {"ofjfct"_view, 16},     {"ret9rn"_view, 17},
  {"strzct"_view, 18},    {"lixrary"_view, 19},    {"on_doad"_view, 20},
  {"pa!kag!"_view, 21},   {"war_ing"_view, 22},    {"testi1g"_view, 24},
  {"in3ert_x"_view, 24},  {"de-_tion"_view, 25},   {"remova3s"_view, 26},
  {"log  rint"_view, 27}, {"log_print3"_view, 28}, {"z"_view, 29},
  {"a"_view, 30},         {"nain"_view, 31},
};
static constexpr Count keyword_count = Data::array_size(keyword_source);

using KeywordTable = Table<Count, keyword_source>;

static Static::Vector<const View::Bytes*, max_key_count> loopup_scramble;
auto create_scramble(Count mask) -> void {
  // Try to create a chaotic lookup order with a percentage of bad values
  // Added to the mix.
  for (Count i = 0; i < max_key_count; i++) {
    auto index = Random::generate();
    if (index & mask) {
      // Miss
      loopup_scramble[i] = &non_keyword_source[index % keyword_count].key;
    } else {
      // Hit
      loopup_scramble[i] = &keyword_source[index % keyword_count].key;
    }
  }
}

template <Dynamic::MapVectorization vector, Count values, Count mask>
auto keyword_test() -> void {
  Dynamic::Map<View::Bytes, Signed_32, vector> local_map(keyword_count);
  for (Count i = 0; i < keyword_count; i++) {
    local_map.insert(keyword_source[i].key, keyword_source[i].value);
  }

  create_scramble(mask);
  Signed_32 accumulator = local_map.get_size();

  Benchmark::start_time();
  for (Count i = 0; i < values; i++) {
    accumulator += local_map.find_or_default(*loopup_scramble[i], -1);
  }
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

template <Count values, Count mask>
auto keyword_table() -> void {
  // Try to create a chaotic lookup order with a percentage of bad values
  // Added to the mix.
  create_scramble(mask);
  Signed_32 accumulator = 0;

  Benchmark::start_time();
  for (Count i = 0; i < values; i++) {
    accumulator += KeywordTable::find_or_default(*loopup_scramble[i], -1);
  }
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

static Harness MapKeywords = {
  .name = "Keyword Lookup"_view,
};

#define MAP_KEYWORD_TEST(type, count, mask)                    \
  PERIMORTEM_BENCHMARK(MapKeywords, count##_##mask##_##type) { \
    keyword_test<type, count, mask>();                         \
  }

#define MAP_TABLE_TEST(count, mask)                             \
  PERIMORTEM_BENCHMARK(MapKeywords, count##_##mask##_##table) { \
    keyword_table<count, mask>();                               \
  }

#ifdef PERI_SLOW_BENCH
#define MAP_KEYWORD_TEST_RANGE(count, mask) \
  MAP_KEYWORD_TEST(scalar, count, mask);    \
  MAP_KEYWORD_TEST(partial, count, mask);   \
  MAP_KEYWORD_TEST(full, count, mask);      \
  MAP_TABLE_TEST(count, mask);
#else
#define MAP_KEYWORD_TEST_RANGE(count, mask) \
  MAP_KEYWORD_TEST(partial, count, mask);   \
  MAP_TABLE_TEST(count, mask);
#endif

// Hit ranges in 1 / 1000 chance of keys appearing in the table since table
// behavior changes based on hit rate.
// hit_1000 equal 100.0% source keys
// hit_0016 equal   1.6% source keys
constexpr auto hit_1000 = Count(0b00000000'00000000);
#ifdef PERI_SLOW_BENCH
constexpr auto hit_0500 = Count(0b00000000'00000001);
constexpr auto hit_0062 = Count(0b00000000'00001111);
constexpr auto hit_0016 = Count(0b00000000'00111111);
constexpr auto hit_0004 = Count(0b00000000'11111111);
#endif
constexpr auto hit_0001 = Count(0b00000011'11111111);

MAP_KEYWORD_TEST_RANGE(4096, hit_1000);
#ifdef PERI_SLOW_BENCH
MAP_KEYWORD_TEST_RANGE(4096, hit_0500);
MAP_KEYWORD_TEST_RANGE(4096, hit_0062);
MAP_KEYWORD_TEST_RANGE(4096, hit_0016);
MAP_KEYWORD_TEST_RANGE(4096, hit_0004);
#endif
MAP_KEYWORD_TEST_RANGE(4096, hit_0001);
MAP_KEYWORD_TEST_RANGE(16384, hit_1000);
#ifdef PERI_SLOW_BENCH
MAP_KEYWORD_TEST_RANGE(16384, hit_0500);
MAP_KEYWORD_TEST_RANGE(16384, hit_0062);
MAP_KEYWORD_TEST_RANGE(16384, hit_0016);
MAP_KEYWORD_TEST_RANGE(16384, hit_0004);
#endif
MAP_KEYWORD_TEST_RANGE(16384, hit_0001);
#ifdef PERI_SLOW_BENCH
MAP_KEYWORD_TEST_RANGE(65536, hit_1000);
MAP_KEYWORD_TEST_RANGE(65536, hit_0500);
MAP_KEYWORD_TEST_RANGE(65536, hit_0062);
MAP_KEYWORD_TEST_RANGE(65536, hit_0016);
MAP_KEYWORD_TEST_RANGE(65536, hit_0004);
MAP_KEYWORD_TEST_RANGE(65536, hit_0001);
#endif

#ifdef PERI_BENCH_CPP

template <Count values, Bool is_lookup>
auto cpp_int_map_test() -> void {
  std::unordered_map<int, int> cpp_map;
  cpp_map.reserve(values);
  for (Count i = 0; i < values; i++) {
    cpp_map.emplace(int(lookup_keys[i]), int(i));
  }
  int accumulator = int(cpp_map.size());

  if constexpr (is_lookup) {
    Benchmark::start_time();
    accumulator = 0;
    for (Count i = 0; i < values; i++) {
      auto it = cpp_map.find(int(lookup_keys[i % values]));
      if (it != cpp_map.end()) {
        accumulator += it->second;
      }
    }
  }
  Benchmark::end_time();
  Benchmark::prevent_optimization(accumulator);
}

template <Count values, Count mask>
auto cpp_keyword_test() -> void {
  std::unordered_map<std::string_view, int> cpp_map;
  for (Count i = 0; i < keyword_count; i++) {
    auto& kv = keyword_source[i];
    cpp_map.emplace(
        std::string_view(
            Data::cast<char>(kv.key.get_data()), kv.key.get_size()),
        kv.value);
  }
  create_scramble(mask);
  int accumulator = int(cpp_map.size());

  Benchmark::start_time();
  for (Count i = 0; i < values; i++) {
    auto& key = *loopup_scramble[i];
    auto it = cpp_map.find(
        std::string_view(Data::cast<char>(key.get_data()), key.get_size()));
    accumulator += (it != cpp_map.end()) ? it->second : -1;
  }
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

#define INT_MAP_COMPARISON(count, var)                            \
  static Benchmark::Comparison int_map_##var##_##count = {        \
    .harness = &MapSigned_32s,                                    \
    .label = #var " " #count " ints"_view,                        \
    .variants =                                                   \
        {                                                         \
          Benchmark::ComparisonVariant{                           \
              "scalar"_view, #var "_" #count "_ints_scalar"_view}, \
          Benchmark::ComparisonVariant{                           \
              "partial"_view, #var "_" #count "_ints_partial"_view}, \
          Benchmark::ComparisonVariant{                           \
              "full"_view, #var "_" #count "_ints_full"_view},   \
        },                                                        \
  };                                                              \
  PERIMORTEM_COMPARISON(int_map_##var##_##count) {                \
    cpp_int_map_test<count, var>();                               \
  }

INT_MAP_COMPARISON(256, lookup)
INT_MAP_COMPARISON(1024, lookup)
INT_MAP_COMPARISON(4096, lookup)
INT_MAP_COMPARISON(16384, lookup)
INT_MAP_COMPARISON(65536, lookup)

INT_MAP_COMPARISON(256, insert)
INT_MAP_COMPARISON(1024, insert)
INT_MAP_COMPARISON(4096, insert)
INT_MAP_COMPARISON(16384, insert)
INT_MAP_COMPARISON(65536, insert)

#define KEYWORD_COMPARISON(count, mask)                       \
  static Benchmark::Comparison kw_##count##_##mask##_comp = { \
    .harness = &MapKeywords,                                  \
    .label = #count " " #mask ""_view,                        \
    .variants =                                               \
        {                                                     \
          Benchmark::ComparisonVariant{                       \
              "scalar"_view, #count "_" #mask "_scalar"_view}, \
          Benchmark::ComparisonVariant{                       \
              "partial"_view, #count "_" #mask "_partial"_view}, \
          Benchmark::ComparisonVariant{                       \
              "full"_view, #count "_" #mask "_full"_view},   \
          Benchmark::ComparisonVariant{                       \
              "table"_view, #count "_" #mask "_table"_view}, \
        },                                                    \
  };                                                          \
  PERIMORTEM_COMPARISON(kw_##count##_##mask##_comp) {         \
    cpp_keyword_test<count, mask>();                          \
  }

KEYWORD_COMPARISON(4096, hit_1000)
KEYWORD_COMPARISON(4096, hit_0001)
KEYWORD_COMPARISON(16384, hit_1000)
KEYWORD_COMPARISON(16384, hit_0001)

#endif  // PERI_BENCH_CPP
