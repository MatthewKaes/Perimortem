# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

def c_header(name, header, deps = ["//ttx:ttx"]):
    cc_library(
        name = "c_header_" + name,
        srcs = ["c/header.c"],
        copts = [
            "-std=c17",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-DTTX_HEADER=\\\"" + header + "\\\"",
        ],
        deps = deps,
    )
