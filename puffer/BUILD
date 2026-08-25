# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Puffer - Tetrodotoxin language-server host."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "position_encoding",
    srcs = ["lsp/position_encoding.cpp"],
    hdrs = ["lsp/position_encoding.hpp"],
    deps = ["//perimortem:headless"],
)

cc_library(
    name = "lsp",
    srcs = glob(
        [
            "lsp/**/*.cpp",
        ],
        exclude = ["lsp/position_encoding.cpp"],
    ),
    hdrs = glob(
        [
            "lsp/**/*.hpp",
        ],
        exclude = ["lsp/position_encoding.hpp"],
    ),
    deps = [
        ":position_encoding",
        "//perimortem:headless",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:formatting",
        "//tetrodotoxin:library",
        "//tetrodotoxin:package",
        "//tetrodotoxin/app",
        "//tetrodotoxin/render",
        "//tetrodotoxin/scene",
        "//tetrodotoxin/shader",
        "//ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "application.cpp",
        "application.hpp",
        "command.cpp",
        "command.hpp",
        "main.cpp",
        "package.cpp",
        "package.hpp",
    ],
    deps = [
        ":lsp",
        "//perimortem:headless",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:package",
        "//tetrodotoxin/app",
        "//tetrodotoxin/render",
        "//tetrodotoxin/scene",
        "//tetrodotoxin/shader",
        "//tetrodotoxin/terminal:application",
        "//tetrodotoxin/terminal:graphics",
        "//tetrodotoxin/terminal:llvm",
        "//tetrodotoxin/terminal:spirv",
        "//tetrodotoxin/terminal:vulkan",
        "//ttx",
    ],
)
