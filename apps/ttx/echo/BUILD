# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Canonical Echo TTX application sources."""

load("//toolchain:tetrodotoxin.bzl", "ttx_application", "ttx_package")

package(default_visibility = ["//visibility:private"])

ttx_package(
    name = "package",
    package_name = "Perimortem.Echo",
    manifest = "package.ttx",
    version = [
        1,
        0,
    ],
    visibility = ["//visibility:public"],
    deps = ["//packages/ttx:perimortem_system"],
)

ttx_application(
    name = "echo",
    app_member = "AppSource",
    package = ":package",
    visibility = ["//visibility:public"],
)
