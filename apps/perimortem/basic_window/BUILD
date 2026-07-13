load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "basic_window",
    srcs = ["main.cpp"],
    deps = [
        "//perimortem:platform",
        "//perimortem:vulkan",
    ],
)
