"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

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
        "compiler/intermediate/*.hpp",
        "compiler/shader/*.hpp",
        "compiler/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":dialect",
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
    name = "format",
    srcs = glob([
        "format/*.cpp",
        "format/syntax/**/*.cpp",
    ]),
    hdrs = glob([
        "format/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":lexical",
        ":syntax",
        "//perimortem",
    ],
)

cc_library(
    name = "syntax",
    srcs = glob([
        "syntax/*.cpp",
        "syntax/attribute/*.cpp",
        "syntax/ast/*.cpp",
        "syntax/dialect/*.cpp",
        "syntax/expression/*.cpp",
        "syntax/type/*.cpp",
    ]),
    hdrs = glob([
        "syntax/*.hpp",
        "syntax/attribute/*.hpp",
        "syntax/ast/*.hpp",
        "syntax/dialect/*.hpp",
        "syntax/expression/*.hpp",
        "syntax/type/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":lexical",
        "//perimortem",
    ],
)

cc_library(
    name = "ttx_facts",
    hdrs = [
        "ttx/facts.hpp",
    ],
    includes = ["."],
    deps = [
        "//perimortem",
    ],
)

cc_library(
    name = "ttx_core",
    srcs = glob([
        "ttx/core/*.cpp",
    ]),
    hdrs = glob([
        "ttx/core/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":syntax",
        ":ttx_facts",
        "//perimortem",
    ],
)

cc_library(
    name = "ttx_graphics",
    srcs = glob([
        "ttx/graphics/*.cpp",
    ]),
    hdrs = glob([
        "ttx/graphics/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":syntax",
        ":ttx_core",
        ":ttx_facts",
        "//perimortem",
    ],
)

cc_library(
    name = "ttx_standard",
    srcs = [
        "ttx/standard.cpp",
    ],
    hdrs = [
        "ttx/standard.hpp",
    ],
    includes = ["."],
    deps = [
        ":syntax",
        ":ttx_core",
        ":ttx_facts",
        ":ttx_graphics",
    ],
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
        ":syntax",
        ":ttx_standard",
        "//perimortem",
    ],
)

cc_library(
    name = "dialect",
    srcs = glob([
        "dialect/**/*.cpp",
    ]),
    hdrs = glob([
        "dialect/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":resolution",
        ":syntax",
        ":ttx_standard",
        "//perimortem",
    ],
)
