"""Puffer - Tetrodotoxin language-server host."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "position_encoding",
    srcs = ["lsp/position_encoding.cpp"],
    hdrs = ["lsp/position_encoding.hpp"],
    deps = ["//perimortem"],
)

cc_library(
    name = "lsp",
    srcs = glob([
        "lsp/**/*.cpp",
    ], exclude = ["lsp/position_encoding.cpp"]),
    hdrs = glob([
        "lsp/**/*.hpp",
    ], exclude = ["lsp/position_encoding.hpp"]),
    deps = [
        ":position_encoding",
        "//perimortem",
        "//tetrodotoxin:app",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:formatting",
        "//tetrodotoxin:library",
        "//tetrodotoxin:package",
        "//tetrodotoxin:scene",
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
        "//backend:llvm",
        "//perimortem",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:app",
        "//tetrodotoxin:library",
        "//tetrodotoxin:package",
        "//ttx",
    ],
)
