# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Durable App policy surfaces."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "archive",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/app/language",
        "//tetrodotoxin/language",
        "//tetrodotoxin/scene/language",
        "//ttx:bootstrap_concept",
    ],
)
