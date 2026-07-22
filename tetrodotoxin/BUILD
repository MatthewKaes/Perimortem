"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "diagnostics",
    srcs = [
        "diagnostics/suggestions.cpp",
    ],
    hdrs = [
        "diagnostics/suggestions.hpp",
    ],
    includes = ["."],
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//ttx:model",
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
        "//perimortem:core",
        "//perimortem:memory",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "interpreter",
    srcs = glob([
        "interpreter/**/*.cpp",
    ]),
    hdrs = glob([
        "interpreter/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":model",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "archiver",
    srcs = [
        "manifest.cpp",
        "reader.cpp",
        "writer.cpp",
    ],
    hdrs = [
        "dependency.hpp",
        "format.hpp",
        "manifest.hpp",
        "reader.hpp",
        "writer.hpp",
    ],
    strip_include_prefix = ".",
    deps = [
        "//perimortem",
        "//ttx",
    ],
)

cc_library(
    name = "linker",
    srcs = [
        "linker/linker.cpp",
        "linker/target/elf.cpp",
    ],
    hdrs = [
        "linker/linker.hpp",
        "linker/object/relocation.hpp",
        "linker/object/section.hpp",
        "linker/object/symbol.hpp",
        "linker/target/elf.hpp",
        "linker/target/format.hpp",
    ],
    includes = ["."],
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
    ],
)

cc_library(
    name = "compiler",
    srcs = [
        glob("compiler/**/*.cpp"),
    ],
    hdrs = [
        glob("compiler/**/*.hpp"),
    ],
    includes = ["."],
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
        "//ttx:concept",
        "//ttx:model",
    ],
)

cc_library(
    name = "runtime",
    srcs = glob(["runtime/*.cpp"]),
    hdrs = glob(["runtime/*.hpp"]),
    includes = ["."],
    deps = [
        ":model",
        "//perimortem:core",
        "//perimortem:memory",
        "//ttx:concept",
        "//ttx:model",
    ],
)
