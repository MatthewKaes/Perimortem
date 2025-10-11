"""
Tetordotoxin Language Server - An LSP implementation for TTX to allow easy tooling integration.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "ttx-lang-server",
    srcs = glob(["**/*.cpp"]),
    # hdrs = glob(["**/*.hpp"]),
    includes = ["."],
    deps = [
        "//tetrodotoxin:parser",
    ],
)