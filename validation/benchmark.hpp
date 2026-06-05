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

#ifdef PERI_BENCH_CPP

namespace Validation::Benchmark {

static constexpr Count max_comparison_variants = 8;

// One column in a multi-way comparison table.
struct ComparisonVariant {
  Perimortem::Core::View::Bytes header;
  Perimortem::Core::View::Bytes benchmark_name;
};

// A comparison group: several Perimortem variants measured against one C++
// baseline. Variants with an empty header act as a sentinel (end of list).
struct Comparison {
  const Harness* harness = nullptr;
  Perimortem::Core::View::Bytes label;
  ComparisonVariant variants[max_comparison_variants] = {};
};

auto create_comparison(const Comparison& comp, BenchmarkFunc func) -> void;

class ComparisonEntry {
 public:
  ComparisonEntry(const Comparison& comp, BenchmarkFunc func) {
    create_comparison(comp, func);
  }
};

}  // namespace Validation::Benchmark

// Define a multi-way comparison. comp_var must be a static Comparison struct
// visible at the call site. The function body that follows is the C++ baseline.
#define PERIMORTEM_COMPARISON(comp_var)                        \
  auto __comp_##comp_var() -> void;                            \
  namespace {                                                  \
  Validation::Benchmark::ComparisonEntry _reg_comp_##comp_var{ \
    comp_var, __comp_##comp_var};                              \
  }                                                            \
  auto __comp_##comp_var() -> void

#endif  // PERI_BENCH_CPP
