"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load(":ttx.bzl", "ttx_package")

package(default_visibility = ["//visibility:public"])

ttx_package(
    name = "graphics_package",
    srcs = [
        "packages/graphics/package.ttx",
        "packages/graphics/library/types.ttx",
        "packages/graphics/library/sprite.ttx",
        "packages/graphics/renderers/renderer_2d.ttx",
        "packages/graphics/shaders/default_2d.ttx",
    ],
)

cc_library(
    name = "abi",
    hdrs = glob([
        "abi/*.hpp",
    ], allow_empty = True),
    includes = ["."],
    deps = [
        "//perimortem",
    ],
)

alias(
    name = "dialects",
    actual = "//ttx:dialect",
)

alias(
    name = "dialect",
    actual = ":dialects",
)

cc_library(
    name = "compiler",
    srcs = [
        "compiler/assembler/spirv.cpp",
        "compiler/assembler/x86_64.cpp",
        "compiler/compiler.cpp",
        "compiler/context/names.cpp",
    ] + glob([
        "compiler/shader/*.cpp",
    ]),
    hdrs = glob([
        "compiler/assembler/*.hpp",
        "compiler/context/*.hpp",
        "compiler/shader/*.hpp",
        "compiler/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":abi",
        ":dialect",
        ":validation",
        "//perimortem",
    ],
)

cc_library(
    name = "linker",
    srcs = [
        "linker/linker.cpp",
        "linker/target/elf.cpp",
    ],
    hdrs = glob([
        "linker/target/*.hpp",
        "linker/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":compiler",
        "//perimortem",
    ],
)

# The TTX compiler binary - invoked by Bazel via the ttx_library rule.
alias(
    name = "ttx",
    actual = "//tetrodotoxin/cli:tetrodotoxin",
)

alias(
    name = "format",
    actual = "//ttx:format",
)

alias(
    name = "ttx_core",
    actual = "//ttx:core",
)

cc_library(
    name = "resolution",
    srcs = glob([
        "resolution/*.cpp",
    ]),
    hdrs = glob([
        "resolution/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
    ],
)
