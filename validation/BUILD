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
load("//toolchain:tetrodotoxin.bzl", "ttx_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_headers",
    hdrs = glob([
        "**/*.hpp",
    ]),
    includes = ["."],
)

ttx_library(
    name = "foreign_llvm",
    srcs = ["data/ttx/library/foreign.ttx"],
    library_name = "Foreign",
)

cc_library(
    name = "foreign_integration",
    srcs = ["data/ttx/llvm/foreign_harness.c"],
    copts = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = [":foreign_llvm"],
)

ttx_library(
    name = "runtime_llvm",
    srcs = ["data/ttx/llvm/runtime.ttx"],
    debug = "full",
    library_name = "Runtime",
)

cc_library(
    name = "runtime_integration",
    srcs = ["data/ttx/llvm/runtime_harness.c"],
    copts = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = [":runtime_llvm"],
)

# This standalone integration proves that ttx_library propagates every runtime
# object needed by managed generated values. It intentionally has no direct
# Perimortem dependency, and unit_tests executes it to observe process cleanup.
cc_binary(
    name = "runtime_link_integration",
    srcs = ["data/ttx/llvm/runtime_harness.c"],
    copts = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    defines = ["TTX_STANDALONE_INTEGRATION=1"],
    deps = [":runtime_llvm"],
)

# Validation executes children directly so Bazel launcher output stays outside
# captured streams. Unit suite process cases prove this observer itself. They
# are not evidence for a generated TTX Terminal.
cc_library(
    name = "process_fixture",
    srcs = ["process/child.cpp"],
    hdrs = ["process/child.hpp"],
    deps = ["//perimortem"],
)

# Every unit test source belongs to this target. New suites live beneath
# unit_tests so they build and run against the complete validation graph.
# The runner has no test selection argument.
cc_binary(
    name = "unit_tests",
    srcs = glob([
        "unit_tests/perimortem/**/*.cpp",
        "unit_tests/process/**/*.cpp",
        "unit_tests/puffer/**/*.cpp",
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
        ":runtime_link_integration",
        "//apps/ttx/scene_lifetime:sources",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        ":process_fixture",
        ":foreign_integration",
        ":runtime_integration",
        "//tetrodotoxin:library_llvm",
        "//perimortem",
        "//tetrodotoxin:diagnostics",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:language",
        "//tetrodotoxin:library",
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
