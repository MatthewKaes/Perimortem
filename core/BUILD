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
        ":resource",
        ":storage",
    ],
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
    srcs = glob(["storage/*.cpp"]),
    hdrs = glob(["storage/*.hpp"]),
    includes = ["."],
    deps = ["@zstd"],
)

cc_test(
    name = "tests",
    size = "small",
    srcs = [
        "resource/tests/manager_tests.cpp",
        "storage/tests/virtual_disk_reader_tests.cpp",
        "storage/tests/virtual_disk_writer_tests.cpp",
    ],
    deps = [
        ":core",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
