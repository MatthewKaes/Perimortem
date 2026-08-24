# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Canonical source Packages distributed with Tetrodotoxin."""

load(
    "//toolchain:tetrodotoxin.bzl",
    "ttx_cpp_api",
    "ttx_native_provider",
    "ttx_package",
)

package(default_visibility = ["//visibility:private"])

ttx_package(
    name = "perimortem_memory",
    package_name = "Perimortem.Memory",
    cpp_header = "perimortem/memory/dynamic/bytes.hpp",
    manifest = "Perimortem.Memory/package.ttx",
    version = [
        1,
        0,
    ],
    visibility = ["//visibility:public"],
)

ttx_cpp_api(
    name = "perimortem_memory_cpp",
    package = ":perimortem_memory",
    visibility = ["//visibility:public"],
)

ttx_package(
    name = "perimortem_math",
    package_name = "Perimortem.Math",
    manifest = "Perimortem.Math/package.ttx",
    version = [
        1,
        0,
    ],
    visibility = ["//visibility:public"],
)

ttx_package(
    name = "perimortem_graphics",
    package_name = "Perimortem.Graphics",
    manifest = "Perimortem.Graphics/package.ttx",
    native_deps = [":perimortem_graphics_native"],
    version = [
        1,
        0,
    ],
    visibility = ["//visibility:public"],
)

ttx_native_provider(
    name = "perimortem_graphics_native",
    dep = "//perimortem:abi_graphics",
    functions = [
        "perimortem_graphics_image_decode",
    ],
    provider_name = "Perimortem.Graphics.Host",
)

ttx_native_provider(
    name = "perimortem_system_native",
    dep = "//perimortem:abi_system",
    functions = [
        "perimortem_system_terminal_read_line",
        "perimortem_system_terminal_write_line",
    ],
    provider_name = "Perimortem.System.Host",
)

ttx_package(
    name = "perimortem_system",
    package_name = "Perimortem.System",
    manifest = "Perimortem.System/package.ttx",
    native_deps = [":perimortem_system_native"],
    version = [
        1,
        0,
    ],
    visibility = ["//visibility:public"],
    deps = [":perimortem_memory"],
)
