"""
Tetrodotoxin standard TTX source packages.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//toolchain:tetrodotoxin.bzl", "ttx_package_folder")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "types",
    hdrs = [
        "types.hpp",
    ],
    deps = [
        "//perimortem:core",
        "//tetrodotoxin/archiver:version",
        "//ttx",
    ],
)

ttx_package_folder(
    name = "perimortem_math",
    package_name = "Perimortem.Math",
    root = "perimortem/math",
)

ttx_package_folder(
    name = "perimortem_runtime",
    package_name = "Perimortem.Runtime",
    root = "perimortem/runtime",
)

ttx_package_folder(
    name = "perimortem_graphics",
    package_name = "Perimortem.Graphics",
    root = "perimortem/graphics",
    deps = [
        ":perimortem_math",
    ],
)
