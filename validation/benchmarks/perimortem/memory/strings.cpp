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
static Harness DynamicBytes = {.name = "Strings"_view};

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

PERIMORTEM_BENCHMARK(DynamicBytes, concat_8) {
  concat_test<8>();
}

PERIMORTEM_BENCHMARK(DynamicBytes, concat_16) {
  concat_test<16>();
}

PERIMORTEM_BENCHMARK(DynamicBytes, concat_32) {
  concat_test<32>();
}

PERIMORTEM_BENCHMARK(DynamicBytes, concat_64) {
  concat_test<64>();
}

PERIMORTEM_BENCHMARK(DynamicBytes, concat_128) {
  concat_test<128>();
}

PERIMORTEM_BENCHMARK(DynamicBytes, append_bytes) {
  Dynamic::Bytes buffer;
  constexpr auto append_count = 1 << 10;
  for (Count i = 0; i < append_count; i++) {
    buffer.append('0');
  }
  Count size = buffer.get_size();
  Benchmark::prevent_optimization(size);
  buffer.reset();
}

PERIMORTEM_BENCHMARK(DynamicBytes, small_string) {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    Dynamic::Bytes buffer = "small string"_view;
    size += buffer.get_size();
  }
  Benchmark::prevent_optimization(size);
}

PERIMORTEM_BENCHMARK(DynamicBytes, small_concat) {
  Count size = 0;
  for (Count i = 0; i < batch_count; i++) {
    Dynamic::Bytes buffer = "small"_view;
    buffer.concat(" string"_view);
    size += buffer.get_size();
  }
  Benchmark::prevent_optimization(size);
}
