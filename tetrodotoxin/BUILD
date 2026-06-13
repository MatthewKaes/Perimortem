"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "compiler",
    srcs = [
        "compiler/assembler/x86_64.cpp",
        "compiler/compiler.cpp",
        "compiler/context/names.cpp",
    ],
    hdrs = glob([
        "compiler/assembler/*.hpp",
        "compiler/context/*.hpp",
        "compiler/intermediate/*.hpp",
        "compiler/*.hpp",
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

# The TTX compiler binary — invoked by Bazel via the ttx_library rule.
cc_binary(
    name = "ttx",
    srcs = ["cli/main.cpp"],
    deps = [
        ":compiler",
        ":linker",
        "//perimortem",
    ],
)

cc_library(
    name = "lexical",
    srcs = glob([
        "lexical/*.cpp",
    ]),
    hdrs = glob([
        "lexical/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
    ],
)

cc_library(
    name = "syntax",
    srcs = glob([
        "syntax/*.cpp",
        "syntax/ast/*.cpp",
    ]),
    hdrs = glob([
        "syntax/*.hpp",
        "syntax/ast/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":lexical",
        "//perimortem",
    ],
)
