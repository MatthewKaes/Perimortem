"""
Validation - Unit tests and benchmarks for the Perimortem engine.

Run unit tests:
  bazel run //validation:unit_tests

Run the full benchmark suite:
  bazel run -c opt //validation:benchmarks

Enable C++ STL comparison benchmarks:
  bazel run -c opt //validation:benchmarks --define PERI_BENCH_CPP=1
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//tetrodotoxin:ttx.bzl", "ttx_library")

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

ttx_library(
    name = "ttx_tests",
    srcs = glob([
        "unit_tests/tetrodotoxin/**/*.ttx",
    ]),
)

cc_binary(
    name = "unit_tests",
    srcs = glob([
        "unit_tests/perimortem/**/*.cpp",
        "unit_tests/tetrodotoxin/**/*.cpp",
    ]) + [
        "unit_test.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        ":ttx_tests",
        "//perimortem",
        "//tetrodotoxin:compiler",
    ],
)

cc_binary(
    name = "benchmarks",
    srcs = glob([
        "benchmarks/perimortem/**/*.cpp",
    ]) + [
        "benchmark.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
    ],
)
