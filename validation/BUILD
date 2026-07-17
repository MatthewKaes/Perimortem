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
load("//toolchain:tetrodotoxin.bzl", "ttx_library")

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
    library_name = "Validation.TtxTests",
    srcs = [
        "unit_tests/tetrodotoxin/ttx/source.ttx",
    ],
)

ttx_library(
    name = "shader_artifact_smoke",
    library_name = "Validation.ShaderArtifact",
    srcs = [
        "data/ttx/shader_artifact/render.ttx",
        "data/ttx/shader_artifact/shader.ttx",
    ],
)

ttx_library(
    name = "app_scene_parse_smoke",
    library_name = "Validation.AppScene",
    srcs = [
        "//apps/ttx/demo:main",
        "//apps/ttx/demo:splash_screen",
    ],
    deps = [
        "//tetrodotoxin/standard:graphics",
        "//tetrodotoxin/standard:math",
        "//tetrodotoxin/standard:runtime",
    ],
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
    ],
)

cc_binary(
    name = "ttx_model_tests",
    srcs = [
        "unit_test.cpp",
        "unit_tests/ttx/attribute.cpp",
        "unit_tests/ttx/callable.cpp",
        "unit_tests/ttx/expression.cpp",
        "unit_tests/ttx/flow.cpp",
        "unit_tests/ttx/generic.cpp",
        "unit_tests/ttx/group.cpp",
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
    name = "interpreter_tests",
    srcs = [
        "unit_test.cpp",
        "unit_tests/tetrodotoxin/interpreter.cpp",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//perimortem",
        "//tetrodotoxin:interpreter",
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
    ]) + [
        "unit_test.cpp",
    ],
    copts = ["-Wreturn-type-c-linkage"],
    data = [
        ":shader_artifact_smoke",
        ":ttx_tests",
        "//tetrodotoxin/standard:graphics",
        "//tetrodotoxin/standard:math",
        "//tetrodotoxin/standard:runtime",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        ":ttx_tests",
        "//perimortem",
        "//tetrodotoxin/abi",
        "//tetrodotoxin:compiler",
        "//tetrodotoxin:diagnostics",
        "//tetrodotoxin:interpreter",
        "//tetrodotoxin:isa",
        "//tetrodotoxin:isa_lowerers",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:model",
        "//tetrodotoxin/archiver",
        "//tetrodotoxin/puffer:compiler",
        "//tetrodotoxin/puffer:package_builder",
        "//tetrodotoxin/puffer:resolution",
        "//tetrodotoxin/puffer:toolchain",
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
    data = [
        "//tetrodotoxin/standard:graphics",
        "//tetrodotoxin/standard:math",
    ],
    includes = ["."],
    local_defines = select({
        ":bench_cpp_enabled": ["PERI_BENCH_CPP=1"],
        "//conditions:default": [],
    }),
    deps = [
        ":test_headers",
        "//perimortem",
        "//tetrodotoxin/archiver",
    ],
)
