// Perimortem Engine
// Copyright © Matt Kaes

#ifdef PERI_BENCH_CPP
#include <algorithm>
#endif

#include "validation/benchmark.hpp"

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Validation;

namespace Perimortem::Core::View {
auto operator>(const Bytes& lhs, const Bytes& rhs) -> Bool {
  if (lhs.get_size() != rhs.get_size()) {
    return lhs.get_size() > rhs.get_size();
  }

  auto data = lhs.get_data();
  for (Count i = 0; i < lhs.get_size(); i++) {
    if (data[i] != rhs.get_data()[i]) {
      return data[i] > rhs.get_data()[i];
    }
  }

  return false;
}
}  // namespace Perimortem::Core::View

static Static::Vector<Count, 1 << 6> count_64;
static Static::Vector<Count, 1 << 9> count_512;
static Static::Vector<Count, 1 << 12> count_4k;
static Static::Vector<Count, 1 << 14> count_16k;

// Make sure keys are randomized
template <typename count_array>
auto fill_random(count_array& target) -> void {
  Count* data = target.get_data();
  for (Count i = 0; i < target.get_size(); i++) {
    data[i] = Random::generate();
  }
}

static Harness Sort64Int = {
  .name = "Sorting"_view,
  .setup = []() { fill_random(count_64); },
};

PERIMORTEM_BENCHMARK(Sort64Int, int_64_items) {
  Algorithm::sort(count_64.get_access());
  Benchmark::prevent_optimization(count_64[0]);
}

static Harness Sort512Int = {
  .name = "Sorting"_view,
  .setup = []() { fill_random(count_512); },
};

PERIMORTEM_BENCHMARK(Sort512Int, int_512_items) {
  Algorithm::sort(count_512.get_access());
  Benchmark::prevent_optimization(count_512[0]);
}

static Harness Sort4kInt = {
  .name = "Sorting"_view,
  .setup = []() { fill_random(count_4k); },
};

PERIMORTEM_BENCHMARK(Sort4kInt, int_4k_items) {
  Algorithm::sort(count_4k.get_access());
  Benchmark::prevent_optimization(count_4k[0]);
}

static Harness Sort16kInt = {
  .name = "Sorting"_view,
  .setup = []() { fill_random(count_16k); },
};

PERIMORTEM_BENCHMARK(Sort16kInt, int_16k_items) {
  Algorithm::sort(count_16k.get_access());
  Benchmark::prevent_optimization(count_16k[0]);
}

static constexpr Static::Vector<View::Bytes, 24> keyword_seeds = {{
  "as"_view,      "if"_view,      "for"_view,     "new"_view,     "via"_view,
  "else"_view,    "func"_view,    "init"_view,    "self"_view,    "true"_view,
  "alias"_view,   "error"_view,   "debug"_view,   "false"_view,   "using"_view,
  "while"_view,   "entity"_view,  "object"_view,  "return"_view,  "struct"_view,
  "library"_view, "on_load"_view, "package"_view, "warning"_view,
}};
static constexpr Count permute_count = 16;
static constexpr Count word_width = 12;

static Static::Bytes<keyword_seeds.get_size() * word_width * permute_count>
    generated_strings;
static Static::Vector<View::Bytes, keyword_seeds.get_size() * permute_count>
    word_pool;

// Generates a larger batch of test words off of a set of source seed
// words by appending "_xx" to each one and writing it into a buffer.
//
// Stores state so it only runs the generation once regardless of how many
// string sorting tests we have.
auto populate_words() -> void {
  static Bool populated = False;
  if (populated) {
    return;
  }

  auto generated_index = 0;
  for (Count i = 0; i < keyword_seeds.get_size(); i++) {
    View::Bytes seed_word = keyword_seeds[i];
    for (Count j = 0; j < permute_count; j++) {
      auto slot = generated_strings.get_data() + generated_index;
      Data::copy(slot, seed_word.get_data(), seed_word.get_size());

      slot[seed_word.get_size()] = '_';
      slot[seed_word.get_size() + 1] = Bits_8('0' + (i / 100) % 10);
      slot[seed_word.get_size() + 2] = Bits_8('0' + (i / 10) % 10);

      Count generated_size = seed_word.get_size() + 3;
      word_pool[i] = View::Bytes(slot, generated_size);
      generated_index += generated_size;
    }
  }

  populated = True;
}

