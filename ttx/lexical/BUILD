"""
TTX lexical analysis.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lexical",
    srcs = [
        "class.cpp",
        "tokenizer.cpp",
    ],
    hdrs = [
        "class.hpp",
        "token.hpp",
        "tokenizer.hpp",
    ],
    include_prefix = "ttx/lexical",
    deps = [
        "//perimortem",
    ],
)
