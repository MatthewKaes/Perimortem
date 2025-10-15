// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "parser/token.hpp"
#include "parser/ttx.hpp"

#include "concepts/narrow_resolver.hpp"
#include "concepts/sparse_index.hpp"
#include "concepts/sparse_lookup.hpp"

using namespace Perimortem::Concepts;
using namespace Tetrodotoxin::Language::Parser;

// Number of words to load
const int tests = 10000;

template <class T>
void doNotOptimizeAway(T&& datum) {
  asm volatile("" : "+r"(datum));
}

static const std::vector<std::string_view> test_data = {
    // Keywords
    "if",
    "for",
    "new",
    "via",
    "else",
    "func",
    "init",
    "true",
    "class",
    "debug",
    "error",
    "false",
    "while",
    "return",
    "on_load",
    "package",
    "warning",
    "requires",
    // Identifiers
    "value",
    "test",
    "temp",
    "i",
    "x",
    "classes",
    "function",
    "values",
    "what",
    "test2",
    "klass_name",
    "to_string",
    "my_variable",
    "special_required",
    "header",
    "magic_number",
    "caculate_value"
    "test_func",
    "fetch_keywrod",
    "print",
    "size",
    "color",
    "constant",

};

namespace Tetrodotoxin::Language::Parser::KeywordTable {
using value_type = Classifier;
constexpr auto sparse_factor = 150;
constexpr auto seed = 5793162292815199453UL;
constexpr TablePair<const char*, value_type> data[] = {
    make_pair("if", Classifier::If),
    make_pair("for", Classifier::For),
    make_pair("new", Classifier::New),
    make_pair("via", Classifier::Via),
    make_pair("else", Classifier::Else),
    make_pair("func", Classifier::FuncDef),
    make_pair("init", Classifier::Init),
    make_pair("true", Classifier::True),
    make_pair("class", Classifier::TypeDef),
    make_pair("while", Classifier::While),
    make_pair("false", Classifier::False),
    make_pair("error", Classifier::Error),
    make_pair("debug", Classifier::Debug),
    make_pair("return", Classifier::Return),
    make_pair("on_load", Classifier::OnLoad),
    make_pair("package", Classifier::Package),
    make_pair("warning", Classifier::Warning),
    make_pair("requires", Classifier::Requires),
};

// constexpr auto search_result =
//     SeedFinder<255, Classifier, array_size(data), data,
//     sparse_factor, seed >::candidate_search();

using lookup =
    SparseLookupTable<value_type, array_size(data), data, sparse_factor, seed>;

static_assert(lookup::has_perfect_hash());
static_assert(sizeof(lookup::sparse_table) <= 512,
              "Ideally keyword sparse table would be less than 512 bytes. "
              "Try to find a smaller size.");

}  // namespace Tetrodotoxin::Language::Parser::KeywordTable

static void BRUTE_FORCE_compare_table(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
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
  std::unordered_map<std::string_view, Classifier> data;
  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    data[KeywordTable::data[i].key] = KeywordTable::data[i].value;
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
      if (data.contains(entry)) {
        doNotOptimizeAway(data[entry]);
      }
    }
  }
}
BENCHMARK(BASELINE_unordered_map);

static void OPTIMIZED_STL_vector_table(benchmark::State& state) {
  std::array<std::vector<std::pair<std::string_view, Classifier>>, 8>
      vector_table;

  for (int i = 0; i < KeywordTable::lookup::item_count; i++) {
    uint32_t index =
        std::char_traits<char>::length(KeywordTable::data[i].key) - 1;
    vector_table[index].push_back(
        std::make_pair(std::string_view{KeywordTable::data[i].key, index + 1},
                       KeywordTable::data[i].value));
  }

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
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
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
      doNotOptimizeAway(collision_lookup::find_or_default(
          entry.data(), entry.size(), Classifier::None));
    }
  }
}
BENCHMARK(sparse_lookup_table_collision);

static void sparse_lookup_table_perfect_stack(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
      doNotOptimizeAway(KeywordTable::lookup::find_or_default(
          entry.data(), Classifier::None));
    }
  }
}
BENCHMARK(sparse_lookup_table_perfect_stack);

static void sparse_lookup_table_perfect_view(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
      doNotOptimizeAway(KeywordTable::lookup::find_or_default(
          entry.data(), entry.size(), Classifier::None));
    }
  }
}
BENCHMARK(sparse_lookup_table_perfect_view);

using index_lookup = SparseIndexTable<KeywordTable::value_type,
                                      array_size(KeywordTable::data),
                                      KeywordTable::data>;

