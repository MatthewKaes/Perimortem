# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Canonical multi Scene TTX application."""

load("//toolchain:tetrodotoxin.bzl", "ttx_source")

package(default_visibility = ["//visibility:public"])

ttx_source(
    name = "package",
    src = "package.ttx",
    deps = [
        "//packages/ttx:perimortem_graphics",
        "//packages/ttx:perimortem_math",
        "//packages/ttx:perimortem_system",
    ],
)

exports_files(["build.ttx"])
