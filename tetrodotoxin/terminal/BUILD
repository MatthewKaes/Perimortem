# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Terminal producers for completed Tetrodotoxin graphs."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "graph_text",
    srcs = ["graph_text.cpp"],
    hdrs = ["graph_text.hpp"],
    copts = ["-fPIC"],
    deps = ["//ttx:query"],
)

cc_binary(
    name = "graph_text_plugin.so",
    srcs = ["graph_text_plugin.cpp"],
    copts = ["-fPIC"],
    linkshared = True,
    linkstatic = True,
    deps = [
        ":graph_text",
        ":provider",
        ":provider_abi",
        "//tetrodotoxin:plugin_abi",
        "//tetrodotoxin:environment_provider_abi",
    ],
)

cc_binary(
    name = "native_plugin.so",
    srcs = ["native/plugin.cpp"],
    linkshared = True,
    linkstatic = True,
    deps = [
        ":native",
        ":native_toolchain",
        ":provider",
        "//tetrodotoxin:plugin_abi",
    ],
)

cc_library(
    name = "native_toolchain",
    srcs = [
        "native/provider.cpp",
        "native/toolchain.cpp",
    ],
    hdrs = [
        "native/provider.h",
        "native/toolchain.hpp",
    ],
    deps = [
        "//perimortem:headless",
        "//ttx",
    ],
)

cc_library(
    name = "provider_abi",
    hdrs = ["provider.h"],
    deps = ["//ttx:abi"],
)

cc_library(
    name = "provider",
    srcs = [
        "artifact_request.cpp",
        "provider.cpp",
        "query.cpp",
    ],
    hdrs = [
        "artifact_request.hpp",
        "query.hpp",
    ],
    deps = [
        ":provider_abi",
        "//perimortem:headless",
        "//tetrodotoxin:environment_provider_abi",
        "//tetrodotoxin/language",
        "//ttx",
    ],
)

cc_library(
    name = "application",
    srcs = glob(["application/**/*.cpp"]),
    hdrs = glob(["application/**/*.hpp"]),
    deps = [
        ":abi",
        ":graphics",
        ":vulkan",
        "//perimortem:headless",
        "//tetrodotoxin/app",
        "//tetrodotoxin/scene",
        "//ttx",
    ],
)

cc_library(
    name = "abi",
    srcs = glob([
        "abi/**/*.c",
        "abi/**/*.cpp",
    ]),
    hdrs = glob([
        "abi/**/*.h",
        "abi/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:package",
        "//tetrodotoxin/language",
        "//tetrodotoxin/shader",
        "//ttx",
    ],
)

cc_library(
    name = "llvm",
    srcs = glob([
        "llvm/**/*.cpp",
    ]),
    hdrs = glob([
        "llvm/**/*.hpp",
    ]),
    deps = [
        ":abi",
        ":graphics",
        "//perimortem:headless",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin/language",
        "//tetrodotoxin/scene",
        "//toolchain/llvm:sdk",
        "//ttx",
    ],
)

cc_library(
    name = "graphics",
    srcs = glob([
        "graphics/**/*.cpp",
    ]),
    hdrs = glob([
        "graphics/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/library",
        "//tetrodotoxin/scene",
        "//ttx",
    ],
)

cc_library(
    name = "spirv",
    srcs = glob([
        "spirv/**/*.cpp",
    ]),
    hdrs = glob([
        "spirv/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:library",
        "//tetrodotoxin/language",
        "//tetrodotoxin/render",
        "//tetrodotoxin/shader",
        "//ttx",
    ],
)

cc_library(
    name = "vulkan",
    srcs = glob([
        "vulkan/**/*.cpp",
    ]),
    hdrs = glob([
        "vulkan/**/*.hpp",
    ]),
    deps = [
        ":spirv",
        "//perimortem:headless",
        "//tetrodotoxin/library",
        "//tetrodotoxin/render",
        "//tetrodotoxin/shader",
        "//ttx",
    ],
)

cc_library(
    name = "native",
    srcs = glob(
        ["native/**/*.cpp"],
        exclude = [
            "native/plugin.cpp",
            "native/provider.cpp",
            "native/toolchain.cpp",
        ],
    ),
    hdrs = glob(
        [
            "native/**/*.h",
            "native/**/*.hpp",
        ],
        exclude = [
            "native/provider.h",
            "native/toolchain.hpp",
        ],
    ),
    deps = [
        ":native_toolchain",
        ":abi",
        ":application",
        ":graphics",
        ":llvm",
        ":spirv",
        ":vulkan",
        "//perimortem:headless",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:package",
        "//tetrodotoxin/app",
        "//tetrodotoxin/language",
        "//tetrodotoxin/library",
        "//tetrodotoxin/scene",
        "//tetrodotoxin/shader",
        "//ttx",
    ],
)
