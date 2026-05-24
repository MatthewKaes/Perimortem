// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/benchmark.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Validation;

static Harness AllocatorBench = {
  .name = "Allocators"_view,
};

PERIMORTEM_BENCHMARK(AllocatorBench, bibliotheca_cycle_64) {
  Bibliotheca::Allocation alloc;
  for (Count i = 0; i < 100; i++) {
    alloc = Bibliotheca::check_out(1 << 6);
    Benchmark::prevent_optimization(alloc.ptr);
    Bibliotheca::remit(alloc.ptr);
  }
}

PERIMORTEM_BENCHMARK(AllocatorBench, bibliotheca_cycle_1kb) {
  Bibliotheca::Allocation alloc;
  for (Count i = 0; i < 100; i++) {
    alloc = Bibliotheca::check_out(1 << 10);
    Benchmark::prevent_optimization(alloc.ptr);
    Bibliotheca::remit(alloc.ptr);
  }
}

PERIMORTEM_BENCHMARK(AllocatorBench, bibliotheca_cycle_64kb) {
  Bibliotheca::Allocation alloc;
  for (Count i = 0; i < 100; i++) {
    alloc = Bibliotheca::check_out(1 << 16);
    Benchmark::prevent_optimization(alloc.ptr);
    Bibliotheca::remit(alloc.ptr);
  }
}

PERIMORTEM_BENCHMARK(AllocatorBench, arena_cycle_64) {
  Allocator::Arena arena;
  for (Count i = 0; i < 100; i++) {
    auto alloc = arena.allocate(1 << 6);
    Benchmark::prevent_optimization(alloc);
  }
}

PERIMORTEM_BENCHMARK(AllocatorBench, arena_cycle_1kb) {
  Allocator::Arena arena;
  for (Count i = 0; i < 100; i++) {
    auto alloc = arena.allocate(1 << 10);
    Benchmark::prevent_optimization(alloc);
  }
}

PERIMORTEM_BENCHMARK(AllocatorBench, arena_cycle_64kb) {
  Allocator::Arena arena;
  for (Count i = 0; i < 100; i++) {
    auto alloc = arena.allocate(1 << 16);
    Benchmark::prevent_optimization(alloc);
  }
}

template <Count frame_alloc_count, Count size_minimum, Count size_range>
auto frame_stability() {
  Byte* ptrs[frame_alloc_count];
  for (Count i = 0; i < frame_alloc_count; i++) {
    auto alloc = Bibliotheca::check_out(
        (Random::generate() & size_range) + size_minimum);
    ptrs[i] = alloc.ptr;
  }
  for (Count i = 0; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }
  Benchmark::prevent_optimization(ptrs[0]);
}

#define STABILITY_BENCH(prefix, count, min_bytes, max_mask)     \
  PERIMORTEM_BENCHMARK(                                         \
      AllocatorBench, frame_##prefix##_##count##_allocations) { \
    frame_stability<count, min_bytes, max_mask>();              \
  }

STABILITY_BENCH(small, 1024, 64, 0xFF);
STABILITY_BENCH(small, 16384, 64, 0xFF);
STABILITY_BENCH(small, 65536, 64, 0xFFFF);
STABILITY_BENCH(varied, 1024, 64, 0xFFFF);
STABILITY_BENCH(varied, 16384, 64, 0xFFFF);
STABILITY_BENCH(varied, 65536, 64, 0xFFFF);

template <
    Count frame_alloc_count,
    Count window_count,
    Count size_minimum,
    Count size_range>
auto frame_stability_interleaved() {
  constexpr Count window = frame_alloc_count / window_count;
  Byte* ptrs[frame_alloc_count];

  // Initial allocation.
  for (Count i = 0; i < window; i++) {
    ptrs[i] =
        Bibliotheca::check_out((Random::generate() & size_range) + size_minimum)
            .ptr;
  }

  // Rolling exhchange.
  for (Count i = window; i < frame_alloc_count; i++) {
    ptrs[i] =
        Bibliotheca::check_out((Random::generate() & size_range) + size_minimum)
            .ptr;
    Bibliotheca::remit(ptrs[i - window]);
  }

  // Final clean up.
  for (Count i = frame_alloc_count - window; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }

  Benchmark::prevent_optimization(ptrs[0]);
}

#define INTERLEAVED_BENCH(count, windows)                                      \
  PERIMORTEM_BENCHMARK(                                                        \
      AllocatorBench, frame_##windows##_interleaved##_##count##_allocations) { \
    frame_stability_interleaved<count, windows, 64, 0xFF>();                   \
  }

INTERLEAVED_BENCH(16384, 8);
INTERLEAVED_BENCH(16384, 16);
INTERLEAVED_BENCH(16384, 32);
INTERLEAVED_BENCH(65536, 8);
INTERLEAVED_BENCH(65536, 16);
INTERLEAVED_BENCH(65536, 32);
