"""
Perimortem::Core - Home to all common utilities and classes
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "perimortem",
    # srcs = glob(["*.cpp"]),
    # hdrs = glob(["*.hpp"]),
    includes = ["."],
    deps = [
        ":memory",
        ":storage",
    ],
)

cc_library(
    name = "memory",
    srcs = glob([
        "memory/allocator/*.cpp",
        "memory/dynamic/*.cpp",
        "memory/view/*.cpp",
    ]),
    hdrs = glob([
        "memory/allocator/*.hpp",
        "memory/const/*.hpp",
        "memory/dynamic/*.hpp",
        "memory/managed/*.hpp",
        "memory/view/*.hpp",
    ]),
    includes = ["."],
)

cc_library(
    name = "storage",
    srcs = glob([
        "storage/*.cpp",
        "storage/archive/*.cpp",
        "storage/formats/*.cpp",
    ]),
    hdrs = glob([
        "storage/*.hpp",
        "storage/archive/*.hpp",
        "storage/formats/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":memory",
        "@zstd",
    ],
)

cc_test(
    name = "__tests",
    size = "small",
    srcs = glob([
        "__tests/*.cpp",
    ]),
    deps = [
        ":perimortem",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "__benchmarks",
    size = "small",
    srcs = glob(["__benchmarks/*.cpp"]),
    deps = [
        ":perimortem",
        "@google_benchmark//:benchmark",
        "@google_benchmark//:benchmark_main",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
