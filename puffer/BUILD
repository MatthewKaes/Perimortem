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
    name = "publisher",
    srcs = ["publisher.cpp"],
    hdrs = ["publisher.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:language",
        "//ttx",
    ],
)

cc_library(
    name = "invocation",
    srcs = ["invocation.cpp"],
    hdrs = ["invocation.hpp"],
    deps = [
        "//perimortem:headless",
        "//ttx",
    ],
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
        "//tetrodotoxin:build",
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
        "command.cpp",
        "command.hpp",
        "main.cpp",
        "source.cpp",
        "source.hpp",
    ],
    data = [
        "//sdk:providers",
        "//tetrodotoxin/app:plugin.so",
        "//tetrodotoxin/library:plugin.so",
        "//tetrodotoxin/render:plugin.so",
        "//tetrodotoxin/runtime:system_plugin.so",
        "//tetrodotoxin/scene:plugin.so",
        "//tetrodotoxin/shader:plugin.so",
        "//tetrodotoxin/terminal:graph_text_plugin.so",
        "//tetrodotoxin/terminal:native_plugin.so",
    ],
    linkopts = ["-rdynamic"],
    deps = [
        ":invocation",
        ":lsp",
        ":publisher",
        "//perimortem:headless",
        "//tetrodotoxin:build",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:library",
        "//tetrodotoxin:package",
        "//tetrodotoxin/app",
        "//tetrodotoxin/render",
        "//tetrodotoxin/scene",
        "//tetrodotoxin/shader",
        "//ttx",
    ],
)
