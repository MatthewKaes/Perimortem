"""
Validation - Unit tests and benchmarks for the Perimortem engine.

Unit tests and benchmarks are all self contained and all tests are run
from a single target. Avoiding multiple targets in the mono repository
prevents drift and ensures all parts of the codebase work together, even
if Perimortem, Tetrodotoxin and TTX are treated as separate projects.

Run unit tests:
  bazel run //validation:unit_tests

Run every unit test with compact output:
  bazel run //validation:unit_tests -- silent

Silent mode changes reporting only. It never selects or skips tests.

Run the full benchmark suite:
  bazel run -c opt //validation:benchmarks

Enable C++ STL comparison benchmarks:
  bazel run -c opt //validation:benchmarks --define PERI_BENCH_CPP=1
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Flip this on with --define PERI_BENCH_CPP=1 to compile in C++ STL comparison
# benchmarks. Off by default to keep build times fast.
config_setting(
    name = "bench_cpp_enabled",
    values = {"define": "PERI_BENCH_CPP=1"},
)

cc_library(
    name = "test_headers",
    hdrs = glob([
        "**/*.hpp",
    ]),
    includes = ["."],
)

# Every unit test source belongs to this target. New suites live beneath
# unit_tests so they build and run against the complete validation graph.
# The runner has no test selection argument.
cc_binary(
    name = "unit_tests",
    srcs = glob([
        "unit_tests/perimortem/**/*.cpp",
        "unit_tests/ttx/**/*.cpp",
        "unit_tests/tetrodotoxin/**/*.cpp",
    ]) + ["unit_test.cpp"],
    copts = ["-Wreturn-type-c-linkage"],
    data = glob([
        "data/**/*.ttx",
    ]),
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
        "//tetrodotoxin:archiver",
        "//tetrodotoxin:compiler",
        "//tetrodotoxin:diagnostics",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:model",
        "//ttx",
    ],
)

cc_binary(
    name = "benchmarks",
    srcs = glob([
        "benchmarks/perimortem/**/*.cpp",
        "benchmarks/tetrodotoxin/**/*.cpp",
    ]) + [
        "benchmark.cpp",
    ],
    includes = ["."],
    local_defines = select({
        ":bench_cpp_enabled": ["PERI_BENCH_CPP=1"],
        "//conditions:default": [],
    }),
    deps = [
        ":test_headers",
        "//perimortem",
        "//tetrodotoxin:archiver",
        "//tetrodotoxin:model",
        "//ttx:concept",
        "//ttx:model",
    ],
)
