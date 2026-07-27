"""Puffer - Tetrodotoxin language-server host."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lsp",
    srcs = glob([
        "lsp/**/*.cpp",
    ]),
    hdrs = glob([
        "lsp/**/*.hpp",
    ]),
    deps = [
        "//perimortem",
        "//ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "main.cpp",
    ],
    deps = [
        ":lsp",
        "//perimortem",
        "//ttx",
    ],
)
