"""
Tetordotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "parser",
    srcs = glob(["parser/**/*.cpp"]),
    hdrs = glob(["parser/**/*.hpp"]),
    includes = ["."],
    deps = [
        "//core",
    ],
)

cc_test(
    name = "tests",
    size = "small",
    srcs = glob(["parser/tests/*.cpp"]),
    deps = [
        ":parser",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
