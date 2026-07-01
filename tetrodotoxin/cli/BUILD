"""
Tetrodotoxin command line tools.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "tetrodotoxin",
    srcs = ["main.cpp"],
    deps = [
        "//perimortem",
        "//ttx/lexical:lexical",
        "//ttx:parse",
    ],
)
