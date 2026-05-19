// Perimortem Engine
// Copyright © Matt Kaes

// This header implements the standard library glue that requires compiler
// extensions or very specific implementations.
//
// There are cases where it makes sense to ally parts of the engine to use the
// standard library, prototyping being one of the bigger ones.

/* Explicity don't protect the include to catch multiple includes */
// #pragma once

#include "validation/unit_test.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>

constexpr const char* clear_color = "\x1b[0m";
constexpr const char* perimortem_color = "\x1b[38;5;124m";
constexpr const char* dark_color = "\x1b[38;5;238m";
constexpr const char* system_color = "\x1b[38;5;246m";
constexpr const char* fail_color = "\x1b[38;5;160m";
constexpr const char* pass_color = "\x1b[38;5;34m";

using namespace Validation::Test;

struct TestInstance {
  const Harness* harness;
  const char* name;
  TestFunc func;
  const char* file;
  long line;
};

struct Benchmark {
  const char* harness = "";
  const char* name = "";
  double time_ms = 0.0;
  const char* file = "";
  long line = 0;
};

TestInstance binary_tests[4096];
unsigned binary_tests_count = 0;
unsigned test_suites = 0;
unsigned passed_tests = 0;
unsigned failed_tests = 0;
unsigned not_run_tests = 0;

// Implementations to link
namespace Validation::Test {
auto do_nothing() -> void {}
auto log_message(const char* file, int line, const char* msg) -> void {
  printf("%s:%d:\n    %s\n", file, line, msg);
}

auto print_result_header(bool actual) -> void {
  if (actual) {
    printf("    ACTUAL =  ");
  } else {
    printf("  EXPECTED =  ");
  }
}

auto expected(bool value, bool actual) -> void {
  print_result_header(actual);
  printf("%s\n", value ? "true" : "false");
}

auto expected(const char* value, bool actual) -> void {
  print_result_header(actual);
  printf("%s\n", value);
}

auto expected(int value, bool actual) -> void {
  print_result_header(actual);
  printf("%d\n", value);
}

auto expected(long long value, bool actual) -> void {
  print_result_header(actual);
  printf("%lld\n", value);
}

auto expected(unsigned long value, bool actual) -> void {
  print_result_header(actual);
  printf("%lu\n", value);
}

auto expected(unsigned long long value, bool actual) -> void {
  print_result_header(actual);
  printf("%llu\n", value);
}

auto expected(double value, bool actual) -> void {
  print_result_header(actual);
  printf("%.6f\n", value);
}

auto expected_text(
    const unsigned char* value,
    unsigned long long size,
    bool actual) -> void {
  print_result_header(actual);
  fwrite(value, 1, size, stdout);
  printf("\n");
}

auto expected_hex(
    const unsigned char* value,
    unsigned long long size,
    bool actual) -> void {
  print_result_header(actual);
  // Print out all the hex bytes with spaces
  for (int i = 0; i < size; i++) {
    printf("%02X ", (unsigned char)value[i]);
  }
  printf("\n");
}

extern auto create(
    const Harness& harness,
    const char* name,
    TestFunc func,
    const char* file,
    long line) -> void {
  binary_tests[binary_tests_count++] = {&harness, name, func, file, line};
}
}  // namespace Validation::Test

static double elapsed_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 +
         (end.tv_nsec - start.tv_nsec) / 1e6;
}

void output_break() {
  printf(
      "%s[==============================================================]\n%s",
      dark_color, clear_color);
}

void output_results() {
  printf("%s\n  Testing Completed:\n", perimortem_color);
  if (passed_tests) {
    printf("%s      Passed:  %u%s\n", pass_color, passed_tests, clear_color);
  }

  if (failed_tests) {
    printf("%s      Failed:  %u%s\n", fail_color, failed_tests, clear_color);
  }

  if (not_run_tests) {
    printf("%s     Not Run:  %u%s\n", system_color, not_run_tests, clear_color);
  }

  float pass_rate =
      (long)(float(passed_tests) / float(binary_tests_count) * 10000) / 100.0f;
  printf(
      "%s  Pass Rate: %s%u / %u%s ( %g %%)\n%s", perimortem_color, clear_color,
      passed_tests, binary_tests_count, system_color, pass_rate, clear_color);
}

int main() {
  const Harness* harness = nullptr;
  test_suites = 0;
  passed_tests = 0;
  failed_tests = 0;
  not_run_tests = binary_tests_count;

  Benchmark longest_test;
  unsigned longest_test_name = 12;

  for (unsigned i = 0; i < binary_tests_count; ++i) {
    const TestInstance& test = binary_tests[i];
    if (strlen(test.name) > longest_test_name) {
      longest_test_name = strlen(test.name);
    }

    if (test.harness != harness) {
      test_suites += 1;
      harness = test.harness;
    }
  }

  // Reset
  harness = nullptr;

  output_break();
  printf(
      "%s  Executing Perimortem test engine from Validation::Test\n",
      perimortem_color);
  printf(
      "  Tests found:  %s%u%s (%u Harness)%s\n", clear_color,
      binary_tests_count, system_color, test_suites, clear_color);
  output_break();

  struct timespec start_full, end_full, start, end;
  clock_gettime(CLOCK_MONOTONIC, &start_full);

  for (unsigned i = 0; i < binary_tests_count; ++i) {
    const TestInstance& test = binary_tests[i];
    if (test.harness != harness) {
      harness = test.harness;
      printf("%s[ START ] %s\n%s", dark_color, harness->name, clear_color);
      harness->init();
    }

    // Somehow the harness was set to nullptr.
    if (harness == nullptr) {
      continue;
    }

    // Setup
    harness->setup();

    not_run_tests -= 1;
    // printf("%s  [  TEST  ] %s\n%s", dark_color, test.name, clear_color);
    TestResult result = TestResult::Pass;
    clock_gettime(CLOCK_MONOTONIC, &start);
    test.func(result);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double test_time_ms = elapsed_ms(start, end);

    // Tear down
    harness->teardown();

    if (test_time_ms > longest_test.time_ms) {
      longest_test.time_ms = test_time_ms;
      longest_test.name = test.name;
      longest_test.harness = harness->name;
      longest_test.file = test.file;
      longest_test.line = test.line;
    }

    switch (result) {
    case TestResult::Pass:
      passed_tests += 1;
      printf(
          "%s  [  PASS  ] %*s", pass_color, (int)(longest_test_name + 2),
          test.name);
      break;
    case TestResult::Failed:
      failed_tests += 1;
      printf(
          "%s  [  FAIL  ] %*s", fail_color, (int)(longest_test_name + 2),
          test.name);
      break;
    }

    printf("%s  (%g ms)\n%s", system_color, test_time_ms, clear_color);
  }

  clock_gettime(CLOCK_MONOTONIC, &end_full);
  double full_time_ms = elapsed_ms(start_full, end_full);

  output_break();

  output_results();

  printf(
      "%s\n  Total Time:  %s%g ms\n\n%s", perimortem_color, clear_color,
      full_time_ms, clear_color);

  output_break();

  printf("\n");
  fflush(stdout);

  return failed_tests;
}
