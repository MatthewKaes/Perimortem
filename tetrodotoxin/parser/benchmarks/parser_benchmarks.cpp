// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "parser/parse_tables.hpp"
#include "parser/token.hpp"
#include "parser/ttx.hpp"

#include "concepts/sparse_index.hpp"

using namespace Tetrodotoxin::Language::Parser;

const int tests = 1000;

template <class T>
void doNotOptimizeAway(T&& datum) {
  asm volatile("" : "+r"(datum));
}

static void BRUTE_FORCE_compare_table(benchmark::State& state) {
  std::vector<std::string_view> values;
  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    values.push_back(KeywordTable::data[i].key);
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = values[i % values.size()];
      bool found = false;
      for (auto item : KeywordTable::data) {
        if (entry.compare(item.key) == 0) {
          doNotOptimizeAway(item.value);
          found = true;
          break;
        }
      }

      if (!found)
        doNotOptimizeAway(Classifier::None);
    }
  }
}
BENCHMARK(BRUTE_FORCE_compare_table);

static void BASELINE_unordered_map(benchmark::State& state) {
  std::vector<std::string_view> values;
  std::unordered_map<std::string_view, Classifier> data;
  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    values.push_back(KeywordTable::data[i].key);
    data[KeywordTable::data[i].key] = KeywordTable::data[i].value;
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      uint32_t index = i % values.size();
      if (data.contains(values[index])) {
        doNotOptimizeAway(data[values[index]]);
      }
    }
  }
}
BENCHMARK(BASELINE_unordered_map);

static void OPTIMIZED_STL_vector_table(benchmark::State& state) {
  std::vector<std::string_view> values;
  std::array<std::vector<std::pair<std::string_view, Classifier>>, 8>
      vector_table;

  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    values.push_back(KeywordTable::data[i].key);
    uint32_t index =
        std::char_traits<char>::length(KeywordTable::data[i].key) - 1;
    vector_table[index].push_back(
        std::make_pair(std::string_view{KeywordTable::data[i].key, index + 1},
                       KeywordTable::data[i].value));
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = values[i % values.size()];
      if (entry.size() == 0 || entry.size() > vector_table.size()) {
        continue;
      }

      auto& bucket = vector_table[entry.size() - 1];
      for (int k = 0; k < bucket.size(); k++) {
        auto& item = bucket[k];
        if (item.first[0] != entry[0])
          continue;

        if (item.first.compare(entry) != 0)
          continue;

        doNotOptimizeAway(item.second);
      }

      doNotOptimizeAway(Classifier::None);
    }
  }
}
BENCHMARK(OPTIMIZED_STL_vector_table);

using collision_lookup = SparseLookupTable<KeywordTable::value_type,
                                           array_size(KeywordTable::data),
                                           KeywordTable::data,
                                           150>;
static_assert(!collision_lookup::has_perfect_hash());

static void sparse_lookup_table_collision(benchmark::State& state) {
  std::vector<std::string_view> values;
  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    values.push_back(KeywordTable::data[i].key);
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      uint32_t index = i % values.size();
      doNotOptimizeAway(collision_lookup::find_or_default(
          values[index].data(), values[index].size(), Classifier::None));
    }
  }
}
BENCHMARK(sparse_lookup_table_collision);

static void sparse_lookup_table_perfect_stack(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      uint32_t index = i % KeywordTable::lookup::item_count;
      doNotOptimizeAway(KeywordTable::lookup::find_or_default(
          KeywordTable::data[index].key, Classifier::None));
    }
  }
}
BENCHMARK(sparse_lookup_table_perfect_stack);

static void sparse_lookup_table_perfect_view(benchmark::State& state) {
  std::vector<std::string_view> values;
  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    values.push_back(KeywordTable::data[i].key);
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      uint32_t index = i % values.size();
      doNotOptimizeAway(KeywordTable::lookup::find_or_default(
          values[index].data(), values[index].size(), Classifier::None));
    }
  }
}
BENCHMARK(sparse_lookup_table_perfect_view);

using index_lookup = SparseIndexTable<KeywordTable::value_type,
                                      array_size(KeywordTable::data),
                                      KeywordTable::data>;

static void sparse_index_table(benchmark::State& state) {
  std::vector<std::string_view> values;
  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    values.push_back(KeywordTable::data[i].key);
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      uint32_t index = i % values.size();
      doNotOptimizeAway(
          index_lookup::find_or_default(values[index], Classifier::None));
    }
  }
}
BENCHMARK(sparse_index_table);

// constexpr auto lookup_size = sizeof(KeywordTable::lookup::sparse_table);
// constexpr auto radix_size = sizeof(radix_lookup::sparse_table);
// constexpr auto index_size = sizeof(index_lookup::sparse_table);

