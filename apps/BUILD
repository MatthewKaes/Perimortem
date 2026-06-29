"""
Demo binaries for Perimortem.

Run the icon fade demo:
  bazel run //apps:splash_screen
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("//tetrodotoxin:ttx.bzl", "ttx_library")

package(default_visibility = ["//visibility:public"])

ttx_library(
    name = "icon_shader",
    srcs = ["shaders/icon.ttx"],
)

cc_binary(
    name = "splash_screen",
    srcs = ["splash_screen.cpp"],
    deps = [
        ":icon_shader",
        "//perimortem",
    ],
)
