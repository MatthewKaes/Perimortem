"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "parser",
    srcs = glob([
        "parser/*.cpp",
        "parser/ast/*.cpp",
    ]),
    hdrs = glob([
        "parser/*.hpp",
        "parser/ast/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//core",
    ],
)

cc_test(
    name = "tests",
    size = "small",
    srcs = glob(["parser/tests/*.cpp"]),
    copts = ["-I.bin/Perimortem/external/google_benchmark+/include"],
    cxxopts = ["-I.bin/Perimortem/external/google_benchmark+/include"],
    deps = [
        ":parser",
        "@google_benchmark//:benchmark",
        "@google_benchmark//:benchmark_main",
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

