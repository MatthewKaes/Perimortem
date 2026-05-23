// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/uuid.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

static Harness SystemUuid = {
  .name = "UUIDs"_view,
};

PERIMORTEM_BENCHMARK(SystemUuid, generate) {
  // Full UUID generation: entropy read + Philox seeding + 128-bit construction.
  Count value = 0;
  for (Count i = 0; i < 1024; i++) {
    auto uuid = Uuid::generate();
    value ^= uuid.get_value()[0];
  }
  Benchmark::prevent_optimization(value);
}

PERIMORTEM_BENCHMARK(SystemUuid, serialize) {
  // Format a UUID to its 36-character canonical string representation.
  Count value = 0;
  for (Count i = 0; i < 1024; i++) {
    auto uuid = Uuid::generate();
    auto serialized = uuid.serialize();
    value += serialized[0];
  }
  Benchmark::prevent_optimization(value);
}

PERIMORTEM_BENCHMARK(SystemUuid, deserialize_uuid) {
  auto uuid_str = "550e8400-e29b-41d4-a716-446655440000"_bytes;
  Count value = 0;
  Benchmark::prevent_optimization(uuid_str);
  for (Count i = 0; i < 1024; i++) {
    Uuid uuid;
    uuid.deserialize(uuid_str);
    value ^= uuid.get_value()[0];
    uuid_str[i & 0b11111] += 1;
  }
  Benchmark::prevent_optimization(value);
}

PERIMORTEM_BENCHMARK(SystemUuid, deserialize_packed) {
  auto uuid_str = "550e8400e29b41d4a716446655440000"_bytes;
  Count value = 0;
  Benchmark::prevent_optimization(uuid_str);
  for (Count i = 0; i < 1024; i++) {
    Uuid uuid;
    uuid.deserialize(uuid_str);
    value ^= uuid.get_value()[0];
    uuid_str[i & 0b11111] += 1;
  }
  Benchmark::prevent_optimization(value);
}