// Make sure keys are randomized
template <typename word_array>
auto fill_words(word_array& target) -> void {
  // Randomly selects 4 Byte::View chunks to fill the array with.
  for (Count i = 0; i < target.get_size() / 4; i += 4) {
    Bits_64 random_indexes = Random::generate();
    target[i] = word_pool[Bits_16(random_indexes) % word_pool.get_size()];
    target[i + 1] =
        word_pool[Bits_16(random_indexes >> 16) % word_pool.get_size()];
    target[i + 2] =
        word_pool[Bits_16(random_indexes >> 32) % word_pool.get_size()];
    target[i + 3] =
        word_pool[Bits_16(random_indexes >> 48) % word_pool.get_size()];
  }
}

static Static::Vector<View::Bytes, 1 << 8> words_256;
static Static::Vector<View::Bytes, 1 << 12> words_4k;
static Static::Vector<View::Bytes, 1 << 14> words_16k;

static Harness SortStrings256 = {
  .name = "Sorting"_view,
  .init = populate_words,
  .setup = []() { fill_words(words_256); },
};

PERIMORTEM_BENCHMARK(SortStrings256, views_256) {
  Algorithm::sort(words_256.get_access());
  Count sorted_str_size = words_256[0].get_size();
  Benchmark::prevent_optimization(sorted_str_size);
}

static Harness SortStrings4k = {
  .name = "Sorting"_view,
  .init = populate_words,
  .setup = []() { fill_words(words_4k); },
};

PERIMORTEM_BENCHMARK(SortStrings4k, views_4k) {
  Algorithm::sort(words_4k.get_access());
  Count sorted_str_size = words_4k[0].get_size();
  Benchmark::prevent_optimization(sorted_str_size);
}

static Harness SortStrings16k = {
  .name = "Sorting"_view,
  .init = populate_words,
  .setup = []() { fill_words(words_16k); },
};

PERIMORTEM_BENCHMARK(SortStrings16k, views_16k) {
  Algorithm::sort(words_16k.get_access());
  Count sorted_str_size = words_16k[0].get_size();
  Benchmark::prevent_optimization(sorted_str_size);
}

#ifdef PERI_BENCH_CPP

// All integer sort harnesses share the name "Sorting" so using that name here
// lets find_stored_ns locate their benchmark stats during comparison lookup.
static Harness SortIntComp = {
  .name = "Sorting"_view,
  .setup =
      []() {
        fill_random(count_64);
        fill_random(count_512);
        fill_random(count_4k);
        fill_random(count_16k);
      },
};

static Harness SortStringComp = {
  .name = "Sorting"_view,
  .init = populate_words,
  .setup =
      []() {
        fill_words(words_256);
        fill_words(words_4k);
        fill_words(words_16k);
      },
};

template <typename array_type>
auto cpp_std_sort_ints(array_type& arr) -> void {
  std::sort(arr.get_data(), arr.get_data() + arr.get_size());
  Benchmark::prevent_optimization(arr[0]);
}

template <typename array_type>
auto cpp_std_sort_views(array_type& arr) -> void {
  std::sort(
      arr.get_data(), arr.get_data() + arr.get_size(),
      [](const View::Bytes& a, const View::Bytes& b) { return b > a; });
  Count sorted_size = arr[0].get_size();
  Benchmark::prevent_optimization(sorted_size);
}

#define INT_SORT_COMPARISON(size, buffer)                          \
  static Benchmark::Comparison sort_##size##_ints_comp = {         \
    .harness = &SortIntComp,                                       \
    .label = "ints " #size ""_view,                                \
    .variants = {{"perimortem"_view, "int_" #size "_items"_view}}, \
  };                                                               \
  PERIMORTEM_COMPARISON(sort_##size##_ints_comp) {                 \
    cpp_std_sort_ints(buffer);                                     \
  }

INT_SORT_COMPARISON(64, count_64)
INT_SORT_COMPARISON(512, count_512)
INT_SORT_COMPARISON(4k, count_4k)
INT_SORT_COMPARISON(16k, count_16k)

#define STRING_SORT_COMPARISON(size, buffer)                   \
  static Benchmark::Comparison sort_##size##_strings_comp = {  \
    .harness = &SortStringComp,                                \
    .label = "strings " #size ""_view,                         \
    .variants = {{"perimortem"_view, "views_" #size ""_view}}, \
  };                                                           \
  PERIMORTEM_COMPARISON(sort_##size##_strings_comp) {          \
    cpp_std_sort_views(buffer);                                \
  }

STRING_SORT_COMPARISON(256, words_256)
STRING_SORT_COMPARISON(4k, words_4k)
STRING_SORT_COMPARISON(16k, words_16k)

#endif  // PERI_BENCH_CPP
