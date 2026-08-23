"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "language",
    srcs = glob([
        "language/**/*.cpp",
    ]),
    hdrs = glob([
        "language/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

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
        ":language",
        ":linker",
        "//perimortem",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
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
    ],
)

cc_library(
    name = "app",
    srcs = glob([
        "app/**/*.cpp",
    ]),
    hdrs = glob([
        "app/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":language",
        ":library",
        ":linker",
        "//perimortem",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "library",
    srcs = glob([
        "library/**/*.cpp",
    ]),
    hdrs = glob(
        [
            "library/**/*.hpp",
        ],
    ),
    includes = ["."],
    deps = [
        ":language",
        ":linker",
        "//perimortem",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "scene",
    srcs = glob([
        "scene/**/*.cpp",
    ]),
    hdrs = glob([
        "scene/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":language",
        ":library",
        "//perimortem",
        "//ttx:concept",
        "//ttx:lexical",
    ],
)

cc_library(
    name = "shader",
    srcs = glob([
        "shader/**/*.cpp",
    ]),
    hdrs = glob([
        "shader/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem",
    ],
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
        "//perimortem",
    ],
)

cc_library(
    name = "environment",
    srcs = glob([
        "environment/**/*.cpp",
    ]),
    hdrs = glob([
        "environment/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":language",
        ":package",
        "//perimortem",
        "//ttx:concept",
        "//ttx:model",
    ],
)
