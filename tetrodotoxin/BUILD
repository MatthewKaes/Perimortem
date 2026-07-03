"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# TODO: Move the standard library packages to the right layer.
# Maybe ttx? Maybe standard or standard_library?
# ttx_package(
#     name = "graphics_package",
#     srcs = [
#         "packages/graphics/library/sprite.ttx",
#         "packages/graphics/library/types.ttx",
#         "packages/graphics/package.ttx",
#         "packages/graphics/renderers/renderer_2d.ttx",
#         "packages/graphics/shaders/default_2d.ttx",
#     ],
# )

cc_library(
    name = "resolution",
    srcs = [
        "resolution/resolver.cpp",
        "resolution/source/cache.cpp",
        "resolution/source/record.cpp",
    ],
    hdrs = [
        "resolution/resolver.hpp",
        "resolution/source/cache.hpp",
        "resolution/source/record.hpp",
    ],
    includes = ["."],
    deps = [
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "compiler",
    srcs = [
        "compiler/assembler/spir_v.cpp",
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
