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
        "language/**/*.c",
        "builtin/**/*.cpp",
        "language/**/*.cpp",
    ]),
    hdrs = glob([
        "language/**/*.h",
        "builtin/**/*.hpp",
        "language/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx:bootstrap_concept",
        "//ttx:lexical",
        "//ttx:bootstrap_model",
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
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx:bootstrap_concept",
        "//ttx:lexical",
        "//ttx:bootstrap_model",
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
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx:bootstrap_concept",
        "//ttx:lexical",
        "//ttx:bootstrap_model",
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
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx:bootstrap_concept",
        "//ttx:lexical",
        "//ttx:bootstrap_model",
    ],
)
