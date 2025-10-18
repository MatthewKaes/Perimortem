"""
Tetordotoxin Language Server - An Lsp implementation for TTX to allow easy tooling integration.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ttx-service",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob(["src/*.hpp"]),
    includes = ["."],
    deps = [
        "//tetrodotoxin:parser",
    ],
)

cc_binary(
    name = "ttx-lang-server",
    srcs = ["main.cpp"],
    includes = ["."],
    deps = [
        ":ttx-service",
    ],
)

cc_test(
    name = "tests",
    size = "small",
    srcs = glob(["tests/*.cpp"]),
    deps = [
        ":ttx-service",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
