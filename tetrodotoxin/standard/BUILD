"""
Tetrodotoxin standard TTX source packages.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "types",
    hdrs = [
        "types.hpp",
    ],
    deps = [
        "//perimortem:core",
        "//ttx",
    ],
)

exports_files(
    [
        "perimortem/graphics/library/sprite.ttx",
        "perimortem/graphics/library/types.ttx",
        "perimortem/graphics/package.ttx",
        "perimortem/graphics/renderers/renderer_2d.ttx",
        "perimortem/graphics/shaders/default_2d.ttx",
        "perimortem/math/library/types.ttx",
        "perimortem/math/package.ttx",
        "perimortem/runtime/library/types.ttx",
        "perimortem/runtime/package.ttx",
    ],
)
