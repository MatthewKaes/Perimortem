"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

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
        "//core",
    ],
)

cc_library(
    name = "parser",
    srcs = glob([
        "parser/*.cpp",
    ]),
    hdrs = glob([
        "parser/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":types",
        "//core",
    ],
)

cc_test(
    name = "tests",
    size = "small",
    srcs = glob(["parser/tests/*.cpp"]),
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
    srcs = glob(["parser/benchmarks/*.cpp"]),
    deps = [
        ":parser",
        "@google_benchmark//:benchmark",
        "@google_benchmark//:benchmark_main",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
