# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Shared Tetrodotoxin language contracts and TTX parser fragments."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "language",
    srcs = glob(["**/*.c", "**/*.cpp"]),
    hdrs = glob(["**/*.h", "**/*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//ttx:bootstrap_concept",
        "//ttx:lexical",
        "//ttx:bootstrap_model",
    ],
)
