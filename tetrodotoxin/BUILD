"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "isa",
    srcs = glob([
        "isa/**/*.cpp",
    ]),
    hdrs = glob([
        "isa/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:utility",
        "//tetrodotoxin/standard:types",
        "//ttx",
    ],
)

cc_library(
    name = "toolchain",
    srcs = [
        "toolchain.cpp",
    ],
    hdrs = [
        "toolchain.hpp",
    ],
    includes = ["."],
    deps = [
        ":isa",
        "//perimortem:core",
        "//perimortem:memory",
        "//ttx",
    ],
)

alias(
    name = "puffer",
    actual = "//tetrodotoxin/puffer:puffer",
)

cc_library(
    name = "compiler",
    srcs = [
        "compiler/library.cpp",
        "compiler/shader.cpp",
        "compiler/assembler/spir_v.cpp",
        "compiler/assembler/x86_64.cpp",
    ],
    hdrs = glob([
        "compiler/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":isa",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
        "//ttx",
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
        ":compiler",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
        "//ttx",
    ],
)
