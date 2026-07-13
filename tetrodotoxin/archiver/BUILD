"""
Durable Tetrodotoxin package data and Puffer Buffer serialization.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "version",
    hdrs = [
        "version.hpp",
    ],
    include_prefix = "tetrodotoxin/archiver",
    strip_include_prefix = ".",
    deps = [
        "//perimortem:core",
    ],
)

cc_library(
    name = "archiver",
    srcs = [
        "reader.cpp",
        "writer.cpp",
    ],
    hdrs = [
        "dependency.hpp",
        "format.hpp",
        "manifest.hpp",
        "package.hpp",
        "reader.hpp",
        "reference.hpp",
        "terminal.hpp",
        "type/reference.hpp",
        "writer.hpp",
    ],
    include_prefix = "tetrodotoxin/archiver",
    strip_include_prefix = ".",
    deps = [
        ":version",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//tetrodotoxin:compiler",
        "//tetrodotoxin/standard:types",
        "//ttx",
    ],
)
