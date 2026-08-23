"""Terminal producers for completed Tetrodotoxin graphs."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "llvm",
    srcs = glob([
        "llvm/**/*.cpp",
    ]),
    hdrs = glob([
        "llvm/**/*.hpp",
    ]),
    deps = [
        "//perimortem",
        "//tetrodotoxin:language",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//toolchain/llvm:sdk",
        "//ttx:concept",
        "//ttx:lexical",
        "//ttx:model",
    ],
)
