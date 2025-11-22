"""
Perimortem::Core - Home to all common utilities and classes
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "core",
    # srcs = glob(["*.cpp"]),
    # hdrs = glob(["*.hpp"]),
    includes = ["."],
    deps = [
        ":memory",
        ":resource",
        ":storage",
    ],
)

cc_library(
    name = "memory",
    srcs = glob(["memory/*.cpp"]),
    hdrs = glob(["memory/*.hpp"]),
    includes = ["."],
)

cc_library(
    name = "resource",
    srcs = glob(["resource/*.cpp"]),
    hdrs = glob(["resource/*.hpp"]),
    includes = ["."],
    deps = [":storage"],
)

cc_library(
    name = "storage",
    srcs = glob([
        "storage/*.cpp",
        "storage/formats/*.cpp",
    ]),
    hdrs = glob([
        "storage/*.hpp",
        "storage/formats/*.hpp",
    ]),
    includes = ["."],
    deps = ["@zstd"],
)

cc_test(
    name = "tests",
    size = "small",
    srcs = glob([
        "tests/*.cpp",
    ]),
    deps = [
        ":core",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "benchmarks",
    size = "small",
    srcs = glob(["tests/benchmarks/*.cpp"]),
    deps = [
        ":core",
        "@google_benchmark//:benchmark",
        "@google_benchmark//:benchmark_main",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)