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

PERIMORTEM_BENCHMARK(AllocatorBench, frame_stability_bulk_1k) {
  constexpr Count frame_alloc_count = 1 << 10;
  Byte* ptrs[frame_alloc_count];
  for (Count i = 0; i < frame_alloc_count; i++) {
    auto alloc = Bibliotheca::check_out(Random::generate() % (1 << 10));
    ptrs[i] = alloc.ptr;
  }
  for (Count i = 0; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }
  Benchmark::prevent_optimization(ptrs[0]);
}

PERIMORTEM_BENCHMARK(AllocatorBench, frame_stability_bulk_64k) {
  constexpr Count frame_alloc_count = 1 << 16;
  Byte* ptrs[frame_alloc_count];
  for (Count i = 0; i < frame_alloc_count; i++) {
    auto alloc = Bibliotheca::check_out(Random::generate() % (1 << 10));
    ptrs[i] = alloc.ptr;
  }
  for (Count i = 0; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }
  Benchmark::prevent_optimization(ptrs[0]);
}

PERIMORTEM_BENCHMARK(AllocatorBench, frame_stability_wide_1k) {
  constexpr Count frame_alloc_count = 1 << 10;
  Byte* ptrs[frame_alloc_count];
  for (Count i = 0; i < frame_alloc_count; i++) {
    auto alloc = Bibliotheca::check_out(Random::generate() % (1 << 16));
    ptrs[i] = alloc.ptr;
  }
  for (Count i = 0; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }
  Benchmark::prevent_optimization(ptrs[0]);
}

PERIMORTEM_BENCHMARK(AllocatorBench, frame_stability_wide_64k) {
  constexpr Count frame_alloc_count = 1 << 16;
  Byte* ptrs[frame_alloc_count];
  for (Count i = 0; i < frame_alloc_count; i++) {
    auto alloc = Bibliotheca::check_out(Random::generate() % (1 << 16));
    ptrs[i] = alloc.ptr;
  }
  for (Count i = 0; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }
  Benchmark::prevent_optimization(ptrs[0]);
}

PERIMORTEM_BENCHMARK(AllocatorBench, frame_stability_interleaved) {
  // Interleaves allocations and remits
  constexpr Count frame_alloc_count = 1 << 10;
  constexpr Count window = frame_alloc_count / 8;
  Byte* ptrs[frame_alloc_count];

  // Initial allocation.
  for (Count i = 0; i < window; i++) {
    ptrs[i] = Bibliotheca::check_out(Random::generate() % (1 << 10)).ptr;
  }

  // Rolling exhchange.
  for (Count i = window; i < frame_alloc_count; i++) {
    ptrs[i] = Bibliotheca::check_out(Random::generate() % (1 << 10)).ptr;
    Bibliotheca::remit(ptrs[i - window]);
  }

  // Final clean up.
  for (Count i = frame_alloc_count - window; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }

  Benchmark::prevent_optimization(ptrs[0]);
}
