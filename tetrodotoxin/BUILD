"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

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
        "//perimortem",
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
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "resolution",
    srcs = [
        "resolution/resolver.cpp",
        "resolution/source/cache.cpp",
    ],
    hdrs = [
        "resolution/resolver.hpp",
        "resolution/source/cache.hpp",
        "resolution/source/record.hpp",
    ],
    includes = ["."],
    deps = [
        ":isa",
        ":toolchain",
        "//perimortem",
        "//ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "main.cpp",
    ],
    deps = [
        ":linker",
        ":resolution",
        ":toolchain",
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
