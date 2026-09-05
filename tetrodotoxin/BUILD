# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "package",
    srcs = glob([
        "package/**/*.cpp",
    ]),
    hdrs = glob([
        "package/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":environment_provider_abi",
        ":linker",
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/library",
        "//tetrodotoxin/terminal:provider",
        "//ttx",
    ],
)

cc_library(
    name = "linker",
    srcs = glob([
        "linker/**/*.cpp",
    ]),
    hdrs = glob([
        "linker/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem:headless",
    ],
)

cc_library(
    name = "formatting",
    srcs = glob([
        "formatting/**/*.cpp",
    ]),
    hdrs = glob([
        "formatting/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx",
    ],
)

alias(
    name = "library",
    actual = "//tetrodotoxin/library",
)

cc_library(
    name = "diagnostics",
    srcs = glob([
        "diagnostics/**/*.cpp",
    ]),
    hdrs = glob([
        "diagnostics/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem:headless",
    ],
)

cc_library(
    name = "environment_provider_abi",
    hdrs = ["environment/provider.h"],
    deps = [
        "//tetrodotoxin/terminal:provider_abi",
        "//ttx:abi",
    ],
)

cc_library(
    name = "environment",
    srcs = glob([
        "environment/**/*.cpp",
    ]),
    hdrs = glob([
        "environment/**/*.h",
        "environment/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":package",
        ":plugin",
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/terminal:native_toolchain",
        "//tetrodotoxin/terminal:provider",
        "//ttx",
    ],
)

cc_library(
    name = "build",
    srcs = glob(["build/**/*.cpp"]),
    hdrs = glob(["build/**/*.hpp"]),
    includes = ["."],
    deps = [
        ":environment",
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx",
    ],
)

cc_library(
    name = "plugin_abi",
    hdrs = ["plugin/abi.h"],
    deps = [
        "//tetrodotoxin/language:provider_abi",
        "//tetrodotoxin/terminal:provider_abi",
        "//ttx:abi",
    ],
)

cc_library(
    name = "plugin",
    srcs = glob(["plugin/**/*.cpp"]),
    hdrs = glob([
        "plugin/**/*.h",
        "plugin/**/*.hpp",
    ]),
    includes = ["."],
    linkopts = ["-ldl"],
    deps = [
        ":plugin_abi",
        "//tetrodotoxin/language",
        "//tetrodotoxin/language:provider_abi",
        "//tetrodotoxin/terminal:provider_abi",
        "//ttx",
    ],
)
