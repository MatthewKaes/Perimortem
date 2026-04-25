"""
Perimortem::Core - Home to all common utilities and classes
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test",
    srcs = glob([
        "test/*.cpp",
    ]),
    hdrs = glob([
        "test/*.hpp",
    ]),
    includes = ["."],
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