static void sparse_index_table(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
      doNotOptimizeAway(
          index_lookup::find_or_default(entry, Classifier::None));
    }
  }
}
BENCHMARK(sparse_index_table);

namespace Tetrodotoxin::Language::Parser {
static inline constexpr auto check_keyword(std::string_view value, Classifier default_value)
    -> Classifier {
  static constexpr TablePair<std::string_view, Classifier> data[] = {
      {"if", Classifier::If},           {"for", Classifier::For},
      {"new", Classifier::New},         {"via", Classifier::Via},
      {"else", Classifier::Else},       {"func", Classifier::FuncDef},
      {"init", Classifier::Init},       {"true", Classifier::True},
      {"class", Classifier::TypeDef},   {"debug", Classifier::Debug},
      {"error", Classifier::Error},     {"false", Classifier::False},
      {"while", Classifier::While},     {"return", Classifier::Return},
      {"on_load", Classifier::OnLoad},  {"package", Classifier::Package},
      {"warning", Classifier::Warning}, {"requires", Classifier::Requires},
  };

  using keyword_table = NarrowResolver<Classifier, array_size(data), data>;
  static_assert(sizeof(keyword_table::sparse_table) <= 4800,
                "Keyword sparse table should be less than 4800 bytes. "
                "Use keywords only 8 characters or shorter.");

  return keyword_table::find_or_default(value, default_value);
}
}  // namespace Tetrodotoxin::Language::Parser

static void sparse_narrow_resolver(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
      doNotOptimizeAway(check_keyword(entry, Classifier::None));
    }
  }
}
BENCHMARK(sparse_narrow_resolver);

static void sparse_narrow_resolver_keywords_only(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % 18];
      doNotOptimizeAway(check_keyword(entry, Classifier::None));
    }
  }
}
BENCHMARK(sparse_narrow_resolver_keywords_only);

static void sparse_narrow_resolver_identifiers_only(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[(rand() % 23) + 17];
      doNotOptimizeAway(check_keyword(entry, Classifier::None));
    }
  }
}
BENCHMARK(sparse_narrow_resolver_identifiers_only);

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
    static_assert(sizeof("func") - 1 == section_bytes);
    if (view.data()[0] == "func"[0] &&
        !std::memcmp(view.data(), "func", sizeof("func") - 1)) {
      return Classifier::FuncDef;
    }
    static_assert(sizeof("init") - 1 == section_bytes);
    if (view.data()[0] == "init"[0] &&
        !std::memcmp(view.data(), "init", sizeof("init") - 1)) {
      return Classifier::Init;
    }
    static_assert(sizeof("true") - 1 == section_bytes);
    if (view.data()[0] == "true"[0] &&
        !std::memcmp(view.data(), "true", sizeof("true") - 1)) {
      return Classifier::True;
    }
    K_END()
    K_SECTION(5)
    static_assert(sizeof("class") - 1 == section_bytes);
    if (view.data()[0] == "type"[0] &&
        !std::memcmp(view.data(), "class", sizeof("class") - 1)) {
      return Classifier::TypeDef;
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
    static_assert(sizeof("false") - 1 == section_bytes);
    if (view.data()[0] == "false"[0] &&
        !std::memcmp(view.data(), "false", sizeof("false") - 1)) {
      return Classifier::False;
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
    static_assert(sizeof("on_load") - 1 == section_bytes);
    if (view.data()[0] == "on_load"[0] &&
        !std::memcmp(view.data(), "on_load", sizeof("on_load") - 1)) {
      return Classifier::OnLoad;
    }
    static_assert(sizeof("package") - 1 == section_bytes);
    if (view.data()[0] == "package"[0] &&
        !std::memcmp(view.data(), "package", sizeof("package") - 1)) {
      return Classifier::Package;
    }
    static_assert(sizeof("warning") - 1 == section_bytes);
    if (view.data()[0] == "warning"[0] &&
        !std::memcmp(view.data(), "warning", sizeof("warning") - 1)) {
      return Classifier::Warning;
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
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % test_data.size()];
      doNotOptimizeAway(parse_identifier(entry));
    }
  }
}
BENCHMARK(TARGET_handrolled_custom_parser);


static void TARGET_handrolled_custom_parser_keywords_only(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[rand() % 18];
      doNotOptimizeAway(parse_identifier(entry));
    }
  }
}
BENCHMARK(TARGET_handrolled_custom_parser_keywords_only);


static void TARGET_handrolled_custom_parser_identifiers_only(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto entry = test_data[(rand() % 23) + 17];
      doNotOptimizeAway(parse_identifier(entry));
    }
  }
}
BENCHMARK(TARGET_handrolled_custom_parser_identifiers_only);