# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Canonical TTX ABI and its modern C++ reference implementation."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "abi",
    hdrs = [
        "abi.h",
        "query.hpp",
    ],
)

cc_library(
    name = "query",
    srcs = ["query.cpp"],
    hdrs = ["query.hpp"],
    copts = ["-fPIC"],
    deps = [":abi"],
)

# C17 defines the boundary shared with other languages. The C++ sources in this
# target are the reference owners behind that boundary, not a second object
# model. Lexical support participates here because a TTX frontend constructs
# the same Abstracts, Layouts, and Packs exposed by every other frontend.
cc_library(
    name = "ttx",
    srcs = glob(
        [
            "*.cpp",
            "concept/**/*.cpp",
            "lexical/**/*.cpp",
            "model/**/*.cpp",
        ],
        exclude = ["query.cpp"],
    ),
    hdrs = glob([
        "*.h",
        "*.hpp",
        "concept/**/*.hpp",
        "lexical/**/*.hpp",
        "model/**/*.hpp",
    ]),
    copts = [
        "-Wall",
        "-Wextra",
    ],
    deps = [
        ":query",
        "//perimortem:headless",
    ],
)
