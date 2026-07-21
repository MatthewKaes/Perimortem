"""
Durable current-model Tetrodotoxin package data and serialization.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "archiver",
    srcs = [
        "manifest.cpp",
        "reader.cpp",
        "writer.cpp",
    ],
    hdrs = [
        "dependency.hpp",
        "format.hpp",
        "manifest.hpp",
        "reader.hpp",
        "writer.hpp",
    ],
    include_prefix = "tetrodotoxin/archiver",
    strip_include_prefix = ".",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:system",
        "//perimortem:utility",
        "//tetrodotoxin:model",
        "//ttx:concept",
        "//ttx:model",
    ],
)
