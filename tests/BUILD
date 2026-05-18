"""
Tests - Includes all unit tests
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//tetrodotoxin:ttx.bzl", "ttx_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_headers",
    hdrs = glob([
        "**/*.hpp",
    ]),
    includes = ["."],
)

ttx_library(
    name = "ttx_tests",
    srcs = glob([
        "unit_tests/**/*.ttx",
    ]),
)

cc_binary(
    name = "unit_tests",
    srcs = glob([
        "unit_tests/**/*.cpp",
    ]),
    includes = ["."],
    deps = [
        ":test_headers",
        ":ttx_tests",
        "//perimortem",
        "//tetrodotoxin:compiler",
        "//validation:unit_test",
    ],
)
