# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""The Library Dialect."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "library",
    srcs = glob(
        [
            "archive/**/*.cpp",
            "builtin/**/*.cpp",
            "interpreter/**/*.cpp",
            "language/**/*.cpp",
            "*.cpp",
        ],
        exclude = ["plugin.cpp"],
    ),
    hdrs = glob([
        "archive/**/*.hpp",
        "builtin/**/*.hpp",
        "interpreter/**/*.hpp",
        "language/**/*.hpp",
        "*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx",
    ],
)

cc_binary(
    name = "plugin.so",
    srcs = ["plugin.cpp"],
    linkshared = True,
    linkstatic = True,
    deps = [
        ":library",
        "//tetrodotoxin:plugin",
    ],
)
