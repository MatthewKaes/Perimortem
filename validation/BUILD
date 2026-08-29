# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Validation binaries and source-graph integration products."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//toolchain:tetrodotoxin.bzl", "ttx_source")
load("//validation:c_headers.bzl", "c_header")

package(default_visibility = ["//visibility:public"])

c_header("abi_core", "perimortem/abi/c/core.h")
c_header("abstract", "ttx/concept/abstract.h")
c_header("alias", "ttx/concept/alias.h")
c_header("concept_callable", "ttx/concept/callable.h")
c_header("constant", "ttx/concept/constant.h")
c_header("concept_context", "ttx/concept/context.h")
c_header("documentation", "ttx/concept/documentation.h")
c_header("interface", "ttx/concept/interface.h")
c_header("layout", "ttx/concept/layout.h")
c_header("none", "ttx/concept/none.h")
c_header("concept_pack", "ttx/concept/pack.h")
c_header("unknown", "ttx/concept/unknown.h")
c_header("addressable", "ttx/model/addressable.h")
c_header("model_callable", "ttx/model/callable.h")
c_header("model_context", "ttx/model/context.h")
c_header("documentation_block", "ttx/model/documentations/block.h")
c_header("documentation_comment", "ttx/model/documentations/comment.h")
c_header("documentation_merged", "ttx/model/documentations/merged.h")
c_header("interface_callable", "ttx/model/interfaces/callable.h")
c_header("layout_addressable", "ttx/model/layouts/addressable.h")
c_header("layout_composite", "ttx/model/layouts/composite.h")
c_header("layout_fluid", "ttx/model/layouts/fluid.h")
c_header("layout_named", "ttx/model/layouts/named.h")
c_header("layout_ranged", "ttx/model/layouts/ranged.h")
c_header("layout_termination", "ttx/model/layouts/termination.h")
c_header("layout_value", "ttx/model/layouts/value.h")
c_header("model_pack", "ttx/model/pack.h")
c_header("type", "ttx/model/type.h")

cc_library(
    name = "c_ttx_headers",
    deps = [
        ":c_header_abi_core",
        ":c_header_abstract",
        ":c_header_addressable",
        ":c_header_alias",
        ":c_header_concept_callable",
        ":c_header_concept_context",
        ":c_header_concept_pack",
        ":c_header_constant",
        ":c_header_documentation",
        ":c_header_documentation_block",
        ":c_header_documentation_comment",
        ":c_header_documentation_merged",
        ":c_header_interface",
        ":c_header_layout",
        ":c_header_layout_addressable",
        ":c_header_layout_composite",
        ":c_header_layout_fluid",
        ":c_header_layout_named",
        ":c_header_layout_ranged",
        ":c_header_layout_termination",
        ":c_header_layout_value",
        ":c_header_interface_callable",
        ":c_header_model_callable",
        ":c_header_model_context",
        ":c_header_model_pack",
        ":c_header_none",
        ":c_header_type",
        ":c_header_unknown",
    ],
)

cc_library(
    name = "test_headers",
    hdrs = glob(["**/*.hpp"]),
    includes = ["."],
)

cc_binary(
    name = "c_ttx_core",
    srcs = ["c/ttx_core.c"],
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = ["//ttx:model"],
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
        "//ttx:bootstrap",
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
