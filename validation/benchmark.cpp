// Perimortem Engine
// Copyright © Matt Kaes

// benchmark runner — analogous to validation/unit_test.cpp but for
// performance measurement rather than correctness. Each benchmark is called
// repeatedly until a wall-clock cap is reached; timing samples are sorted and
// split into three percentile buckets to distinguish typical from outlier
// performance.

#include "validation/benchmark.hpp"

#include <stdio.h>
#include <time.h>

#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Validation;
using namespace Perimortem::Core;

constexpr const char* clear_color = "\x1b[0m";
constexpr const char* perimortem_color = "\x1b[38;5;124m";
constexpr const char* dark_color = "\x1b[38;5;238m";
constexpr const char* system_color = "\x1b[38;5;246m";
constexpr const char* fast_color = "\x1b[38;5;34m";
constexpr const char* slow_color = "\x1b[38;5;160m";

struct Instance {
  const Harness* harness;
  Perimortem::Core::View::Bytes name;
  Benchmark::BenchmarkFunc func;
  Perimortem::Core::View::Bytes file;
  Count line;
};

struct SampleStats {
  Bits_64 bottom_avg_ns;
  Bits_64 middle_avg_ns;
  Bits_64 top_avg_ns;
  Bits_64 min_ns;
  Bits_64 max_ns;
  Count sample_count;
  Long alloc_requests_per_iter;
};

static constexpr Count max_benchmark_count = 1024;
static constexpr Count max_sample_count = 4096;
static constexpr Bits_64 time_cap_ns = 2'000'000'000ULL;

static Static::Vector<Instance, max_benchmark_count> binary_benchmarks;
static Static::Vector<Bits_64, max_sample_count> time_samples;
static Count benchmark_count = 0;

namespace Validation::Benchmark {

auto do_nothing() -> void {}

auto create(
    const Harness& harness,
    Perimortem::Core::View::Bytes name,
    Benchmark::BenchmarkFunc func,
    Perimortem::Core::View::Bytes file,
    Count line) -> void {
  binary_benchmarks[benchmark_count++] = {&harness, name, func, file, line};
}

}  // namespace Validation::Benchmark

static auto elapsed_ns(
    struct timespec start,
    struct timespec end,
    Count clock_nanos) -> Bits_64 {
  return Bits_64(end.tv_sec - start.tv_sec) * 1'000'000'000ULL +
         Bits_64(end.tv_nsec - start.tv_nsec) - clock_nanos;
}

