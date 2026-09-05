# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Independent graphics runtime interfaces and frame submission."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "graphics",
    srcs = glob(["runtime/**/*.cpp"]),
    hdrs = glob(["runtime/**/*.hpp"]),
    deps = [
        "//perimortem:core",
        "//perimortem:graphics",
        "//perimortem:memory",
    ],
)
