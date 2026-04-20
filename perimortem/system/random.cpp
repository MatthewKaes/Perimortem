// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/system/random.hpp"
#include "perimortem/memory/static/hash.hpp"

#include <immintrin.h>

#include <chrono>
#include <random>
#include <sstream>
#include <thread>

using namespace Perimortem::System;
using namespace Perimortem::Memory;

Bits_64 base_seed =
    0b01011011'10110110'11111000'01010111'00010111'01000111'11101000'00111100ULL;

// TODO: Replace the engine with std::philox4x64 when it becomes available.
using random_engine = std::mt19937_64;

// A PRNG generator that pulls in a few sources of entropy over random_device
// to provide better overall seeding.
//
// Should be updated to a proper system per OS but is in general mostly
// portable for x86_64 systems.
auto create_prng() -> random_engine {
  constexpr auto seed_count = 2;
  Bits_64 seed_values[seed_count];

  // Get the random device, which may be deterministic.
  Bool rdrand_success = true;
  for (Count i = 0; i < seed_count; i++) {
    rdrand_success &=
        _rdrand64_step(reinterpret_cast<unsigned long long*>(seed_values + i));
  }

  // Catch microcode issues generating "valid" strings of the same value but
  // reporting successfull generation. e.g. The Zen2 microcode bug.
  //
  // On a very rare occassion this could cause us to fail a valid generation
  // that legitimately created the same key twice.
  if (rdrand_success && seed_values[0] == seed_values[1]) {
    rdrand_success = false;
  }

  // rdrand could fail for a number of reasons so if it fails we'll fall back to
  // a std::random_device.
  if (!rdrand_success) {
    std::random_device rd;

    for (Count i = 0; i < seed_count; i++) {
      seed_values[i] = rd();
    }
  }

  // Contrived thread data to spread out thread generation.
  std::stringstream id_stream;
  id_stream << std::this_thread::get_id();
  auto thread_value = id_stream.str();
  auto thread_hash = Static::Hash(
      View::Bytes(reinterpret_cast<const Byte*>(thread_value.c_str()),
                  thread_value.size()));

  // Contrived time data to spread out over various runs.
  auto time = std::chrono::system_clock::now();
  auto time_hash = Static::Hash(time.time_since_epoch().count());

  std::seed_seq sd{
      seed_values[0 % seed_count] ^ static_cast<Int>(base_seed),
      seed_values[1 % seed_count] ^ static_cast<Int>(base_seed >> 32),
      seed_values[2 % seed_count] ^
          static_cast<Int>(thread_hash.get_value() >> 32),
      seed_values[3 % seed_count] ^ static_cast<Int>(thread_hash.get_value()),
      seed_values[4 % seed_count] ^
          static_cast<Int>(time_hash.get_value() >> 32),
      seed_values[5 % seed_count] ^ static_cast<Int>(time_hash.get_value())};

  // Cascade to the base seed so no two threads start with the same base.
  base_seed = time_hash.Rehash(thread_hash.Rehash(base_seed));
  return random_engine(sd);
}

auto Random::generate() -> Bits_64 {
  thread_local static random_engine engine = create_prng();
  return engine();
}