// Returns a View::Bytes into buf with the formatted time string.
static auto format_time(Static::Bytes<16>& buf, Bits_64 ns) -> View::Bytes {
  auto* cbuf = Data::cast<char>(buf.get_data());
  int written = 0;
  if (ns < 1'000ULL) {
    written = snprintf(cbuf, buf.get_size(), "%llu ns", (unsigned long long)ns);
  } else if (ns < 1'000'000ULL) {
    written = snprintf(cbuf, buf.get_size(), "%.2f us", Real_64(ns) / 1'000.0);
  } else {
    written =
        snprintf(cbuf, buf.get_size(), "%.2f ms", Real_64(ns) / 1'000'000.0);
  }
  return View::Bytes(buf.get_data(), Count(written > 0 ? written : 0));
}

static auto bucket_avg(Count start, Count end_index) -> Bits_64 {
  if (start >= end_index) {
    return time_samples[end_index > 0 ? end_index - 1 : 0];
  }
  Bits_64 total = 0;
  for (Count index = start; index < end_index; index++) {
    total += time_samples[index];
  }
  return total / (end_index - start);
}

static auto compute_stats(Count sample_count, Long alloc_requests)
    -> SampleStats {
  SampleStats stats = {};
  stats.sample_count = sample_count;
  stats.min_ns = time_samples[0];
  stats.max_ns = time_samples[sample_count - 1];
  stats.alloc_requests_per_iter = alloc_requests;

  Count tenth = sample_count / 10;
  if (tenth < 1) {
    tenth = 1;
  }

  stats.bottom_avg_ns = bucket_avg(0, tenth);
  stats.middle_avg_ns = bucket_avg(tenth, sample_count - tenth);
  stats.top_avg_ns = bucket_avg(sample_count - tenth, sample_count);

  return stats;
}

static auto print_stats(
    View::Bytes name,
    Count col_width,
    const SampleStats& stats) -> void {
  Static::Bytes<16> bottom_buf, middle_buf, top_buf;
  View::Bytes bottom = format_time(bottom_buf, stats.bottom_avg_ns);
  View::Bytes middle = format_time(middle_buf, stats.middle_avg_ns);
  View::Bytes top = format_time(top_buf, stats.top_avg_ns);

  printf(
      "  %-*.*s %s%9.*s%s  %9.*s  %s%9.*s%s", (int)col_width,
      (int)name.get_size(), Data::cast<char>(name.get_data()), fast_color,
      (int)bottom.get_size(), Data::cast<char>(bottom.get_data()), clear_color,
      (int)middle.get_size(), Data::cast<char>(middle.get_data()), slow_color,
      (int)top.get_size(), Data::cast<char>(top.get_data()), clear_color);

  if (stats.alloc_requests_per_iter > 0) {
    printf(
        "  | %s%lld alloc/iter%s", system_color,
        (long long)stats.alloc_requests_per_iter, clear_color);
  }

  printf("\n");
}

static auto output_break() -> void {
  printf(
      "%s[==============================================================]\n%s",
      dark_color, clear_color);
}

// Timing blocks
struct timespec total_start;
struct timespec total_now;
struct timespec sample_start;
struct timespec sample_end;

auto Benchmark::start_time() -> void {
  clock_gettime(CLOCK_MONOTONIC, &sample_start);
}

auto Benchmark::end_time() -> void {
  clock_gettime(CLOCK_MONOTONIC, &sample_end);
}

int main() {
  Count col_width = 16;
  Count harness_count = 0;
  const Harness* prev_harness = nullptr;

  for (Count index = 0; index < benchmark_count; index++) {
    Count name_len = binary_benchmarks[index].name.get_size();
    if (name_len > col_width) {
      col_width = name_len;
    }
    if (binary_benchmarks[index].harness != prev_harness) {
      harness_count++;
      prev_harness = binary_benchmarks[index].harness;
    }
  }

  col_width += 2;

  output_break();
  printf(
      "%s  Perimortem benchmark Engine\n"
      "  benchmarks: %s%llu%s   Harnesses: %s%llu%s\n",
      perimortem_color, clear_color, (unsigned long long)benchmark_count,
      system_color, clear_color, (unsigned long long)harness_count,
      clear_color);
  printf(
      "%s  Columns: p0-10 (fast)  p10-90 (typical)  p90-100 (outliers)\n"
      "  Build with -c opt for meaningful numbers%s\n",
      system_color, clear_color);
  output_break();
  printf(
      "%s  %-*s %s p0-10  %s   p10-90   %s p90-100%s\n", dark_color,
      (int)col_width, "", fast_color, dark_color, slow_color, clear_color);

  const Harness* harness = nullptr;

  // Clock warming
  Count clock_nanos = 0;
  for (Count i = 0; i < 128; i++) {
    Benchmark::start_time();
    Benchmark::end_time();
    clock_nanos = sample_end.tv_nsec - sample_start.tv_nsec;
    Benchmark::prevent_optimization(clock_nanos);
  }
  clock_nanos /= 128;
  Benchmark::prevent_optimization(clock_nanos);

  for (Count benchmark_index = 0; benchmark_index < benchmark_count;
       benchmark_index++) {
    const Instance& benchmark = binary_benchmarks[benchmark_index];

    if (benchmark.harness == nullptr) {
      continue;
    }

    if (harness == nullptr || benchmark.harness->name != harness->name) {
      harness = benchmark.harness;
      printf(
          "%s[ START ] %.*s\n%s", dark_color, (int)harness->name.get_size(),
          harness->name.get_data(), clear_color);
      harness->init();
    }

    // Harness wasn't set some how?
    if (harness == nullptr) {
      continue;
    }

    // Warmup: one full cycle before timing starts, to prime caches.
    harness->setup();
    benchmark.func();
    harness->teardown();

    Count sample_count = 0;
    Long total_alloc_delta = 0;

    clock_gettime(CLOCK_MONOTONIC, &total_start);

    while (sample_count < max_sample_count) {
      harness->setup();

      Count allocs_before = Bibliotheca::check_out_requests();
      sample_end.tv_sec = 0;
      sample_end.tv_nsec = 0;
      clock_gettime(CLOCK_MONOTONIC, &sample_start);
      benchmark.func();

      // Regular time stamp if the test didn't explictly grab one.
      if (sample_end.tv_nsec == 0 && sample_end.tv_sec == 0) {
        clock_gettime(CLOCK_MONOTONIC, &sample_end);
      }
      Count allocs_after = Bibliotheca::check_out_requests();

      harness->teardown();

      time_samples[sample_count++] =
          elapsed_ns(sample_start, sample_end, clock_nanos);
      total_alloc_delta += Long(allocs_after - allocs_before);

      // Poll every 16 samples to limit clock_gettime overhead on fast
      // benchmark.
      if ((sample_count & 15) == 0) {
        clock_gettime(CLOCK_MONOTONIC, &total_now);
        if (elapsed_ns(total_start, total_now, clock_nanos) >= time_cap_ns) {
          break;
        }
      }
    }

    Algorithm::sort(
        Access::Vector<Bits_64>(time_samples.get_data(), sample_count));

    Long avg_allocs = total_alloc_delta / Long(sample_count);
    SampleStats stats = compute_stats(sample_count, avg_allocs);
    print_stats(benchmark.name, col_width, stats);
  }

  output_break();
  printf("\n");
  fflush(stdout);
  return 0;
}
