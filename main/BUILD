# The Root of everyhing.
# This package serves as the source of the fork of Perimortem from
# It's Godot reference.

# To access the main editor run:
#
# bazel run main:editor
#

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "editor",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//visibility:public"],
)
