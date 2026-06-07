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
  Long alloc_requests_per_iter;
};

static constexpr Count max_benchmark_count = 1024;
static constexpr Count max_sample_count = 4096;
static constexpr Real_64 time_cap_seconds = 1.5;

static Static::Vector<Instance, max_benchmark_count> binary_benchmarks;
static Static::Vector<Bits_64, max_sample_count> time_samples;
static Count benchmark_count = 0;
static View::Bytes bench_filter = {};

#ifdef PERI_BENCH_CPP
struct ComparisonInstance {
  const Benchmark::Comparison* comp;
  Benchmark::BenchmarkFunc func;
};

static Static::Vector<ComparisonInstance, max_benchmark_count> comparisons;
static Static::Vector<SampleStats, max_benchmark_count> stored_stats;
static Count comparison_count = 0;

auto count_variants(const Benchmark::Comparison& comp) -> Count {
  Count count = 0;
  while (count < Benchmark::max_comparison_variants &&
         comp.variants[count].header.get_size() > 0) {
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

auto create_comparison(const Comparison& comp, BenchmarkFunc func) -> void {
  comparisons[comparison_count++] = {&comp, func};
}

}  // namespace Validation::Benchmark
#endif

// Returns a View::Bytes into buf with the formatted time string.
auto format_time(Static::Bytes<16>& buf, Bits_64 ns) -> View::Bytes {
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

auto compute_stats(Count sample_count, Long alloc_requests) -> SampleStats {
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
  Long total_alloc_delta = 0;
  total_start = Time::now();

  while (sample_count < max_sample_count) {
    harness.setup();
    Count allocs_before = Bibliotheca::check_out_requests();
    Benchmark::start_time();

    // Run the actual function,
    func();

    // If the function recorded it's own end time then don't override it.
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
    total_alloc_delta += Long(allocs_after - allocs_before);

    // Check every 16 samples if we are over our time budget, if we are then
    // early terminate.
    if ((sample_count & 0xF) == 0) {
      if (total_start.measure().convert_to_seconds() >= time_cap_seconds) {
        break;
      }
    }
  }

  Algorithm::sort(
      Access::Vector<Bits_64>(time_samples.get_data(), sample_count));
  return compute_stats(sample_count, total_alloc_delta / Long(sample_count));
}

#ifdef PERI_BENCH_CPP

auto print_time(const char* color, int width, Bits_64 ns) -> void {
  Static::Bytes<16> buf;
  View::Bytes str = format_time(buf, ns);
  printf(
      "  %s%*.*s%s", color, width, (int)str.get_size(),
      Data::cast<char>(str.get_data()), clear_color);
}

auto print_view(const char* color, int width, View::Bytes text) -> void {
  printf(
      "  %s%*.*s%s", color, width, (int)text.get_size(),
      Data::cast<char>(text.get_data()), clear_color);
}

auto find_stored_ns(View::Bytes harness_name, View::Bytes bench_name)
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
  if (bench_filter.get_size() == 0) {
    return True;
  }
  if (bench_filter.get_size() > name.get_size()) {
    return False;
  }
  for (Count i = 0; i < bench_filter.get_size(); i++) {
    if ((bench_filter.get_data()[i] | 0x20) != (name.get_data()[i] | 0x20)) {
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
    Count name_len = binary_benchmarks[index].name.get_size();
    if (name_len > col_width) {
      col_width = name_len;
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
  Count label_col;
  Count max_vcount;
  Count vcols[Benchmark::max_comparison_variants];
};

auto compute_section_layout(Count comp_index, Count section_end)
    -> SectionLayout {
  SectionLayout sl = {};
  sl.label_col = 8;

  for (Count ci = comp_index; ci < section_end; ci++) {
    const Benchmark::Comparison& comp = *comparisons[ci].comp;
    Count llen = comp.label.get_size();
    if (llen > sl.label_col) {
      sl.label_col = llen;
    }
    Count vcount = count_variants(comp);
    if (vcount > sl.max_vcount) {
      sl.max_vcount = vcount;
    }
    for (Count v = 0; v < vcount; v++) {
      Count hlen = comp.variants[v].header.get_size();
      Count col = hlen > 10 ? hlen : 10;
      if (col > sl.vcols[v]) {
        sl.vcols[v] = col;
      }
    }
  }

  sl.label_col += 2;
  return sl;
}

auto print_section_header(
    const Harness& section_harness,
    Count comp_index,
    Count section_end,
    const SectionLayout& sl) -> void {
  output_break();
  section_harness.init();
  printf(
      "%s[ C++ ] %.*s\n", perimortem_color,
      (int)section_harness.name.get_size(),
      Data::cast<char>(section_harness.name.get_data()));

  printf("%s  %-*s", dark_color, (int)sl.label_col, "");
  for (Count v = 0; v < sl.max_vcount; v++) {
    View::Bytes hdr = {};
    for (Count ci = comp_index; ci < section_end; ci++) {
      if (v < count_variants(*comparisons[ci].comp)) {
        hdr = comparisons[ci].comp->variants[v].header;
        break;
      }
    }
    printf(
        "  %*.*s", (int)sl.vcols[v], (int)hdr.get_size(),
        Data::cast<char>(hdr.get_data()));
  }
  printf("  %10s  %10s  %7s%s\n", "C++", "Best", "Delta", clear_color);
}

auto run_comparison_row(Count ci, const SectionLayout& sl) -> void {
  const ComparisonInstance& cinst = comparisons[ci];
  const Benchmark::Comparison& comp = *cinst.comp;
  Count vcount = count_variants(comp);

  Bits_64 cpp_ns = run_samples(*comp.harness, cinst.func).middle_avg_ns;
  print_view(dark_color, -(int)sl.label_col, comp.label);

  Bits_64 vns_cache[Benchmark::max_comparison_variants];
  for (Count v = 0; v < Benchmark::max_comparison_variants; v++) {
    vns_cache[v] = Bits_64(-1);
  }
  Bits_64 best_ns = Bits_64(-1);
  Bits_64 worst_ns = 0;
  Count best_v = Benchmark::max_comparison_variants;
  for (Count v = 0; v < vcount; v++) {
    Bits_64 vns =
        find_stored_ns(comp.harness->name, comp.variants[v].benchmark_name);
    vns_cache[v] = vns;
    if (vns != Bits_64(-1)) {
      if (vns < best_ns) {
        best_ns = vns;
        best_v = v;
      }
      if (vns > worst_ns) {
        worst_ns = vns;
      }
    }
  }

  for (Count v = 0; v < sl.max_vcount; v++) {
    if (v < vcount && vns_cache[v] != Bits_64(-1)) {
      Bits_64 vns = vns_cache[v];
      const char* color = clear_color;
      if (vns == best_ns) {
        color = fast_color;
      } else if (vns == worst_ns) {
        color = slow_color;
      }
      print_time(color, (int)sl.vcols[v], vns);
    } else if (v < vcount) {
      printf("  %*s", (int)sl.vcols[v], "---");
    } else {
      printf("  %*s", (int)sl.vcols[v], "");
    }
  }

  print_time(system_color, 10, cpp_ns);

  if (best_ns != Bits_64(-1)) {
    Real_64 delta = (best_ns > 0) ? Real_64(Long(cpp_ns) - Long(best_ns)) /
                                        Real_64(best_ns) * 100.0
                                  : 0.0;
    const char* delta_color = (delta >= 0.0) ? fast_color : slow_color;
    View::Bytes best_name =
        (cpp_ns < best_ns) ? "C++"_view : comp.variants[best_v].header;
    print_view(delta_color, 10, best_name);
    printf("  %s%+7.1f%%%s", delta_color, delta, clear_color);
  }
  printf("\n");
}

auto run_comparison_pass() -> void {
  if (comparison_count == 0) {
    return;
  }

  Count comp_index = 0;
  while (comp_index < comparison_count) {
    const Harness* section_harness = comparisons[comp_index].comp->harness;

    Count section_end = comp_index;
    while (section_end < comparison_count &&
           comparisons[section_end].comp->harness == section_harness) {
      section_end++;
    }

    if (harness_matches(section_harness->name)) {
      SectionLayout sl = compute_section_layout(comp_index, section_end);
      print_section_header(*section_harness, comp_index, section_end, sl);

      for (Count ci = comp_index; ci < section_end; ci++) {
        run_comparison_row(ci, sl);
      }
    }

    comp_index = section_end;
  }
}

#endif  // PERI_BENCH_CPP

int main(int argc, const char* argv[]) {
  if (argc > 1) {
    bench_filter = NullTerminated::to_view(argv[1]);
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
