# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
TTX language core.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "abi",
    srcs = [
        "abi.cpp",
        "context.cpp",
        "query.cpp",
    ],
    hdrs = [
        "abi.h",
        "query.hpp",
    ],
    copts = [
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
)

cc_library(
    name = "cpp",
    srcs = [
        "abstract.cpp",
        "alias.cpp",
        "callable.cpp",
        "composite.cpp",
        "domain.cpp",
        "fluid.cpp",
        "fitting.cpp",
        "finite_extent.cpp",
        "layout.cpp",
        "named.cpp",
        "ranged.cpp",
        "route.cpp",
        "value_layout.cpp",
    ],
    hdrs = [
        "abstract.hpp",
        "alias.hpp",
        "callable.hpp",
        "composite.hpp",
        "domain.hpp",
        "fluid.hpp",
        "fitting.hpp",
        "finite_extent.hpp",
        "layout.hpp",
        "named.hpp",
        "ranged.hpp",
        "route.hpp",
        "value_layout.hpp",
    ],
    deps = [":abi"],
)

cc_library(
    name = "lexical",
    srcs = glob([
        "lexical/**/*.cpp",
    ]),
    hdrs = glob([
        "lexical/**/*.hpp",
    ]),
    deps = [
        ":bootstrap_concept",
        "//perimortem:headless",
    ],
)

cc_library(
    name = "concept",
    srcs = glob([
        "concept/**/*.c",
        "concept/**/*_internal.h",
    ]),
    hdrs = glob([
        "concept/**/*.h",
    ], exclude = ["concept/**/*_internal.h"]),
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = [
        "//perimortem:core",
    ],
)

cc_library(
    name = "bootstrap_concept",
    srcs = glob([
        "bootstrap/concept/**/*.cpp",
    ]),
    hdrs = glob([
        "bootstrap/concept/**/*.hpp",
        "concept/**/*.h",
    ]),
    deps = [
        ":concept",
        "//perimortem:headless",
    ],
)

cc_library(
    name = "model",
    srcs = glob([
        "model/**/*.c",
        "model/**/*_internal.h",
    ]),
    hdrs = glob([
        "model/**/*.h",
    ], exclude = ["model/**/*_internal.h"]),
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    deps = [
        ":concept",
    ],
)

cc_library(
    name = "bootstrap_model",
    srcs = glob([
        "bootstrap/model/**/*.cpp",
    ]),
    hdrs = glob([
        "bootstrap/model/**/*.hpp",
        "model/**/*.h",
    ]),
    deps = [
        ":bootstrap_concept",
        ":model",
        "//perimortem:headless",
    ],
)

cc_library(
    name = "ttx",
    deps = [
        ":concept",
        ":model",
        "//perimortem:core",
    ],
)

cc_library(
    name = "bootstrap",
    deps = [
        ":bootstrap_concept",
        ":bootstrap_model",
        ":lexical",
        "//perimortem:headless",
    ],
)
