# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

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
load(
    "//toolchain:tetrodotoxin.bzl",
    "ttx_native_provider",
    "ttx_package",
)

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_headers",
    hdrs = glob([
        "**/*.hpp",
    ]),
    includes = ["."],
)

cc_library(
    name = "runtime_native",
    srcs = ["data/ttx/llvm/runtime_provider.c"],
)

ttx_native_provider(
    name = "runtime_native_provider",
    dep = ":runtime_native",
    functions = [
        "llvm_dense_access",
        "llvm_dense_view",
        "llvm_object_identity",
        "ttx_system_print",
    ],
    provider_name = "Validation.Runtime.Host",
)

ttx_package(
    name = "runtime_llvm",
    package_name = "Validation.Runtime",
    debug = "full",
    manifest = "data/ttx/products/runtime/package.ttx",
    native_deps = [":runtime_native_provider"],
    version = [
        1,
        0,
    ],
)

ttx_native_provider(
    name = "runtime_export_provider",
    dep = ":runtime_llvm",
    functions = ["llvm_large_sum"],
    provider_name = "Validation.Runtime",
)

cc_library(
    name = "foreign_native",
    srcs = ["data/ttx/llvm/foreign_provider.c"],
)

ttx_native_provider(
    name = "foreign_native_provider",
    dep = ":foreign_native",
    functions = ["library_foreign_add"],
    provider_name = "Validation.Foreign.Host",
    readonly_states = ["library_foreign_bias"],
    writable_states = ["library_foreign_state"],
)

ttx_package(
    name = "foreign_llvm",
    package_name = "Validation.Foreign",
    manifest = "data/ttx/products/foreign/package.ttx",
    native_deps = [
        ":foreign_native_provider",
        ":runtime_export_provider",
    ],
    version = [
        1,
        0,
    ],
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

ttx_native_provider(
    name = "system_abi_provider",
    dep = "//perimortem:abi",
    functions = [
        "perimortem_system_terminal_read_line",
        "perimortem_system_terminal_write_line",
    ],
    provider_name = "Perimortem.System",
)

ttx_package(
    name = "system_abi_llvm",
    package_name = "Validation.SystemAbi",
    manifest = "data/ttx/products/system_abi/package.ttx",
    native_deps = [":system_abi_provider"],
    version = [
        1,
        0,
    ],
)

cc_binary(
    name = "system_abi_integration",
    srcs = ["data/ttx/llvm/system_abi_harness.c"],
    copts = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = [
        ":system_abi_llvm",
        "//perimortem:abi",
    ],
)

cc_library(
    name = "package_header_integration",
    srcs = ["data/ttx/llvm/package_headers_harness.c"],
    copts = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = ["//packages/ttx:perimortem_system"],
)

cc_binary(
    name = "graphics_integration",
    srcs = ["data/ttx/graphics_harness.c"],
    copts = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = ["//packages/ttx:perimortem_graphics"],
)

ttx_package(
    name = "package_native_provider",
    package_name = "Validation.Provider",
    manifest = "data/ttx/package_native/provider/package.ttx",
    version = [
        1,
        0,
    ],
)

ttx_package(
    name = "package_native_consumer",
    package_name = "Validation.Consumer",
    manifest = "data/ttx/package_native/consumer/package.ttx",
    version = [
        1,
        0,
    ],
    deps = [":package_native_provider"],
)

ttx_package(
    name = "shader_artifact_package",
    package_name = "Validation.Shader",
    manifest = "data/ttx/shader_artifact/package.ttx",
    version = [
        1,
        0,
    ],
    deps = ["//packages/ttx:perimortem_math"],
)

cc_binary(
    name = "package_native_integration",
    srcs = ["data/ttx/package_native/harness.c"],
    copts = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = [":package_native_consumer"],
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

# This process links only the Package product. The Package carries its selected
# host provider and managed runtime dependencies into the final application.
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

# This process sees only the generated facade and its TTX implementation. It
# keeps the portable seed out of both compilation and linking so every familiar
# C++ operation proves the independent ABI product.
cc_binary(
    name = "cpp_api_integration",
    srcs = ["data/ttx/cpp_api_harness.cpp"],
    deps = ["//packages/ttx:perimortem_memory_cpp"],
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
    data = glob(
        ["data/**/*.ttx"],
        exclude = ["data/ttx/shader_artifact/**"],
    ) + [
        "data/ttx/app_profiles/resources/icon.bin",
        "data/ttx/package_resources/resources/empty.bin",
        "data/ttx/package_resources/resources/table.bin",
        ":cpp_api_integration",
        ":graphics_integration",
        ":package_native_integration",
        ":runtime_link_integration",
        ":system_abi_integration",
        "//apps/ttx/echo",
        "//apps/ttx/echo:package",
        "//apps/ttx/scene_lifetime:scene_lifetime",
        "//apps/ttx/scene_lifetime:authored_sources",
        "//packages/ttx:perimortem_graphics",
        "//packages/ttx:perimortem_math",
        "//packages/ttx:perimortem_memory",
        "//packages/ttx:perimortem_system",
    ],
    includes = ["."],
    deps = [
        ":foreign_integration",
        ":package_header_integration",
        ":process_fixture",
        ":runtime_integration",
        ":shader_artifact_package",
        ":test_headers",
        "//packages/ttx:perimortem_system",
        "//perimortem",
        "//puffer:position_encoding",
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