#define K_SECTION(size) \
  case size: {          \
    constexpr uint64_t section_bytes = size;
#define K_END(size) \
  break;            \
  }

auto parse_identifier(std::string_view view) -> Classifier {
  switch (view.size()) {
    K_SECTION(2)
    static_assert(sizeof("if") - 1 == section_bytes);
    if (view.data()[0] == "if"[0] &&
        !std::memcmp(view.data(), "if", sizeof("if") - 1)) {
      return Classifier::If;
    }
    K_END()
    K_SECTION(3)
    static_assert(sizeof("for") - 1 == section_bytes);
    if (view.data()[0] == "for"[0] &&
        !std::memcmp(view.data(), "for", sizeof("for") - 1)) {
      return Classifier::For;
    }
    static_assert(sizeof("new") - 1 == section_bytes);
    if (view.data()[0] == "new"[0] &&
        !std::memcmp(view.data(), "new", sizeof("new") - 1)) {
      return Classifier::New;
    }
    K_END()
    K_SECTION(4)
    static_assert(sizeof("else") - 1 == section_bytes);
    if (view.data()[0] == "else"[0] &&
        !std::memcmp(view.data(), "else", sizeof("else") - 1)) {
      return Classifier::Else;
    }
    static_assert(sizeof("this") - 1 == section_bytes);
    if (view.data()[0] == "this"[0] &&
        !std::memcmp(view.data(), "this", sizeof("this") - 1)) {
      return Classifier::This;
    }
    static_assert(sizeof("true") - 1 == section_bytes);
    if (view.data()[0] == "true"[0] &&
        !std::memcmp(view.data(), "true", sizeof("true") - 1)) {
      return Classifier::True;
    }
    static_assert(sizeof("from") - 1 == section_bytes);
    if (view.data()[0] == "from"[0] &&
        !std::memcmp(view.data(), "from", sizeof("from") - 1)) {
      return Classifier::From;
    }
    static_assert(sizeof("init") - 1 == section_bytes);
    if (view.data()[0] == "init"[0] &&
        !std::memcmp(view.data(), "init", sizeof("init") - 1)) {
      return Classifier::Init;
    }
    static_assert(sizeof("func") - 1 == section_bytes);
    if (view.data()[0] == "func"[0] &&
        !std::memcmp(view.data(), "func", sizeof("func") - 1)) {
      return Classifier::FuncDef;
    }
    static_assert(sizeof("type") - 1 == section_bytes);
    if (view.data()[0] == "type"[0] &&
        !std::memcmp(view.data(), "type", sizeof("type") - 1)) {
      return Classifier::TypeDef;
    }
    K_END()
    K_SECTION(5)
    static_assert(sizeof("false") - 1 == section_bytes);
    if (view.data()[0] == "false"[0] &&
        !std::memcmp(view.data(), "false", sizeof("false") - 1)) {
      return Classifier::False;
    }
    static_assert(sizeof("debug") - 1 == section_bytes);
    if (view.data()[0] == "debug"[0] &&
        !std::memcmp(view.data(), "debug", sizeof("debug") - 1)) {
      return Classifier::Debug;
    }
    static_assert(sizeof("error") - 1 == section_bytes);
    if (view.data()[0] == "error"[0] &&
        !std::memcmp(view.data(), "error", sizeof("error") - 1)) {
      return Classifier::Error;
    }
    static_assert(sizeof("while") - 1 == section_bytes);
    if (view.data()[0] == "while"[0] &&
        !std::memcmp(view.data(), "while", sizeof("while") - 1)) {
      return Classifier::While;
    }
    K_END()
    K_SECTION(6)
    static_assert(sizeof("return") - 1 == section_bytes);
    if (view.data()[0] == "return"[0] &&
        !std::memcmp(view.data(), "return", sizeof("return") - 1)) {
      return Classifier::Return;
    }
    K_END()
    K_SECTION(7)
    static_assert(sizeof("package") - 1 == section_bytes);
    if (view.data()[0] == "package"[0] &&
        !std::memcmp(view.data(), "package", sizeof("package") - 1)) {
      return Classifier::Package;
    }
    K_END()
    K_SECTION(8)
    static_assert(sizeof("requires") - 1 == section_bytes);
    if (view.data()[0] == "requires"[0] &&
        !std::memcmp(view.data(), "requires", sizeof("requires") - 1)) {
      return Classifier::Requires;
    }
    K_END()
  }
  return Classifier::Identifier;
}

static void TARGET_handrolled_custom_parser(benchmark::State& state) {
  std::vector<std::string_view> values;
  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    values.push_back(KeywordTable::data[i].key);
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      uint32_t index = i % values.size();
      doNotOptimizeAway(parse_identifier(values[index]));
    }
  }
}
BENCHMARK(TARGET_handrolled_custom_parser);
