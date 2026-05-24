// Perimortem Engine
// Copyright © Matt Kaes

#ifdef PERI_BENCH_CPP
#include <string>
#endif

#include "validation/benchmark.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

constexpr auto batch_count = 1024;
static Harness StringBench = {
  .name = "Strings"_view,
};

template <Count byte_length>
auto concat_test() -> void {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    Dynamic::Bytes buffer;
    buffer.concat(Static::Bytes<byte_length>());
    size += buffer.get_size();
  }
  Benchmark::prevent_optimization(size);
}

#define CONCAT_TEST(size)                            \
  PERIMORTEM_BENCHMARK(StringBench, concat_##size) { \
    concat_test<size>();                             \
  }

CONCAT_TEST(8);
CONCAT_TEST(16);
CONCAT_TEST(32);
CONCAT_TEST(64);
CONCAT_TEST(128);

PERIMORTEM_BENCHMARK(StringBench, append_bytes) {
  Dynamic::Bytes buffer;
  constexpr auto append_count = 1 << 10;
  for (Count i = 0; i < append_count; i++) {
    buffer.append('0');
  }
  Count size = buffer.get_size();
  Benchmark::prevent_optimization(size);
  buffer.reset();
}

PERIMORTEM_BENCHMARK(StringBench, small_string) {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    Dynamic::Bytes buffer = "small string"_view;
    size += buffer.get_size();
  }
  Benchmark::prevent_optimization(size);
}

PERIMORTEM_BENCHMARK(StringBench, small_concat) {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    Dynamic::Bytes buffer = "small"_view;
    buffer.concat(" string"_view);
    size += buffer.get_size();
  }
  Benchmark::prevent_optimization(size);
}

#ifdef PERI_BENCH_CPP

template <Count byte_length>
static auto cpp_string_concat() -> void {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    std::string buffer;
    buffer.append(byte_length, '\0');
    Benchmark::prevent_optimization(data);
    size += Count(buffer.data());
  }
  Benchmark::prevent_optimization(size);
}

static auto cpp_string_append_bytes() -> void {
  std::string buffer;
  constexpr auto append_count = 1 << 10;
  for (Count i = 0; i < append_count; i++) {
    buffer += '0';
  }
  Count size = Count(buffer.size());
  Benchmark::prevent_optimization(size);
}

static auto cpp_string_small_string() -> void {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    std::string buffer = "small string";
    auto data = buffer.data();
    Benchmark::prevent_optimization(data);
    size += Count(buffer.size());
  }
  Benchmark::prevent_optimization(size);
}

static auto cpp_string_small_concat() -> void {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    std::string buffer = "small";
    buffer += " string";
    auto data = buffer.data();
    Benchmark::prevent_optimization(data);
    size += Count(buffer.size());
  }
  Benchmark::prevent_optimization(size);
}

#define CONCAT_COMPARISON(size)                              \
  static Benchmark::Comparison concat_##size##_comp = {      \
    .harness = &StringBench,                                 \
    .label = "concat " #size ""_view,                        \
    .variants = {{"dynamic"_view, "concat_" #size ""_view}}, \
  };                                                         \
  PERIMORTEM_COMPARISON(concat_##size##_comp) {              \
    cpp_string_concat<size>();                               \
  }

CONCAT_COMPARISON(8)
CONCAT_COMPARISON(16)
CONCAT_COMPARISON(32)
CONCAT_COMPARISON(64)
CONCAT_COMPARISON(128)

static Benchmark::Comparison append_bytes_comp = {
  .harness = &StringBench,
  .label = "append bytes"_view,
  .variants = {{"dynamic"_view, "append_bytes"_view}},
};
PERIMORTEM_COMPARISON(append_bytes_comp) {
  cpp_string_append_bytes();
}

static Benchmark::Comparison small_string_comp = {
  .harness = &StringBench,
  .label = "small string"_view,
  .variants = {{"dynamic"_view, "small_string"_view}},
};
PERIMORTEM_COMPARISON(small_string_comp) {
  cpp_string_small_string();
}

static Benchmark::Comparison small_concat_comp = {
  .harness = &StringBench,
  .label = "small concat"_view,
  .variants = {{"dynamic"_view, "small_concat"_view}},
};
PERIMORTEM_COMPARISON(small_concat_comp) {
  cpp_string_small_concat();
}

#endif  // PERI_BENCH_CPP
