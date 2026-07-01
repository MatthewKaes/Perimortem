// Perimortem Engine
// Copyright © Matt Kaes

#ifdef PERI_BENCH_CPP
#include <random>
#endif

#include "validation/benchmark.hpp"

#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem::System;
using namespace Validation;

// Large batch to amortise the clock_gettime overhead — Philox is extremely fast
// so a single call produces results in under a nanosecond.
static constexpr Count random_batch = 1024;

static Harness SystemRandom = {
  .name = "Random"_view,
};

PERIMORTEM_BENCHMARK(SystemRandom, generate_1024) {
  // Throughput benchmark: 1024 Philox-derived 64-bit values.
  Bits_64 accumulator = 0;
  for (Count batch_index = 0; batch_index < random_batch; batch_index++) {
    accumulator ^= Random::generate();
  }
  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(SystemRandom, read_entropy) {
  // Single OS entropy read to measure the syscall overhead vs Philox.
  Bits_64 accumulator = 0;
  for (Count batch_index = 0; batch_index < random_batch; batch_index++) {
    accumulator ^= Random::read_entropy();
  }
  Benchmark::prevent_optimization(accumulator);
}

#ifdef PERI_BENCH_CPP

auto cpp_mt19937_generate() -> void {
  thread_local static std::mt19937_64 rng(std::random_device{}());
  Bits_64 accumulator = 0;
  for (Count batch_index = 0; batch_index < random_batch; batch_index++) {
    accumulator ^= Bits_64(rng());
  }
  Benchmark::prevent_optimization(accumulator);
}

auto cpp_random_device_read() -> void {
  // std::random_device only returns 32 bits at a time so we call it twice per
  // iteration to match the entropy we generate from Philox. Over the 1024 calls
  // we still get the same relative throughput measurement.
  static std::random_device rd;
  Bits_64 accumulator = 0;
  for (Count batch_index = 0; batch_index < random_batch; batch_index++) {
    accumulator ^= Bits_64(rd()) << 32 | Bits_64(rd());
  }
  Benchmark::prevent_optimization(accumulator);
}

static Benchmark::Comparison random_generate_comp = {
  .harness = &SystemRandom,
  .label = "generate 1024"_view,
  .variants = {{"philox"_view, "generate_1024"_view}},
};
PERIMORTEM_COMPARISON(random_generate_comp) {
  cpp_mt19937_generate();
}

static Benchmark::Comparison random_entropy_comp = {
  .harness = &SystemRandom,
  .label = "entropy 1024"_view,
  .variants = {{"rdrand"_view, "read_entropy"_view}},
};
PERIMORTEM_COMPARISON(random_entropy_comp) {
  cpp_random_device_read();
}

#endif  // PERI_BENCH_CPP
