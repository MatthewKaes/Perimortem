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
static constexpr Count batch_count = 1024;

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

static Harness HashBench = {
  .name = "Hashing"_view,
  .setup = []() { fill_hash(hash_buffer); },
  .batch_count = batch_count,
};

// Create a data dependency which disables Clang SIMD optimization so we can get
// a feel for scalar performance since in real use hashes tend to be performed
// as part of a hot path and it's not typical to vectorize over a range of a
// thousand keys in one go.
PERIMORTEM_BENCHMARK(HashBench, bits_32) {
  Bits_32 input = Data::cast<Bits_32>(hash_buffer.get_data())[0];
  Bits_64 accumulator = 0;
  for (Count i = 0; i < batch_count; i++) {
    Bits_64 result = Hash(input).get_value();
    accumulator ^= result;
    input = Bits_32(result);
  }
  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(HashBench, bits_64) {
  Bits_64 input = Data::cast<Bits_64>(hash_buffer.get_data())[0];
  Bits_64 accumulator = 0;
  for (Count i = 0; i < batch_count; i++) {
    Bits_64 result = Hash(input).get_value();
    accumulator ^= result;
    input = result;
  }
  Benchmark::prevent_optimization(accumulator);
}

template <Count hash_length>
auto compute_hash() -> void {
  // Slide the window by one byte per iteration so the optimizer cannot prove
  // all calls return the same value and fold the XOR chain to zero.
  constexpr Count max_offset = 8;
  Bits_64 accumulator = 0;
  for (Count i = 0; i < batch_count; i++) {
    Count offset = (max_offset > 0) ? (i % (max_offset + 1)) : 0;
    accumulator ^= Hash(hash_buffer.slice(offset, hash_length)).get_value();
  }
  Benchmark::prevent_optimization(accumulator);
}

#define HASH_BENCH(key_length)                               \
  PERIMORTEM_BENCHMARK(HashBench, key_length_##key_length) { \
    compute_hash<key_length>();                              \
  }

HASH_BENCH(64);
HASH_BENCH(128);
HASH_BENCH(256);
HASH_BENCH(512);
HASH_BENCH(550);
HASH_BENCH(600);
HASH_BENCH(2048);
HASH_BENCH(4096);
HASH_BENCH(8192);
