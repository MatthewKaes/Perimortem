# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Validation binaries and source-graph integration products."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

genrule(
    name = "rust_ttx_provider_archive",
    srcs = glob(["cross_language/rust/*.rs"]),
    outs = ["librust_ttx_provider.a"],
    cmd = "/usr/bin/rustc --edition=2021 -D warnings -C panic=abort --crate-type staticlib $(location cross_language/rust/driver.rs) -o $@",
)

cc_binary(
    name = "plugin_fixture.so",
    srcs = ["plugin/fixture.c"],
    linkshared = True,
    linkstatic = True,
    visibility = ["//visibility:private"],
    deps = [
        "//tetrodotoxin:plugin",
        "//ttx",
    ],
)

cc_binary(
    name = "window_failure_fixture",
    srcs = ["process/window_failure.cpp"],
    visibility = ["//visibility:private"],
    deps = ["//tetrodotoxin/runtime:application"],
)

cc_binary(
    name = "unit_tests",
    srcs = glob(
        [
            "cross_language/c/*.c",
            "cross_language/cpp/*.cpp",
            "process/**/*.cpp",
            "unit_tests/perimortem/**/*.cpp",
            "unit_tests/process/**/*.cpp",
            "unit_tests/puffer/**/*.cpp",
            "unit_tests/ttx/**/*.cpp",
            "unit_tests/tetrodotoxin/**/*.cpp",
        ],
        exclude = ["process/window_failure.cpp"],
    ) + glob([
        "**/*.h",
        "**/*.hpp",
    ]) + [
        "c_headers.c",
        "cross_language/consumer.c",
        "cross_language/driver.cpp",
        "unit_test.cpp",
        ":rust_ttx_provider_archive",
        "//ttx:abi.h",
    ],
    data = glob(
        ["data/**/*.ttx"],
        exclude = ["data/ttx/shader_artifact/**"],
    ) + [
        "data/ttx/app_profiles/resources/icon.bin",
        "data/ttx/package_resources/resources/empty.bin",
        "data/ttx/package_resources/resources/table.bin",
        ":plugin_fixture.so",
        ":window_failure_fixture",
        "//packages/ttx:perimortem_memory",
        "//tetrodotoxin/terminal:graph_text_plugin.so",
    ],
    includes = ["."],
    linkopts = [
        "-ldl",
        "-lm",
        "-lpthread",
        "-rdynamic",
    ],
    deps = [
        "//perimortem",
        "//puffer:invocation",
        "//puffer:position_encoding",
        "//puffer:publisher",
        "//tetrodotoxin:build",
        "//tetrodotoxin:diagnostics",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:formatting",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:package",
        "//tetrodotoxin:plugin",
        "//tetrodotoxin/app",
        "//tetrodotoxin/graphics",
        "//tetrodotoxin/language",
        "//tetrodotoxin/language:provider_abi",
        "//tetrodotoxin/render",
        "//tetrodotoxin/runtime:application",
        "//tetrodotoxin/scene",
        "//tetrodotoxin/shader",
        "//tetrodotoxin/terminal:graph_text",
        "//tetrodotoxin/terminal:llvm",
        "//tetrodotoxin/terminal:provider",
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
