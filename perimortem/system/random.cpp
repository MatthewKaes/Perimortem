// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/system/random.hpp"
#include "perimortem/memory/static/hash.hpp"

#include <chrono>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>

using namespace Perimortem::System;
using namespace Perimortem::Memory;

std::mutex creation_mutex;
Bits_64 base_seed =
    0b01011011'10110110'11111000'01010111'00010111'01000111'11101000'00111100ULL;

// TODO: A silly method that pulls in a few sources of entropy to help
// random_device perform slightly better. Should be updated to a proper
// system per OS.
auto create_prng() -> std::mt19937_64 {
  std::scoped_lock lock(creation_mutex);

  // Get the random device, which may be deterministic.
  std::random_device rd;

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

  std::seed_seq sd{rd() ^ static_cast<Int>(base_seed),
                   rd() ^ static_cast<Int>(base_seed >> 32),
                   rd() ^ static_cast<Int>(thread_hash.get_value() >> 32),
                   rd() ^ static_cast<Int>(thread_hash.get_value()),
                   rd() ^ static_cast<Int>(time_hash.get_value() >> 32),
                   rd() ^ static_cast<Int>(time_hash.get_value())};

  // Cascade to the base seed so no two threads start with the same base.
  base_seed = time_hash.Rehash(thread_hash.Rehash(base_seed));
  return std::mt19937_64(sd);
}

auto Random::generate() -> Bits_64 {
  thread_local static std::mt19937_64 engine = create_prng();
  return engine();
}
