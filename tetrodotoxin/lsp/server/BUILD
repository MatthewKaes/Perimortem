"""
Tetordotoxin Language Server - An Lsp implementation for TTX to allow easy tooling integration.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rpc",
    srcs = glob(["rpc/*.cpp"]),
    hdrs = glob(["rpc/*.hpp"]),
    includes = ["."],
    deps = [
        "//perimortem:serialization",
        "//perimortem:system",
        "//tetrodotoxin:lexical",
    ],
)

cc_binary(
    name = "ttx-lang-server",
    srcs = [
        "main.cpp",
    ],
    includes = ["."],
    deps = [
        ":rpc",
        "//perimortem:serialization",
        "//perimortem:system",
        "//tetrodotoxin:lexical",
        "//tetrodotoxin:syntax",
    ],
)

#cc_test(
#name = "tests",
#size = "small",
#srcs = glob(["tests/*.cpp"]),
#deps = [
#":ttx-service",
#"@googletest//:gtest",
#"@googletest//:gtest_main",
#],
#)
