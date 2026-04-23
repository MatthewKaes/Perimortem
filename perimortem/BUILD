"""
Perimortem::Core - Home to all common utilities and classes
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "perimortem",
    # srcs = glob(["*.cpp"]),
    # hdrs = glob(["*.hpp"]),
    includes = ["."],
    deps = [
        ":core",
        ":memory",
        ":serialization",
        ":system",
    ],
)

cc_library(
    name = "core",
    srcs = [],
    hdrs = glob([
        "core/access/*.hpp",
        "core/view/*.hpp",
        "core/*.hpp",
    ]),
    includes = ["."],
)

cc_library(
    name = "utility",
    srcs = [],
    hdrs = glob([
        "utility/extensions/*.hpp",
        "utility/func/*.hpp",
        "utility/type/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
    ],
)

cc_library(
    name = "memory",
    srcs = glob([
        "memory/allocator/*.cpp",
        "memory/dynamic/*.cpp",
        "memory/managed/*.cpp",
    ]),
    hdrs = glob([
        "memory/allocator/*.hpp",
        "memory/dynamic/*.hpp",
        "memory/managed/*.hpp",
        "memory/static/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":utility",
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
        ":utility",
    ],
)

cc_library(
    name = "serialization",
    srcs = glob([
        # "serialization/archive/*.cpp",
        "serialization/base64/*.cpp",
        "serialization/binary/*.cpp",
        "serialization/textual/*.cpp",
        "serialization/json/*.cpp",
    ]),
    hdrs = glob([
        # "serialization/archive/*.hpp",
        "serialization/base64/*.hpp",
        "serialization/binary/*.hpp",
        "serialization/textual/*.hpp",
        "serialization/json/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":memory",
        ":system",
        ":utility",
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

cc_binary(
    name = "__tests",
    srcs = glob([
        "__tests/memory/*.cpp",
    ]),
    deps = [
        ":perimortem",
        "//validation:test",
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
