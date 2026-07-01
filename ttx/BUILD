"""
TTX language core.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

alias(
    name = "lexical",
    actual = "//ttx/lexical:lexical",
)

cc_library(
    name = "dialect",
    srcs = glob([
        "dialect/*.cpp",
    ]),
    hdrs = glob([
        "dialect/*.hpp",
    ]),
    include_prefix = "ttx",
    deps = [
        ":parse",
        "//perimortem",
    ],
)

cc_library(
    name = "documentation",
    hdrs = [
        "documentation.hpp",
    ],
    include_prefix = "ttx",
    deps = [
        "//perimortem",
    ],
)

cc_library(
    name = "parse",
    srcs = glob([
        "parse/*.cpp",
    ]),
    hdrs = glob([
        "parse/*.hpp",
    ]),
    include_prefix = "ttx",
    deps = [
        ":documentation",
        ":lexical",
        "//perimortem",
    ],
)

cc_library(
    name = "type",
    srcs = [
        "type.cpp",
    ],
    hdrs = [
        "type.hpp",
    ],
    include_prefix = "ttx",
    deps = [
        ":documentation",
        "//perimortem",
    ],
)

cc_library(
    name = "layout",
    srcs = [
        "layout.cpp",
    ],
    hdrs = [
        "layout.hpp",
    ],
    include_prefix = "ttx",
    deps = [
        ":type",
        "//perimortem",
    ],
)

cc_library(
    name = "core",
    hdrs = [
        "core/prelude.hpp",
    ],
    include_prefix = "ttx",
    deps = [
        ":type",
        "//perimortem",
    ],
)
