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

"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_headers",
    hdrs = glob([
        "**/*.hpp",
    ]),
    includes = ["."],
)

# Validation executes children directly so Bazel launcher output stays outside
# captured streams. Unit-suite self-process cases prove this observer itself;
# they are not evidence for a generated TTX Terminal.
cc_library(
    name = "process_oracle",
    srcs = ["process/child.cpp"],
    hdrs = ["process/child.hpp"],
    deps = ["//perimortem"],
)

# This check proves that the independent C harness compiles. Output evidence
# begins only when a generated TTX Terminal is linked with it and executed.
cc_library(
    name = "library_native_harness_compile_check",
    srcs = ["data/ttx/library/native_harness.c"],
    hdrs = ["data/ttx/library/native.h"],
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
)

# Every unit test source belongs to this target. New suites live beneath
# unit_tests so they build and run against the complete validation graph.
# The runner has no test selection argument.
cc_binary(
    name = "unit_tests",
    srcs = glob([
        "unit_tests/perimortem/**/*.cpp",
        "unit_tests/process/**/*.cpp",
        "unit_tests/ttx/**/*.cpp",
        "unit_tests/tetrodotoxin/**/*.cpp",
    ]) + ["unit_test.cpp"],
    copts = ["-Wreturn-type-c-linkage"],
    data = glob(
        ["data/**/*.ttx"],
        exclude = ["data/ttx/shader_artifact/**"],
    ) + [
        "data/ttx/package_resources/resources/empty.bin",
        "data/ttx/package_resources/resources/table.bin",
        "//apps/ttx/scene_lifetime:sources",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        ":process_oracle",
        "//perimortem",
        "//tetrodotoxin:diagnostics",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:language",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:package",
        "//tetrodotoxin:scene",
        "//tetrodotoxin:shader",
        "//ttx",
    ],
)

cc_binary(
    name = "benchmarks",
    srcs = glob([
        "benchmarks/perimortem/**/*.cpp",
    ]) + [
        "benchmark.cpp",
        "benchmark.hpp",
        "harness.hpp",
    ],
    data = [
        "data/json/init_rpc.json",
        "data/json/test.json",
        "data/json/tokenize_rpc.json",
    ],
    includes = ["."],
    deps = ["//perimortem"],
)
