# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"Language neutral graphics hosting and submission contracts."

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hosting",
    srcs = ["hosting.cpp"],
    hdrs = ["hosting.hpp"],
    deps = [
        "//tetrodotoxin/library:language",
        "//ttx:concept",
    ],
)

cc_library(
    name = "runtime",
    srcs = [
        "descriptor.cpp",
        "submission.cpp",
    ],
    hdrs = [
        "descriptor.hpp",
        "submission.hpp",
    ],
    deps = [
        "//perimortem:core",
        "//perimortem:graphics",
        "//perimortem:memory",
    ],
)
