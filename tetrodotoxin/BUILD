"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load(":ttx.bzl", "ttx_library")

package(default_visibility = ["//visibility:public"])

# # Scratch ttx binary linked to a scratch cc binary.
# ttx_library(
#     name = "hello",
#     srcs = ["__tests/hello.ttx"],
# )

# cc_binary(
#     name = "hello_bin",
#     srcs = ["__tests/hello.cpp"],
#     deps = [
#         ":hello",
#         "//perimortem",
#     ],
# )

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

# cc_library(
#     name = "lexical",
#     srcs = glob([
#         "lexical/*.cpp",
#     ]),
#     hdrs = glob([
#         "lexical/*.hpp",
#     ]),
#     includes = ["."],
#     deps = [
#         "//perimortem",
#     ],
# )

# cc_library(
#     name = "virtual_machine",
#     srcs = glob([
#         "virtual_machine/engine/*.cpp",
#     ]),
#     hdrs = glob([
#         "virtual_machine/engine/*.hpp",
#         "virtual_machine/metadata/*.hpp",
#         "virtual_machine/types/*.hpp",
#         "virtual_machine/types/std/*.hpp",
#     ]),
#     includes = ["."],
#     deps = [
#         ":lexical",
#         "//perimortem",
#     ],
# )

# cc_library(
#     name = "parser",
#     srcs = glob([
#         "parser/*.cpp",
#         "parser/visitor/*.cpp",
#     ]),
#     hdrs = glob([
#         "parser/*.hpp",
#         "parser/visitor/*.hpp",
#     ]),
#     includes = ["."],
#     deps = [
#         ":lexical",
#         ":virtual_machine",
#         "//perimortem",
#     ],
# )

# cc_test(
#     name = "tests",
#     size = "small",
#     srcs = glob(["tests/*.cpp"]),
#     deps = [
#         ":parser",
#         ":virtual_machine",
#         "@googletest//:gtest",
#         "@googletest//:gtest_main",
#     ],
# )

# cc_test(
#     name = "benchmarks",
#     size = "small",
#     srcs = glob(["tests/benchmarks/*.cpp"]),
#     deps = [
#         ":parser",
#         "@google_benchmark//:benchmark",
#         "@google_benchmark//:benchmark_main",
#         "@googletest//:gtest",
#         "@googletest//:gtest_main",
#     ],
# )
