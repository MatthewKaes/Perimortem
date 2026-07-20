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

cc_binary(
    name = "ttx_concept_tests",
    srcs = [
        "unit_test.cpp",
        "unit_tests/ttx/abstract.cpp",
        "unit_tests/ttx/documentation.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
        "//ttx:concept",
        "//ttx:model",
    ],
)

cc_binary(
    name = "ttx_model_tests",
    srcs = [
        "unit_test.cpp",
        "unit_tests/ttx/attribute.cpp",
        "unit_tests/ttx/callable.cpp",
        "unit_tests/ttx/constexpr.cpp",
        "unit_tests/ttx/expression.cpp",
        "unit_tests/ttx/flow.cpp",
        "unit_tests/ttx/generic.cpp",
        "unit_tests/ttx/layout.cpp",
        "unit_tests/ttx/type.cpp",
        "unit_tests/ttx/types.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
        "//ttx:concept",
        "//ttx:model",
    ],
)

cc_binary(
    name = "ttx_lexical_tests",
    srcs = [
        "unit_test.cpp",
        "unit_tests/ttx/lexical/tokenizer.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
        "//ttx:lexical",
    ],
)

cc_binary(
    name = "tetrodotoxin_model_tests",
    srcs = [
        "unit_test.cpp",
        "unit_tests/tetrodotoxin/model.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
        "//tetrodotoxin:model",
        "//ttx:concept",
        "//ttx:model",
    ],
)

cc_binary(
    name = "dialect_tests",
    srcs = [
        "unit_test.cpp",
        "unit_tests/tetrodotoxin/dialect.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
        "//tetrodotoxin:model",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_binary(
    name = "unit_tests",
    srcs = glob([
        "unit_tests/perimortem/**/*.cpp",
        "unit_tests/ttx/**/*.cpp",
        "unit_tests/tetrodotoxin/**/*.cpp",
    ]) + ["unit_test.cpp"],
    copts = ["-Wreturn-type-c-linkage"],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
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
    ],
)
