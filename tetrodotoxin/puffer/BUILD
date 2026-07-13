"""
Puffer - Tetrodotoxin CLI, source resolver, Boot ISA, and LSP server.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "toolchain",
    srcs = [
        "toolchain.cpp",
    ],
    hdrs = [
        "toolchain.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//tetrodotoxin:compiler",
        "//tetrodotoxin:isa",
        "//tetrodotoxin:isa_lowerers",
        "//ttx",
    ],
)

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
        "resolution/context.cpp",
        "resolution/package/buffer.cpp",
        "resolution/package/cache.cpp",
        "resolution/resolver.cpp",
        "resolution/source/cache.cpp",
        "resolution/source/record.cpp",
        "resolution/source/roots.cpp",
    ],
    hdrs = [
        "resolution/context.hpp",
        "resolution/package/buffer.hpp",
        "resolution/package/cache.hpp",
        "resolution/package/restored.hpp",
        "resolution/resolver.hpp",
        "resolution/source/cache.hpp",
        "resolution/source/record.hpp",
        "resolution/source/roots.hpp",
        "resolution/source/snapshot.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        ":boot",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//perimortem:utility",
        "//tetrodotoxin/archiver",
        "//tetrodotoxin:isa",
        "//ttx",
    ],
)

cc_library(
    name = "package_builder",
    srcs = [
        "package/builder.cpp",
    ],
    hdrs = [
        "package/builder.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        ":resolution",
        "//perimortem:core",
        "//perimortem:memory",
        "//tetrodotoxin/archiver",
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
        ":toolchain",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:utility",
        "//tetrodotoxin:isa",
        "//ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "compiler.cpp",
        "compiler.hpp",
        "main.cpp",
    ],
    deps = [
        ":boot",
        ":lsp",
        ":package_builder",
        ":resolution",
        ":toolchain",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//perimortem:utility",
        "//tetrodotoxin/archiver",
        "//tetrodotoxin:compiler",
        "//tetrodotoxin:isa",
        "//tetrodotoxin:linker",
        "//ttx",
    ],
)
