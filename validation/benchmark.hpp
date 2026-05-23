// Perimortem Engine
// Copyright © Matt Kaes

// Intentionally no include guard — a double include in the same translation
// unit causes a compile error, catching accidental re-inclusion before it
// silently produces duplicate benchmark registrations at runtime.
// #pragma once

#include "validation/harness.hpp"

namespace Validation::Benchmark {

using BenchmarkFunc = void (*)();

// Sets a new start point for a test.
// Useful if the test needs specific setup that it needs in the acutal test
// block that can't be moved to the `.setup` function.
auto start_time() -> void;

// Lets the test override the end timestamp.
auto end_time() -> void;

auto create(
    const Harness& harness,
    Perimortem::Core::View::Bytes name,
    BenchmarkFunc func,
    Perimortem::Core::View::Bytes file,
    Count line) -> void;

// Prevents the optimizer from removing test loads using a read/write
// register-or-memory constraint.
template <typename value_type>
auto prevent_optimization(value_type& value) -> void {
  asm volatile("" : "+r,m"(value) : : "memory");
}

class BenchmarkEntry {
 public:
  BenchmarkEntry(
      const Harness& harness,
      Perimortem::Core::View::Bytes name,
      BenchmarkFunc func,
      Perimortem::Core::View::Bytes file,
      Count line) {
    create(harness, name, func, file, line);
  }
};

}  // namespace Validation::Benchmark

#define PERIMORTEM_BENCHMARK(harness, name)                               \
  auto __##harness##__##name() -> void;                                   \
  namespace {                                                             \
  Validation::Benchmark::BenchmarkEntry _reg_bench_##harness##_##name = { \
    harness, #name##_view, __##harness##__##name,                         \
    Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__};       \
  }                                                                       \
  auto __##harness##__##name() -> void
