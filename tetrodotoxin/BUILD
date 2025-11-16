"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

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
        "//core",
    ],
)

cc_library(
    name = "types",
    srcs = glob([
        "types/*.cpp",
    ]),
    hdrs = glob([
        "types/*.hpp",
        "types/std/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":lexical",
        "//core",
    ],
)

cc_library(
    name = "parser",
    srcs = glob([
        "parser/*.cpp",
        "parser/visitor/*.cpp",
    ]),
    hdrs = glob([
        "parser/*.hpp",
        "parser/visitor/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":lexical",
        ":types",
        "//core",
    ],
)

cc_test(
    name = "tests",
    size = "small",
    srcs = glob(["tests/*.cpp"]),
    deps = [
        ":parser",
        ":types",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "benchmarks",
    size = "small",
    srcs = glob(["tests/benchmarks/*.cpp"]),
    deps = [
        ":parser",
        "@google_benchmark//:benchmark",
        "@google_benchmark//:benchmark_main",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
