// Perimortem Engine
// Copyright © Matt Kaes

// benchmark runner — analogous to validation/unit_test.cpp but for
// performance measurement rather than correctness. Each benchmark is called
// repeatedly until a wall-clock cap is reached; timing samples are sorted and
// split into three percentile buckets to distinguish typical from outlier
// performance.

#include "validation/benchmark.hpp"

#include <stdio.h>

#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"

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
  Bits_64 alloc_requests_per_iter;
};

static constexpr Count max_benchmark_count = 1024;
static constexpr Count max_sample_count = 4096;
static constexpr Real_64 time_cap_sec = 1.5;

static Static::Vector<Instance, max_benchmark_count> binary_benchmarks;
static Static::Vector<Bits_64, max_sample_count> time_samples;
static Count benchmark_count = 0;
static View::Bytes benchmark_filter = {};

#ifdef PERI_BENCH_CPP
struct ComparisonInstance {
  const Benchmark::Comparison* comparison;
  Benchmark::BenchmarkFunc func;
};

static Static::Vector<ComparisonInstance, max_benchmark_count> comparisons;
static Static::Vector<SampleStats, max_benchmark_count> stored_stats;
static Count comparison_count = 0;

auto count_variants(const Benchmark::Comparison& comparison) -> Count {
  Count count = 0;
  while (count < Benchmark::max_comparison_variants &&
         comparison.variants[count].header.get_size() > 0) {
    count++;
  }
  return count;
}
#endif

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

#ifdef PERI_BENCH_CPP
namespace Validation::Benchmark {

auto create_comparison(const Comparison& comparison, BenchmarkFunc func)
    -> void {
  comparisons[comparison_count++] = {&comparison, func};
}

}  // namespace Validation::Benchmark
#endif

