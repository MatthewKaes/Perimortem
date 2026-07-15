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
        "lexical/errors.cpp",
        "lexical/tokenizer.cpp",
    ],
    hdrs = [
        "lexical/class.hpp",
        "lexical/cursor.hpp",
        "lexical/errors.hpp",
        "lexical/source.hpp",
        "lexical/token.hpp",
        "lexical/tokenizer.hpp",
    ],
    include_prefix = "ttx/lexical",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
    ],
)

cc_library(
    name = "abstract",
    hdrs = [
        "abstract.hpp",
        "alias.hpp",
        "invalid.hpp",
    ],
    deps = [
        "//perimortem:core",
    ],
)

cc_library(
    name = "model",
    srcs = [
        "layout.cpp",
        "member.cpp",
        "type.cpp",
    ],
    hdrs = [
        "attribute.hpp",
        "documentation.hpp",
        "function.hpp",
        "layout.hpp",
        "member.hpp",
        "type.hpp",
    ],
    deps = [
        ":lexical",
        "//perimortem:core",
        "//perimortem:memory",
    ],
)

cc_library(
    name = "ttx",
    deps = [
        ":abstract",
        ":lexical",
        ":model",
        "//perimortem:core",
        "//perimortem:memory",
    ],
)
