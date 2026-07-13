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
    srcs = [
        "unit_tests/tetrodotoxin/ttx/source.ttx",
    ],
)

ttx_library(
    name = "shader_artifact_smoke",
    srcs = [
        "data/ttx/shader_artifact/render.ttx",
        "data/ttx/shader_artifact/shader.ttx",
    ],
)

alias(
    name = "graphics_package_smoke",
    actual = "//tetrodotoxin/standard:perimortem_graphics",
)

alias(
    name = "math_package_smoke",
    actual = "//tetrodotoxin/standard:perimortem_math",
)

alias(
    name = "runtime_package_smoke",
    actual = "//tetrodotoxin/standard:perimortem_runtime",
)

ttx_library(
    name = "app_scene_parse_smoke",
    srcs = [
        "//apps/ttx/demo:main",
        "//apps/ttx/demo:splash_screen",
    ],
    deps = [
        "//tetrodotoxin/standard:perimortem_graphics",
        "//tetrodotoxin/standard:perimortem_math",
        "//tetrodotoxin/standard:perimortem_runtime",
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
    data = [
        ":shader_artifact_smoke",
        ":ttx_tests",
        "//tetrodotoxin/standard:perimortem_graphics",
        "//tetrodotoxin/standard:perimortem_math",
        "//tetrodotoxin/standard:perimortem_runtime",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        ":ttx_tests",
        "//perimortem",
        "//tetrodotoxin:compiler",
        "//tetrodotoxin:diagnostics",
        "//tetrodotoxin:isa",
        "//tetrodotoxin:isa_lowerers",
        "//tetrodotoxin:linker",
        "//tetrodotoxin/archiver",
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
        "//tetrodotoxin/standard:perimortem_graphics",
        "//tetrodotoxin/standard:perimortem_math",
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
