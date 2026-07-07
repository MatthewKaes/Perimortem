"""
Puffer - Tetrodotoxin CLI, source resolver, Boot ISA, and LSP server.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "boot",
    srcs = glob([
        "isa/boot/**/*.cpp",
    ]),
    hdrs = glob([
        "isa/boot/**/*.hpp",
    ]),
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
        "//tetrodotoxin:isa",
        "//ttx",
    ],
)

cc_library(
    name = "resolution",
    srcs = [
        "resolution/resolver.cpp",
        "resolution/source/cache.cpp",
    ],
    hdrs = [
        "resolution/resolver.hpp",
        "resolution/source/cache.hpp",
        "resolution/source/record.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        ":boot",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//perimortem:utility",
        "//tetrodotoxin:isa",
        "//tetrodotoxin:toolchain",
        "//ttx",
    ],
)

cc_library(
    name = "lsp",
    srcs = glob([
        "lsp/**/*.cpp",
    ]),
    hdrs = glob([
        "lsp/**/*.hpp",
    ]),
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        ":boot",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:utility",
        "//tetrodotoxin:isa",
        "//tetrodotoxin:toolchain",
        "//ttx",
    ],
)

cc_library(
    name = "terminal",
    srcs = [
        "terminal/plan.cpp",
    ],
    hdrs = [
        "terminal/plan.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        ":resolution",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
        "//tetrodotoxin:compiler",
        "//ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "main.cpp",
    ],
    deps = [
        ":boot",
        ":lsp",
        ":resolution",
        ":terminal",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:toolchain",
        "//ttx",
    ],
)
