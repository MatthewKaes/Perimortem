# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
The Library Dialect bundle
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "language",
    srcs = glob([
        "builtin/**/*.cpp",
        "language/**/*.cpp",
    ]),
    hdrs = glob([
        "builtin/**/*.hpp",
        "language/**/*.hpp",
    ]),
    deps = [
        "//perimortem",
        "//tetrodotoxin:language",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "interpreter",
    srcs = glob([
        "interpreter/**/*.cpp",
    ]),
    hdrs = glob([
        "interpreter/**/*.hpp",
    ]),
    deps = [
        ":language",
        "//perimortem",
        "//tetrodotoxin:language",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "archive",
    srcs = glob([
        "archive/**/*.cpp",
    ]),
    hdrs = glob([
        "archive/**/*.hpp",
    ]),
    deps = [
        ":language",
        "//perimortem",
        "//tetrodotoxin:language",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "library",
    srcs = ["dialect.cpp"],
    hdrs = ["dialect.hpp"],
    deps = [
        ":archive",
        ":interpreter",
        ":language",
        "//perimortem",
        "//tetrodotoxin:language",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)
