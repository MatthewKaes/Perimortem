# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""The Scene Dialect."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "scene",
    srcs = glob(
        ["**/*.cpp"],
        exclude = ["plugin.cpp"],
    ),
    hdrs = glob(["**/*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:language",
        "//tetrodotoxin/library",
        "//ttx",
    ],
)

cc_binary(
    name = "plugin.so",
    srcs = ["plugin.cpp"],
    linkshared = True,
    linkstatic = True,
    deps = [
        ":scene",
        "//tetrodotoxin:plugin",
    ],
)
