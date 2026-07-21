"""
Puffer - Tetrodotoxin CLI, source resolver, Boot ISA, and LSP server.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "package_descriptor",
    srcs = [
        "package/descriptor.cpp",
    ],
    hdrs = [
        "package/descriptor.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//tetrodotoxin:interpreter",
        "//tetrodotoxin:model",
        "//ttx:concept",
        "//ttx:lexical",
    ],
)

cc_library(
    name = "package_repository",
    srcs = [
        "resolution/package/repository.cpp",
    ],
    hdrs = [
        "resolution/package/repository.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//tetrodotoxin:model",
        "//tetrodotoxin/archiver",
        "//ttx:concept",
    ],
)

cc_library(
    name = "package_materializer",
    srcs = [
        "package/materializer.cpp",
    ],
    hdrs = [
        "package/materializer.hpp",
    ],
    include_prefix = "tetrodotoxin/puffer",
    strip_include_prefix = ".",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:system",
        "//tetrodotoxin:model",
        "//tetrodotoxin/archiver",
    ],
)

# cc_library(
#     name = "toolchain",
#     srcs = [
#         "toolchain.cpp",
#     ],
#     hdrs = [
#         "toolchain.hpp",
#     ],
#     include_prefix = "tetrodotoxin/puffer",
#     strip_include_prefix = ".",
#     deps = [
#         "//perimortem:core",
#         "//perimortem:memory",
#         "//tetrodotoxin:compiler",
#         "//tetrodotoxin:isa",
#         "//tetrodotoxin:isa_lowerers",
#         "//ttx",
#     ],
# )

# cc_library(
#     name = "resolution",
#     srcs = [
#         "resolution/context.cpp",
#         "resolution/package/buffer.cpp",
#         "resolution/package/cache.cpp",
#         "resolution/resolver.cpp",
#         "resolution/source/cache.cpp",
#         "resolution/source/roots.cpp",
#         "resolution/source/storage.cpp",
#     ],
#     hdrs = [
#         "resolution/context.hpp",
#         "resolution/package/buffer.hpp",
#         "resolution/package/cache.hpp",
#         "resolution/package/restored.hpp",
#         "resolution/resolver.hpp",
#         "resolution/source/cache.hpp",
#         "resolution/source/record.hpp",
#         "resolution/source/roots.hpp",
#         "resolution/source/snapshot.hpp",
#         "resolution/source/storage.hpp",
#     ],
#     include_prefix = "tetrodotoxin/puffer",
#     strip_include_prefix = ".",
#     deps = [
#         ":boot",
#         "//perimortem:core",
#         "//perimortem:memory",
#         "//perimortem:system",
#         "//perimortem:utility",
#         "//tetrodotoxin:isa",
#         "//tetrodotoxin/archiver",
#         "//ttx",
#     ],
# )

# cc_library(
#     name = "lsp",
#     srcs = glob([
#         "lsp/**/*.cpp",
#     ]),
#     hdrs = glob([
#         "lsp/**/*.hpp",
#     ]),
#     include_prefix = "tetrodotoxin/puffer",
#     strip_include_prefix = ".",
#     deps = [
#         ":boot",
#         ":toolchain",
#         "//perimortem:core",
#         "//perimortem:memory",
#         "//perimortem:serialization",
#         "//perimortem:utility",
#         "//tetrodotoxin:isa",
#         "//ttx",
#     ],
# )

# cc_library(
#     name = "compiler",
#     srcs = [
#         "compiler.cpp",
#     ],
#     hdrs = [
#         "compiler.hpp",
#     ],
#     include_prefix = "tetrodotoxin/puffer",
#     strip_include_prefix = ".",
#     deps = [
#         ":boot",
#         ":package_builder",
#         ":resolution",
#         ":toolchain",
#         "//perimortem:core",
#         "//perimortem:memory",
#         "//perimortem:system",
#         "//tetrodotoxin:compiler",
#         "//tetrodotoxin:isa",
#         "//tetrodotoxin/abi",
#         "//tetrodotoxin/archiver",
#         "//tetrodotoxin/standard:types",
#         "//ttx",
#     ],
# )

# cc_binary(
#     name = "puffer",
#     srcs = [
#         "main.cpp",
#     ],
#     deps = [
#         ":compiler",
#         ":lsp",
#         ":resolution",
#         "//perimortem:core",
#         "//perimortem:memory",
#         "//perimortem:system",
#         "//ttx",
#     ],
# )
