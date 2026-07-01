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

alias(
    name = "dialects",
    actual = "//ttx:dialect",
)

alias(
    name = "dialect",
    actual = ":dialects",
)

cc_library(
    name = "compiler_assembler",
    srcs = [
        "compiler/assembler/spirv.cpp",
        "compiler/assembler/x86_64.cpp",
    ],
    hdrs = glob([
        "compiler/assembler/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
    ],
)

cc_library(
    name = "linker",
    srcs = [
        "linker/linker.cpp",
        "linker/target/elf.cpp",
    ],
    hdrs = [
        "linker/linker.hpp",
        "linker/object/relocation.hpp",
        "linker/object/section.hpp",
        "linker/object/symbol.hpp",
        "linker/target/elf.hpp",
        "linker/target/format.hpp",
    ],
    includes = ["."],
    deps = [
        "//perimortem",
    ],
)

# The TTX compiler binary - invoked by Bazel via the ttx_library rule.
alias(
    name = "ttx",
    actual = "//tetrodotoxin/cli:tetrodotoxin",
)

alias(
    name = "ttx_core",
    actual = "//ttx:core",
)
