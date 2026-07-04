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
    name = "puffer_resolution",
    srcs = [
        "puffer/resolution/resolver.cpp",
        "puffer/resolution/source/cache.cpp",
    ],
    hdrs = [
        "puffer/resolution/resolver.hpp",
        "puffer/resolution/source/cache.hpp",
        "puffer/resolution/source/record.hpp",
    ],
    includes = ["."],
    deps = [
        ":isa",
        ":puffer_boot",
        ":toolchain",
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "puffer_boot",
    srcs = glob([
        "puffer/isa/boot/**/*.cpp",
    ]),
    hdrs = glob([
        "puffer/isa/boot/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":isa",
        "//perimortem",
        "//ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "puffer/main.cpp",
    ],
    deps = [
        ":compiler",
        ":linker",
        ":puffer_boot",
        ":puffer_resolution",
        ":toolchain",
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "compiler",
    srcs = [
        "compiler/library.cpp",
        "compiler/assembler/spir_v.cpp",
        "compiler/assembler/x86_64.cpp",
    ],
    hdrs = glob([
        "compiler/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":linker",
        "//perimortem",
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
        "//perimortem",
    ],
)
