"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

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
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "model",
    srcs = glob([
        "model/**/*.cpp",
    ]),
    hdrs = glob([
        "model/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "parser",
    srcs = glob([
        "parser/**/*.cpp",
    ]),
    hdrs = glob([
        "parser/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
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
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "archiver",
    srcs = glob([
        "archiver/**/*.cpp",
    ]),
    hdrs = glob([
        "archiver/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "compiler",
    srcs = glob([
        "compiler/**/*.cpp",
    ]),
    hdrs = glob([
        "compiler/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
        "//ttx",
    ],
)
