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
load("//toolchain:tetrodotoxin.bzl", "ttx_library", "ttx_package")

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

ttx_package(
    name = "graphics_package_smoke",
    srcs = [
        "//perimortem:graphics/library/sprite.ttx",
        "//perimortem:graphics/library/types.ttx",
        "//perimortem:graphics/package.ttx",
        "//perimortem:graphics/renderers/renderer_2d.ttx",
        "//perimortem:graphics/shaders/default_2d.ttx",
    ],
)

ttx_package(
    name = "math_package_smoke",
    srcs = [
        "//perimortem:math/library/types.ttx",
        "//perimortem:math/package.ttx",
    ],
)

ttx_package(
    name = "runtime_package_smoke",
    srcs = [
        "//perimortem:runtime/library/types.ttx",
        "//perimortem:runtime/package.ttx",
    ],
)

ttx_library(
    name = "app_scene_smoke",
    srcs = [
        "//apps:main",
        "//apps:splash_screen",
    ],
    deps = [
        ":graphics_package_smoke",
        ":math_package_smoke",
        ":runtime_package_smoke",
    ],
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
        "//tetrodotoxin:isa",
        "//tetrodotoxin:linker",
        "//tetrodotoxin/puffer:resolution",
        "//tetrodotoxin/puffer:terminal",
        "//tetrodotoxin:toolchain",
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
