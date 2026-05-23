// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/hash.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

// The number of hashes to perform per benchmark run.
static constexpr Count hash_batch_count = 1024;

// Pre-built byte buffers for each size class.
static Static::Bytes<8192> hash_buffer;

// Make sure keys are randomized
template <typename byte_array>
auto fill_hash(byte_array& target) -> void {
  Count* data = Data::cast<Count>(target.get_data());
  for (Count i = 0; i < target.get_size() / 8; i++) {
    data[i] = Random::generate();
  }
}

static Harness HashFixedSize = {
  .name = "Hashing"_view,
};

PERIMORTEM_BENCHMARK(HashFixedSize, bits_32) {
  Bits_64 accumulator = 0;
  for (Count i = 0; i < hash_batch_count; i++) {
    accumulator ^= Hash(Bits_32(i)).get_value();
  }
  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(HashFixedSize, bits_64) {
  Bits_64 accumulator = 0;
  for (Count i = 0; i < hash_batch_count; i++) {
    accumulator ^= Hash(Bits_64(i)).get_value();
  }
  Benchmark::prevent_optimization(accumulator);
}

static Harness HashBytes = {
  .name = "Hashing"_view,
  .setup = []() { fill_hash(hash_buffer); },
};

template <Count hash_length>
auto compute_hash() -> Count {
  // Slide the window by one byte per iteration so the optimizer cannot prove
  // all calls return the same value and fold the XOR chain to zero.
  constexpr Count max_offset = hash_buffer.get_size() - hash_length;
  Bits_64 accumulator = 0;
  for (Count i = 0; i < hash_batch_count; i++) {
    Count offset = (max_offset > 0) ? (i % (max_offset + 1)) : 0;
    accumulator ^= Hash(hash_buffer.slice(offset, hash_length)).get_value();
  }
  return accumulator;
}

#define HASH_BENCH(key_length)                          \
  PERIMORTEM_BENCHMARK(HashBytes, bytes_##key_length) { \
    Bits_64 accumulator = compute_hash<key_length>();   \
    Benchmark::prevent_optimization(accumulator);       \
  }

HASH_BENCH(8);
HASH_BENCH(16);
HASH_BENCH(32);
HASH_BENCH(64);
HASH_BENCH(128);
HASH_BENCH(256);
HASH_BENCH(512);
HASH_BENCH(550);
HASH_BENCH(600);
HASH_BENCH(2048);
HASH_BENCH(4096);
HASH_BENCH(8192);
