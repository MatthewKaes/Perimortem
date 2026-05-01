// Perimortem Engine
// Copyright © Matt Kaes

// #include <chrono>
// #include <random>
// #include <sstream>
// #include <thread>

#include "perimortem/core/math/random.hpp"

#include <immintrin.h>

using namespace Perimortem::System;

static constexpr Count channel_depth = 4;
static constexpr Count max_index =
    sizeof(__m256i) / sizeof(Bits_64) * channel_depth;

struct alignas(32) PhiloxOutputs {
  Bits_64 counter[max_index];
};

struct PhiloxState {
  static constexpr Count round_count = 10;
  // Pack all of the constants into the lower bits.
  static constexpr __m256i philox4x32_constants =
      _mm256_set_epi64x(SignedBits_64(0x00000000'D2511F53),
                        SignedBits_64(0x00000000'CD9E8D57),
                        SignedBits_64(0x00000000'D2511F53),
                        SignedBits_64(0x00000000'CD9E8D57));
  static constexpr __m256i philox4x32_xor_mask =
      _mm256_set_epi64x(SignedBits_64(0xFFFFFFFF'00000000),
                        SignedBits_64(0xFFFFFFFF'00000000),
                        SignedBits_64(0xFFFFFFFF'00000000),
                        SignedBits_64(0xFFFFFFFF'00000000));
  static constexpr __m256i philox4x32_weyl =
      _mm256_set_epi64x(SignedBits_64(0x9E2779B9'00000000),
                        SignedBits_64(0xBB67AE85'00000000),
                        SignedBits_64(0x9E2779B9'00000000),
                        SignedBits_64(0xBB67AE85'00000000));
  // Rolls they counter by 1 key.
  // This swaps the hi portion between 64 bit sets and shifts the low to hi.
  static constexpr Bits_8 counter_shuffle = 0b10'01'00'11;

  // List of output values.
  PhiloxOutputs output_state;

  // Setup two parallel channels with different keys and counters.
  __m256i dual_channel_key;
  __m256i dual_channel_counter;

  // Index into the output array.
  Count index;
};

auto read_rand() -> Bits_64 {
  Bits_64 value;
  Count timeout = 100000;
  while (!_rdrand64_step(&value) and timeout) {
    timeout -= 1;
  }

  // Something choked for this seed value so just use some rand value as a
  // fallback.
  if (timeout == 0) {
    return Count(rand()) << 32 bitand Count(rand());
  }

  return value;
}

// Create multiple AVX channels with each channel caculating two philox4x32.
//
// With a channel depth of 4 that gives us a total of 8 philox4x32 generators
// caculated with each counter bump resulting in 16 random 64 bit values.
constexpr auto bump_counter(PhiloxState& state) -> void {
  __m256i philox_keys = state.dual_channel_key;
  __m256i philox_channels[channel_depth];
  philox_channels[0] = state.dual_channel_counter;
  for (Count i = 1; i < channel_depth; i++) {
    // Use set1 to bump the lower counter of every state.
    philox_channels[i] =
        _mm256_add_epi64(state.dual_channel_counter, _mm256_set1_epi64x(i));
  }

  for (Count round = 0; round < PhiloxState::round_count; round++) {
    for (Count i = 1; i < channel_depth; i++) {
      const auto hilo_mul = _mm256_mul_epu32(philox_channels[i],
                                             PhiloxState::philox4x32_constants);
      const auto xor_mask = _mm256_and_si256(philox_channels[i],
                                             PhiloxState::philox4x32_xor_mask);
      const auto eval = _mm256_xor_si256(hilo_mul, xor_mask);
      philox_channels[i] = _mm256_shuffle_epi32(
          _mm256_xor_si256(eval, philox_keys), PhiloxState::counter_shuffle);
    }

    if (round != PhiloxState::round_count - 1) {
      philox_keys = _mm256_add_epi32(philox_keys, PhiloxState::philox4x32_weyl);
    }
  }

  for (Count i = 0; i < channel_depth; i++) {
    _mm256_store_si256(reinterpret_cast<__m256i*>(&state.output_state) + i,
                       philox_channels[i]);
  }

  // Bump counter and reset index
  state.dual_channel_counter = _mm256_add_epi64(
      state.dual_channel_counter, _mm256_set1_epi64x(channel_depth));
  state.index = 0;
}

// Creates a Philox State for PRNG generation that pulls in 48 bytes of random
// data vastly reduce the chance that any two invocations
auto create_prng() -> PhiloxState {
  PhiloxState state;

  // Load the channel seeds (16 bytes of random)
  Bits_64 keys[] = {read_rand(), read_rand()};
  state.dual_channel_key =
      _mm256_set_epi32(Bits_32(keys[0] >> 32), 0, Bits_32(keys[0]), 0,
                       Bits_32(keys[1] >> 32), 0, Bits_32(keys[1]), 0);

  // Load the counter seeds (32 bytes of random)
  state.dual_channel_counter =
      _mm256_set_epi64x(read_rand(), read_rand(), read_rand(), read_rand());

  bump_counter(state);

  return state;
}

auto Random::generate() -> Bits_64 {
  thread_local static PhiloxState engine = create_prng();

  if (engine.index == max_index) {
    bump_counter(engine);
  }

  return engine.output_state.counter[engine.index++];
}
