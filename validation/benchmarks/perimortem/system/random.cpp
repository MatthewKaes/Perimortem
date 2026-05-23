// Perimortem Engine
// Copyright © Matt Kaes

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
  for (Count batch_idx = 0; batch_idx < random_batch; batch_idx++) {
    accumulator ^= Random::generate();
  }
  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(SystemRandom, read_entropy) {
  // Single OS entropy read to measure the syscall overhead vs Philox.
  Bits_64 accumulator = 0;
  for (Count batch_idx = 0; batch_idx < random_batch; batch_idx++) {
    accumulator ^= Random::read_entropy();
  }
  Benchmark::prevent_optimization(accumulator);
}
