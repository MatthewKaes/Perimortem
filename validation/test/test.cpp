// Perimortem Engine
// Copyright © Matt Kaes

// This header implements the standard library glue that requires compiler
// extensions or very specific implementations.
//
// There are cases where it makes sense to ally parts of the engine to use the
// standard library, prototyping being one of the bigger ones.

/* Explicity don't protect the include to catch multiple includes */
// #pragma once

#include "validation/test/test.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

constexpr const char* clear_color = "\x1b[0m";
constexpr const char* perimortem_color = "\x1b[38;5;52m";
constexpr const char* dark_color = "\x1b[38;5;232m";
constexpr const char* system_color = "\x1b[38;5;248m";
constexpr const char* fail_color = "\x1b[38;5;160m";
constexpr const char* pass_color = "\x1b[38;5;34m";

using namespace Validation::Test;

using namespace std::chrono;

struct TestInstance {
  const Harness& harness;
  const char* name;
  TestFunc func;
  const char* file;
  long line;
};

struct Benchmark {
  const char* harness = "";
  const char* name = "";
  duration<double, std::milli> time = {};
  const char* file = "";
  long line = 0;
};

std::vector<TestInstance> binary_tests;
unsigned test_suites = 0;
unsigned passed_tests = 0;
unsigned failed_tests = 0;
unsigned not_run_tests = 0;

// Implementations to link
namespace Validation::Test {
auto do_nothing() -> void {}
auto log_message(const char* file, int line, const char* msg) -> void {
  std::cout << file << ":" << line << ":" << "\n    " << msg << "\n";
}

extern auto create(const Harness& harness,
                   const char* name,
                   TestFunc func,
                   const char* file,
                   long line) -> void {
  binary_tests.push_back({harness, name, func, file, line});
}
}  // namespace Validation::Test

void output_break() {
  std::cout << dark_color << "[==================]\n" << clear_color;
}

void output_results() {
  std::cout << perimortem_color << "\n  Testing Completed:" << "\n";
  if (passed_tests) {
    std::cout << pass_color << "      Passed:  " << passed_tests << clear_color
              << "\n";
  }

  if (failed_tests) {
    std::cout << fail_color << "      Failed:  " << failed_tests << clear_color
              << "\n";
  }

  if (not_run_tests) {
    std::cout << system_color << "     Not Run:  " << not_run_tests
              << clear_color << "\n";
  }

  float pass_rate =
      long(float(passed_tests) / float(binary_tests.size()) * 10000) / 100.0;
  std::cout << perimortem_color << "  Pass Rate: ";
  std::cout << clear_color << passed_tests << " / " << binary_tests.size()
            << system_color << " ( " << pass_rate << " %)\n"
            << clear_color;
}

int main() {
  const Harness* harness = nullptr;
  test_suites = 0;
  passed_tests = 0;
  failed_tests = 0;
  not_run_tests = binary_tests.size();

  Benchmark longest_test;

  for (const auto& test : binary_tests) {
    if (&test.harness != harness) {
      test_suites += 1;
      harness = &test.harness;
    }
  }

  // Reset
  harness = nullptr;

  output_break();
  std::cout << perimortem_color
            << "  Executing Perimortem test engine from Validation::Test"
            << "\n";
  std::cout << "  Tests found:  " << clear_color << binary_tests.size()
            << system_color << " (" << test_suites << " Harness)" << clear_color
            << "\n";
  output_break();

  high_resolution_clock::time_point start_full = high_resolution_clock::now();

  for (const auto& test : binary_tests) {
    if (&test.harness != harness) {
      harness = &test.harness;
      std::cout << dark_color << "[ START ] " << harness->name << "\n"
                << clear_color;
      harness->init();
    }

    not_run_tests -= 1;
    // std::cout << dark_color << "  [  TEST  ] " << test.name << "\n"
    //           << clear_color;
    TestResult result;
    high_resolution_clock::time_point start = high_resolution_clock::now();
    test.func(result);
    high_resolution_clock::time_point end = high_resolution_clock::now();
    duration<double, std::milli> test_time = end - start;

    if (test_time > longest_test.time) {
      longest_test.time = test_time;
      longest_test.name = test.name;
      longest_test.harness = harness->name;
      longest_test.file = test.file;
      longest_test.line = test.line;
    }

    switch (result) {
      case TestResult::Pass:
        passed_tests += 1;
        std::cout << pass_color << "  [  PASS  ] " << test.name;
        break;
      case TestResult::Failed:
        failed_tests += 1;
        std::cout << fail_color << "  [  FAIL  ] " << test.name;
        break;
    }

    std::cout << system_color << "  (" << test_time.count() << " ms)\n"
              << clear_color;
  }

  high_resolution_clock::time_point end_full = high_resolution_clock::now();
  duration<double, std::milli> full_time = end_full - start_full;

  output_break();

  output_results();

  std::cout << perimortem_color << "\n  Total Time:  ";
  std::cout << clear_color << full_time.count() << " ms\n\n" << clear_color;

  output_break();

  std::cout << std::endl;
}
