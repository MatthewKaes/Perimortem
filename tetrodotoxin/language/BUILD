# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Shared Tetrodotoxin language contracts and TTX parser fragments."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "artifact_file_abi",
    hdrs = ["artifact_file.h"],
    deps = [
        "//tetrodotoxin/terminal:provider_abi",
        "//ttx:abi",
    ],
)

cc_library(
    name = "provider_abi",
    hdrs = [
        "error.h",
        "production.h",
        "provider.h",
    ],
    deps = ["//ttx:abi"],
)

cc_library(
    name = "provider",
    srcs = [
        "provider.cpp",
        "query.cpp",
    ],
    hdrs = [
        "provider.h",
        "query.hpp",
    ],
    deps = [
        ":provider_abi",
        "//tetrodotoxin/terminal:provider_abi",
        "//ttx",
    ],
)

cc_library(
    name = "language",
    srcs = glob(
        ["**/*.cpp"],
        exclude = [
            "provider.cpp",
            "query.cpp",
        ],
    ),
    hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    deps = [
        ":artifact_file_abi",
        ":provider",
        "//perimortem:headless",
        "//tetrodotoxin/terminal:provider_abi",
        "//ttx",
    ],
)
