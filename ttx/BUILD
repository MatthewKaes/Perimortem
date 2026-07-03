"""
TTX language core.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lexical",
    srcs = [
        "lexical/class.cpp",
        "lexical/cursor.cpp",
        "lexical/tokenizer.cpp",
    ],
    hdrs = [
        "lexical/class.hpp",
        "lexical/cursor.hpp",
        "lexical/error.hpp",
        "lexical/token.hpp",
        "lexical/tokenizer.hpp",
    ],
    include_prefix = "ttx/lexical",
    deps = [
        "//perimortem",
    ],
)

cc_library(
    name = "model",
    srcs = [
        "layout.cpp",
        "type.cpp",
    ],
    hdrs = [
        "documentation.hpp",
        "layout.hpp",
        "type.hpp",
    ],
    deps = [
        ":lexical",
        "//perimortem",
    ],
)

cc_library(
    name = "dialect",
    srcs = glob([
        "dialect/*.cpp",
    ]),
    hdrs = glob([
        "dialect/*.hpp",
    ]),
    deps = [
        ":model",
        "//perimortem",
    ],
)

cc_library(
    name = "ttx",
    hdrs = [
        "core/types.hpp",
    ],
    deps = [
        ":dialect",
        ":lexical",
        ":model",
        "//perimortem",
    ],
)
