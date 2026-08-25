# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Durable Scene query surfaces."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "archive",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/library",
        "//tetrodotoxin/library:archive",
        "//tetrodotoxin/library:language",
        "//tetrodotoxin/scene/language",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)