// Returns a View::Bytes into buffer with the formatted time string.
auto format_time(Static::Bytes<16>& buffer, Bits_64 ns)
    -> View::Bytes {
  auto* character_buffer = Data::cast<char>(buffer.get_data());
  int written = 0;
  if (ns < 1'000ULL) {
    written = snprintf(
        character_buffer, buffer.get_size(), "%llu ns",
        (unsigned long long)ns);
  } else if (ns < 1'000'000ULL) {
    written = snprintf(
        character_buffer, buffer.get_size(), "%.2f us",
        Real_64(ns) / 1'000.0);
  } else {
    written = snprintf(
        character_buffer, buffer.get_size(), "%.2f ms",
        Real_64(ns) / 1'000'000.0);
  }
  return View::Bytes(buffer.get_data(), Count(written > 0 ? written : 0));
}

auto bucket_avg(Count start, Count end_index) -> Bits_64 {
  if (start >= end_index) {
    return time_samples[end_index > 0 ? end_index - 1 : 0];
  }
  Bits_64 total = 0;
  for (Count index = start; index < end_index; index++) {
    total += time_samples[index];
  }
  return total / (end_index - start);
}

auto compute_stats(Count sample_count, Bits_64 alloc_requests) -> SampleStats {
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

auto print_stats(View::Bytes name, Count col_width, const SampleStats& stats)
    -> void {
  Static::Bytes<16> bottom_buffer, middle_buffer, top_buffer;
  View::Bytes bottom = format_time(bottom_buffer, stats.bottom_avg_ns);
  View::Bytes middle = format_time(middle_buffer, stats.middle_avg_ns);
  View::Bytes top = format_time(top_buffer, stats.top_avg_ns);

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

auto output_break() -> void {
  printf(
      "%s[==============================================================]\n%s",
      dark_color, clear_color);
}

// Timing blocks
Time total_start;
Time sample_start;
Time sample_end;

auto Benchmark::start_time() -> void {
  sample_end = Time::never();
  sample_start = Time::now();
}

auto Benchmark::end_time() -> void {
  sample_end = Time::now();
}

auto run_samples(const Harness& harness, Benchmark::BenchmarkFunc func)
    -> SampleStats {
  // Perform one run as a warm up.
  harness.setup();
  func();
  harness.teardown();

  Count sample_count = 0;
  Bits_64 total_alloc_delta = 0;
  total_start = Time::now();

  while (sample_count < max_sample_count) {
    harness.setup();
    Count allocs_before = Bibliotheca::check_out_requests();
    Benchmark::start_time();

    // Run the actual func,
    func();

    // If the func recorded it's own end time then don't override it.
    if (sample_end == Time::never()) {
      Benchmark::end_time();
    }

    // Grab the number of allocations and tear down the test.
    Count allocs_after = Bibliotheca::check_out_requests();
    harness.teardown();

    // Scale the sample data based on the number of batches.
    // By default batch_count is set to 1, but this allows for tests to do their
    // own batching to provide a more accurate hotloop measurement.
    time_samples[sample_count++] =
        sample_start.measure(sample_end).convert_to_nanoseconds() /
        harness.batch_count;
    total_alloc_delta += Bits_64(allocs_after - allocs_before);

    // Check every 16 samples if we are over our time budget, if we are then
    // early terminate.
    if ((sample_count & 0xF) == 0) {
      if (total_start.measure().convert_to_seconds() >= time_cap_sec) {
        break;
      }
    }
  }

  Algorithm::sort(
      Access::Vector<Bits_64>(time_samples.get_data(), sample_count));
  return compute_stats(sample_count, total_alloc_delta / Bits_64(sample_count));
}

#ifdef PERI_BENCH_CPP

auto print_time(const char* color, int width, Bits_64 ns) -> void {
  Static::Bytes<16> buffer;
  View::Bytes time_text = format_time(buffer, ns);
  printf(
      "  %s%*.*s%s", color, width, (int)time_text.get_size(),
      Data::cast<char>(time_text.get_data()), clear_color);
}

auto print_view(const char* color, int width, View::Bytes text) -> void {
  printf(
      "  %s%*.*s%s", color, width, (int)text.get_size(),
      Data::cast<char>(text.get_data()), clear_color);
}

auto find_stored_time(View::Bytes harness_name, View::Bytes bench_name)
    -> Bits_64 {
  for (Count bi = 0; bi < benchmark_count; bi++) {
    if (binary_benchmarks[bi].harness->name == harness_name &&
        binary_benchmarks[bi].name == bench_name) {
      return stored_stats[bi].middle_avg_ns;
    }
  }
  return Bits_64(-1);
}
#endif

auto harness_matches(View::Bytes name) -> Bool {
  if (benchmark_filter.get_size() == 0) {
    return True;
  }
  if (benchmark_filter.get_size() > name.get_size()) {
    return False;
  }
  for (Count i = 0; i < benchmark_filter.get_size(); i++) {
    if ((benchmark_filter.get_data()[i] | 0x20) !=
        (name.get_data()[i] | 0x20)) {
      return False;
    }
  }
  return True;
}

struct Layout {
  Count col_width;
  Count harness_count;
};

auto compute_layout() -> Layout {
  Count col_width = 16;
  Count harness_count = 0;
  const Harness* prev_harness = nullptr;

  for (Count index = 0; index < benchmark_count; index++) {
    if (!harness_matches(binary_benchmarks[index].harness->name)) {
      continue;
    }
    Count name_length = binary_benchmarks[index].name.get_size();
    if (name_length > col_width) {
      col_width = name_length;
    }
    if (binary_benchmarks[index].harness != prev_harness) {
      harness_count++;
      prev_harness = binary_benchmarks[index].harness;
    }
  }

  return {col_width + 2, harness_count};
}

auto print_run_header(const Layout& layout) -> void {
  output_break();
  printf(
      "%s  Perimortem benchmark Engine\n"
      "  benchmarks: %s%llu%s   Harnesses: %s%llu%s\n",
      perimortem_color, clear_color, (unsigned long long)benchmark_count,
      system_color, clear_color, (unsigned long long)layout.harness_count,
      clear_color);
  printf(
      "%s  Columns: p0-10 (fast)  p10-90 (typical)  p90-100 (outliers)\n"
      "  Build with -c opt for meaningful numbers%s\n",
      system_color, clear_color);
  output_break();
  printf(
      "%s  %-*s %s p0-10  %s   p10-90   %s p90-100%s\n", dark_color,
      (int)layout.col_width, "", fast_color, dark_color, slow_color,
      clear_color);
}

auto run_benchmark_pass(const Layout& layout) -> void {
  const Harness* harness = nullptr;
  for (Count benchmark_index = 0; benchmark_index < benchmark_count;
       benchmark_index++) {
    const Instance& benchmark = binary_benchmarks[benchmark_index];

    if (benchmark.harness == nullptr) {
      continue;
    }

    if (!harness_matches(benchmark.harness->name)) {
      continue;
    }

    if (harness == nullptr || benchmark.harness->name != harness->name) {
      harness = benchmark.harness;
      printf(
          "%s[ START ] %.*s\n%s", dark_color, (int)harness->name.get_size(),
          harness->name.get_data(), clear_color);
      harness->init();
    }

    if (harness == nullptr) {
      continue;
    }

    SampleStats stats = run_samples(*harness, benchmark.func);
    print_stats(benchmark.name, layout.col_width, stats);
#ifdef PERI_BENCH_CPP
    stored_stats[benchmark_index] = stats;
#endif
  }
}

#ifdef PERI_BENCH_CPP

struct SectionLayout {
  Count label_column;
  Count max_variant_count;
  Static::Vector<Count, Benchmark::max_comparison_variants> variant_columns;
};

auto compute_section_layout(Count comparison_index, Count section_end)
    -> SectionLayout {
  SectionLayout layout = {};
  layout.label_column = 8;

  for (Count i = comparison_index; i < section_end; i++) {
    const Benchmark::Comparison& comparison = *comparisons[i].comparison;
    Count label_length = comparison.label.get_size();
    if (label_length > layout.label_column) {
      layout.label_column = label_length;
    }
    Count variant_count = count_variants(comparison);
    if (variant_count > layout.max_variant_count) {
      layout.max_variant_count = variant_count;
    }
    for (Count v = 0; v < variant_count; v++) {
      Count header_length = comparison.variants[v].header.get_size();
      Count column_width = header_length > 10 ? header_length : 10;
      if (column_width > layout.variant_columns[v]) {
        layout.variant_columns[v] = column_width;
      }
    }
  }

  layout.label_column += 2;
  return layout;
}

auto print_section_header(
    const Harness& section_harness,
    Count comparison_index,
    Count section_end,
    const SectionLayout& layout) -> void {
  output_break();
  section_harness.init();
  printf(
      "%s[ C++ ] %.*s\n", perimortem_color,
      (int)section_harness.name.get_size(),
      Data::cast<char>(section_harness.name.get_data()));

  printf("%s  %-*s", dark_color, (int)layout.label_column, "");
  for (Count v = 0; v < layout.max_variant_count; v++) {
    View::Bytes header = {};
    for (Count i = comparison_index; i < section_end; i++) {
      if (v < count_variants(*comparisons[i].comparison)) {
        header = comparisons[i].comparison->variants[v].header;
        break;
      }
    }
    printf(
        "  %*.*s", (int)layout.variant_columns[v], (int)header.get_size(),
        Data::cast<char>(header.get_data()));
  }
  printf("  %10s  %10s  %7s%s\n", "C++", "Best", "Delta", clear_color);
}

auto run_comparison_row(Count comparison_index, const SectionLayout& layout)
    -> void {
  const ComparisonInstance& comparison_instance = comparisons[comparison_index];
  const Benchmark::Comparison& comparison = *comparison_instance.comparison;
  Count variant_count = count_variants(comparison);

  Bits_64 cpp_time =
      run_samples(*comparison.harness, comparison_instance.func)
          .middle_avg_ns;
  print_view(dark_color, -(int)layout.label_column, comparison.label);

  Static::Vector<Bits_64, Benchmark::max_comparison_variants> variant_times;
  for (Count v = 0; v < Benchmark::max_comparison_variants; v++) {
    variant_times[v] = Bits_64(-1);
  }

  Bits_64 fastest_time = Bits_64(-1);
  Bits_64 slowest_time = 0;
  Count fastest_variant = Benchmark::max_comparison_variants;
  for (Count v = 0; v < variant_count; v++) {
    Bits_64 variant_time = find_stored_time(
        comparison.harness->name, comparison.variants[v].benchmark_name);
    variant_times[v] = variant_time;
    if (variant_time != Bits_64(-1)) {
      if (variant_time < fastest_time) {
        fastest_time = variant_time;
        fastest_variant = v;
      }
      if (variant_time > slowest_time) {
        slowest_time = variant_time;
      }
    }
  }

  for (Count v = 0; v < layout.max_variant_count; v++) {
    if (v < variant_count && variant_times[v] != Bits_64(-1)) {
      Bits_64 variant_time = variant_times[v];
      const char* color = clear_color;
      if (variant_time == fastest_time) {
        color = fast_color;
      } else if (variant_time == slowest_time) {
        color = slow_color;
      }
      print_time(color, (int)layout.variant_columns[v], variant_time);
    } else if (v < variant_count) {
      printf("  %*s", (int)layout.variant_columns[v], "---");
    } else {
      printf("  %*s", (int)layout.variant_columns[v], "");
    }
  }

  print_time(system_color, 10, cpp_time);

  if (fastest_time != Bits_64(-1)) {
    Real_64 delta = (fastest_time > 0) ? Real_64(cpp_time - fastest_time) /
                                             Real_64(fastest_time) * 100.0
                                       : 0.0;
    const char* delta_color = (delta >= 0.0) ? fast_color : slow_color;
    View::Bytes best_name = (cpp_time < fastest_time)
                                ? "C++"_view
                                : comparison.variants[fastest_variant].header;
    print_view(delta_color, 10, best_name);
    printf("  %s%+7.1f%%%s", delta_color, delta, clear_color);
  }
  printf("\n");
}

auto run_comparison_pass() -> void {
  if (comparison_count == 0) {
    return;
  }

  Count comparison_index = 0;
  while (comparison_index < comparison_count) {
    const Harness* section_harness =
        comparisons[comparison_index].comparison->harness;

    Count section_end = comparison_index;
    while (section_end < comparison_count &&
           comparisons[section_end].comparison->harness == section_harness) {
      section_end++;
    }

    if (harness_matches(section_harness->name)) {
      SectionLayout layout =
          compute_section_layout(comparison_index, section_end);
      print_section_header(
          *section_harness, comparison_index, section_end, layout);

      for (Count i = comparison_index; i < section_end; i++) {
        run_comparison_row(i, layout);
      }
    }

    comparison_index = section_end;
  }
}

#endif  // PERI_BENCH_CPP

int main(int argc, const char* argv[]) {
  if (argc > 1) {
    benchmark_filter = NullTerminated::to_view(argv[1]);
  }
  Layout layout = compute_layout();
  print_run_header(layout);
  run_benchmark_pass(layout);
#ifdef PERI_BENCH_CPP
  run_comparison_pass();
#endif
  output_break();
  printf("\n");
  fflush(stdout);
  return 0;
}
