# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Validation binaries and source-graph integration products."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//toolchain:tetrodotoxin.bzl", "ttx_source")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_headers",
    hdrs = glob(["**/*.hpp"]),
    includes = ["."],
)

ttx_source(
    name = "runtime_package",
    src = "data/ttx/products/runtime/package.ttx",
)

ttx_source(
    name = "foreign_package",
    src = "data/ttx/products/foreign/package.ttx",
)

ttx_source(
    name = "system_abi_package",
    src = "data/ttx/products/system_abi/package.ttx",
)

ttx_source(
    name = "native_dependency",
    src = "data/ttx/package_native/provider/package.ttx",
)

ttx_source(
    name = "package_native_consumer",
    src = "data/ttx/package_native/consumer/package.ttx",
    deps = [":native_dependency"],
)

ttx_source(
    name = "shader_package",
    src = "data/ttx/shader_artifact/package.ttx",
    deps = ["//packages/ttx:perimortem_math"],
)

cc_library(
    name = "process_fixture",
    srcs = ["process/child.cpp"],
    hdrs = ["process/child.hpp"],
    deps = ["//perimortem"],
)

cc_binary(
    name = "unit_tests",
    srcs = glob(
        [
            "unit_tests/perimortem/**/*.cpp",
            "unit_tests/process/**/*.cpp",
            "unit_tests/puffer/**/*.cpp",
            "unit_tests/ttx/**/*.cpp",
            "unit_tests/tetrodotoxin/**/*.cpp",
        ],
    ) + ["unit_test.cpp"],
    data = glob(
        ["data/**/*.ttx"],
        exclude = ["data/ttx/shader_artifact/**"],
    ) + [
        "data/ttx/app_profiles/resources/icon.bin",
        "data/ttx/package_resources/resources/empty.bin",
        "data/ttx/package_resources/resources/table.bin",
        "//packages/ttx:perimortem_memory",
    ],
    includes = ["."],
    deps = [
        ":process_fixture",
        ":test_headers",
        "//perimortem",
        "//puffer:position_encoding",
        "//puffer:publisher",
        "//tetrodotoxin:diagnostics",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:formatting",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:package",
        "//tetrodotoxin/app",
        "//tetrodotoxin/graphics/runtime:runtime",
        "//tetrodotoxin/language",
        "//tetrodotoxin/render",
        "//tetrodotoxin/runtime:application",
        "//tetrodotoxin/scene",
        "//tetrodotoxin/shader",
        "//tetrodotoxin/terminal:llvm",
        "//tetrodotoxin/terminal:spirv",
        "//ttx",
    ],
)

cc_binary(
    name = "benchmarks",
    srcs = glob(["benchmarks/perimortem/**/*.cpp"]) + [
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

cc_binary(
    name = "session_oracle",
    srcs = [
        "unit_test.cpp",
        "unit_tests/process/child.cpp",
        "unit_tests/tetrodotoxin/runtime/session.cpp",
    ],
    includes = ["."],
    deps = [
        ":process_fixture",
        ":test_headers",
        "//perimortem:core",
        "//tetrodotoxin/runtime:application",
    ],
)
