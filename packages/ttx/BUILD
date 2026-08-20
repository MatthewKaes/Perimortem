"""Canonical source Packages distributed with Tetrodotoxin."""

load("//toolchain:tetrodotoxin.bzl", "ttx_package")

package(default_visibility = ["//visibility:private"])

ttx_package(
    name = "perimortem_memory",
    package_name = "Perimortem.Memory",
    manifest = "Perimortem.Memory/package.ttx",
    version = [1, 0],
    visibility = ["//visibility:public"],
)

ttx_package(
    name = "perimortem_system",
    package_name = "Perimortem.System",
    manifest = "Perimortem.System/package.ttx",
    version = [1, 0],
    deps = [":perimortem_memory"],
    visibility = ["//visibility:public"],
)
