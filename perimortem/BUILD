# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
Perimortem::Core - Home to all common utilities and classes
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "perimortem",
    includes = ["."],
    deps = [
        ":compression",
        ":core",
        ":graphics",
        ":math",
        ":memory",
        ":platform",
        ":serialization",
        ":system",
        ":vulkan",
    ],
)

cc_library(
    name = "headless",
    deps = [
        ":core",
        ":graphics",
        ":memory",
        ":serialization",
        ":system",
    ],
)

cc_library(
    name = "core",
    srcs = [
        "core/algorithm/search.cpp",
        "core/bibliotheca.c",
        "core/diagnostics/log.cpp",
        "core/diagnostics/fatal.c",
        "core/diagnostics/source.cpp",
        "core/hash.c",
        "core/implementation.c",
        "core/object.c",
        "core/reader/binary.cpp",
        "core/reader/serial.cpp",
        "core/reader/textual.cpp",
        "core/thread/worker.cpp",
        "core/time.cpp",
        "core/writer/binary.cpp",
        "core/writer/serial.cpp",
        "core/writer/textual.cpp",
    ],
    hdrs = glob([
        "core/access/*.hpp",
        "core/static/*.hpp",
        "core/view/*.hpp",
        "core/algorithm/*.hpp",
        "core/reader/*.hpp",
        "core/writer/*.hpp",
        "core/diagnostics/*.hpp",
        "core/diagnostics/*.h",
        "core/thread/*.hpp",
        "core/*.hpp",
        "core/*.h",
    ]),
    includes = ["."],
    linkopts = ["-lpthread"],
)

cc_library(
    name = "utility",
    srcs = [],
    hdrs = glob([
        "utility/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
    ],
)

cc_library(
    name = "math",
    hdrs = glob(["math/**/*.hpp"]),
    includes = ["."],
    deps = [":core"],
)

cc_library(
    name = "memory",
    srcs = glob(
        [
            "memory/*.c",
            "memory/allocator/*.cpp",
            "memory/dynamic/*.cpp",
            "memory/managed/*.cpp",
        ],
        allow_empty = True,
    ),
    hdrs = glob([
        "memory/*.h",
        "memory/allocator/*.hpp",
        "memory/dynamic/*.hpp",
        "memory/managed/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":utility",
    ],
)

cc_library(
    name = "compression",
    srcs = glob([
        "compression/*.cpp",
        "compression/bit_stream/*.cpp",
    ]),
    hdrs = glob([
        "compression/*.hpp",
        "compression/bit_stream/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":memory",
    ],
)

cc_library(
    name = "system",
    srcs = glob([
        "system/*.c",
        "system/*.cpp",
    ]),
    hdrs = glob(
        [
            "system/*.hpp",
            "system/*.h",
        ],
        exclude = [
            "system/window.hpp",
        ],
    ),
    includes = ["."],
    linkopts = ["-lpthread"],
    deps = [
        ":core",
        ":memory",
        ":utility",
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
    includes = ["."],
    deps = [
        ":compression",
        ":core",
        ":memory",
    ],
)

cc_library(
    name = "vulkan",
    srcs = glob(["vulkan/**/*.cpp"]),
    hdrs = glob(["vulkan/**/*.hpp"]),
    includes = ["."],
    linkopts = ["-lvulkan"],
    deps = [
        ":core",
        ":graphics",
        ":memory",
        ":system",
    ],
)

cc_library(
    name = "platform",
    srcs = glob([
        "system/platform/wayland/*.cpp",
    ]),
    hdrs = ["system/window.hpp"] + glob([
        "system/platform/wayland/*.hpp",
    ]),
    includes = ["."],
    linkopts = ["-lwayland-client"],
    deps = [
        ":core",
        ":system",
    ],
)

cc_library(
    name = "serialization",
    srcs = [
        "serialization/base64.cpp",
        "serialization/json/blueprint.cpp",
        "serialization/json/node.cpp",
        "serialization/stream/binary.cpp",
        "serialization/stream/textual.cpp",
    ],
    hdrs = glob([
        "serialization/*.hpp",
        "serialization/json/*.hpp",
        "serialization/stream/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":memory",
    ],
)
