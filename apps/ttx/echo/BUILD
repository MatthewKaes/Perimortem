# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Canonical Echo TTX application sources."""

load("//toolchain:tetrodotoxin.bzl", "ttx_source")

package(default_visibility = ["//visibility:private"])

ttx_source(
    name = "package",
    src = "package.ttx",
    visibility = ["//visibility:public"],
    deps = ["//packages/ttx:perimortem_system"],
)
