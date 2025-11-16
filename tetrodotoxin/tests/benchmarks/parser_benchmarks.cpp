// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "lexical/token.hpp"

#include "concepts/narrow_resolver.hpp"
#include "concepts/sparse_index.hpp"
#include "concepts/sparse_lookup.hpp"

using namespace Perimortem::Concepts;
using namespace Tetrodotoxin::Lexical;

// Number of words to load
const int tests = 10000;

template <class T>
void doNotOptimizeAway(T&& datum) {
  asm volatile("" : "+r"(datum));
}

static const std::vector<std::string_view> test_data = {
    // Keywords
    "as",
    "if",
    "for",
    "new",
    "else",
    "func",
    "init",
    "self",
    "true",
    "alis",
    "debug",
    "error",
    "false",
    "using",
    "while",
    "entity",
    "object",
    "return",
    "struct",
    "library",
    "on_load",
    "package",
    "warning",
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

namespace Tetrodotoxin::Lexical::KeywordTable {
using value_type = Classifier;
constexpr auto sparse_factor = 150;
constexpr auto seed = 5793162292815203644UL;
constexpr TablePair<const char*, value_type> data[] = {
    {"as", Classifier::As},           {"if", Classifier::If},
    {"for", Classifier::For},         {"new", Classifier::New},
    {"else", Classifier::Else},       {"func", Classifier::Func},
    {"init", Classifier::Init},       {"self", Classifier::Self},
    {"true", Classifier::True},       {"alis", Classifier::Alias},
    {"debug", Classifier::Debug},     {"error", Classifier::Error},
    {"false", Classifier::False},     {"using", Classifier::Using},
    {"while", Classifier::While},     {"entity", Classifier::Entity},
    {"object", Classifier::Object},   {"return", Classifier::Return},
    {"struct", Classifier::Struct},   {"library", Classifier::Library},
    {"on_load", Classifier::OnLoad},  {"package", Classifier::Package},
    {"warning", Classifier::Warning},
};

// constexpr auto search_result =
//     SeedFinder<255, Classifier, array_size(data), data,
//     sparse_factor, seed >::candidate_search();

using lookup =
    SparseLookupTable<value_type, array_size(data), data, sparse_factor, seed>;

static_assert(lookup::has_perfect_hash());
static_assert(sizeof(lookup::sparse_table) <= 600,
              "Ideally keyword sparse table would be less than 600 bytes. "
              "Try to find a smaller size.");

}  // namespace Tetrodotoxin::Lexical::KeywordTable

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
      doNotOptimizeAway(index_lookup::find_or_default(entry, Classifier::None));
    }
  }
}
BENCHMARK(sparse_index_table);

namespace Tetrodotoxin::Lexical {
static inline constexpr auto check_keyword(std::string_view value,
                                           Classifier default_value)
    -> Classifier {
  static constexpr TablePair<std::string_view, Classifier> data[] = {
      {"as", Classifier::As},           {"if", Classifier::If},
      {"for", Classifier::For},         {"new", Classifier::New},
      {"else", Classifier::Else},       {"func", Classifier::Func},
      {"init", Classifier::Init},       {"self", Classifier::Self},
      {"true", Classifier::True},       {"alis", Classifier::Alias},
      {"debug", Classifier::Debug},     {"error", Classifier::Error},
      {"false", Classifier::False},     {"using", Classifier::Using},
      {"while", Classifier::While},     {"entity", Classifier::Entity},
      {"object", Classifier::Object},   {"return", Classifier::Return},
      {"struct", Classifier::Struct},   {"library", Classifier::Library},
      {"on_load", Classifier::OnLoad},  {"package", Classifier::Package},
      {"warning", Classifier::Warning},
  };

  using keyword_table = NarrowResolver<Classifier, array_size(data), data>;
  static_assert(sizeof(keyword_table::sparse_table) <= 4800,
                "Keyword sparse table should be less than 4800 bytes. "
                "Use keywords only 8 characters or shorter.");

  return keyword_table::find_or_default(value, default_value);
}
}  // namespace Tetrodotoxin::Lexical

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
