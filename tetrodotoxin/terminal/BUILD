# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Terminal producers for completed Tetrodotoxin graphs."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "abi",
    srcs = glob([
        "abi/**/*.cpp",
    ]),
    hdrs = glob([
        "abi/**/*.hpp",
    ]),
    deps = [
        "//perimortem",
        "//tetrodotoxin/language",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//ttx:concept",
        "//ttx:model",
    ],
)

cc_library(
    name = "llvm",
    srcs = glob([
        "llvm/**/*.cpp",
    ]),
    hdrs = glob([
        "llvm/**/*.hpp",
    ]),
    deps = [
        ":abi",
        "//perimortem",
        "//tetrodotoxin/language",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//toolchain/llvm:sdk",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)

cc_library(
    name = "spirv",
    srcs = glob([
        "spirv/**/*.cpp",
    ]),
    hdrs = glob([
        "spirv/**/*.hpp",
    ]),
    deps = [
        "//perimortem",
    ],
)
