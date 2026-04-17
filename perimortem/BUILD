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
        ":core",
        ":memory",
        ":storage",
        ":system",
    ],
)

cc_library(
    name = "core",
    srcs = [],
    hdrs = glob([
        "core/*.hpp",
    ]),
    includes = ["."],
)

cc_library(
    name = "memory",
    srcs = glob([
        "memory/allocator/*.cpp",
        "memory/dynamic/*.cpp",
        "memory/managed/*.cpp",
        "memory/view/*.cpp",
    ]),
    hdrs = glob([
        "memory/allocator/*.hpp",
        "memory/dynamic/*.hpp",
        "memory/managed/*.hpp",
        "memory/static/*.hpp",
        "memory/view/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
    ],
)

cc_library(
    name = "system",
    srcs = glob([
        "system/*.cpp",
    ]),
    hdrs = glob([
        "system/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":memory",
    ],
)

cc_library(
    name = "storage",
    srcs = glob([
        "storage/serialization/archive/*.cpp",
        "storage/serialization/*.cpp",
    ]),
    hdrs = glob([
        "storage/serialization/archive/*.hpp",
        "storage/serialization/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":memory",
        ":system",
        "@zstd",
    ],
)

# cc_library(
#     name = "system",
#     srcs = glob([
#         "system/*.cpp",
#     ]),
#     hdrs = glob([
#         "system/*.hpp",
#     ]),
#     includes = ["."],
#     deps = [
#         ":memory",
#     ],
# )

cc_test(
    name = "__tests",
    size = "small",
    srcs = glob([
        "__tests/**/*.cpp",
    ]),
    deps = [
        ":perimortem",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# cc_test(
#     name = "__benchmarks",
#     size = "small",
#     srcs = glob(["__benchmarks/*.cpp"]),
#     deps = [
#         ":perimortem",
#         "@google_benchmark//:benchmark",
#         "@google_benchmark//:benchmark_main",
#         "@googletest//:gtest",
#         "@googletest//:gtest_main",
#     ],
# )
