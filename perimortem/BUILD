"""
Perimortem::Core - Home to all common utilities and classes
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "perimortem",
    includes = ["."],
    deps = [
        ":abi",
        ":compression",
        ":core",
        ":graphics",
        ":memory",
        ":platform",
        ":serialization",
        ":system",
        ":vulkan",
    ],
)

cc_library(
    name = "abi",
    srcs = glob([
        "abi/**/*.cpp",
        "abi/core/cleanup.hpp",
    ]),
    hdrs = glob(
        ["abi/**/*.hpp"],
        exclude = ["abi/core/cleanup.hpp"],
    ),
    includes = ["."],
    deps = [
        ":core",
    ],
)

cc_library(
    name = "core",
    srcs = [
        "core/algorithm/search.cpp",
        "core/bibliotheca.cpp",
        "core/diagnostics/log.cpp",
        "core/diagnostics/source.cpp",
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
        "core/thread/*.hpp",
        "core/*.hpp",
    ]),
    includes = ["."],
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
    name = "memory",
    srcs = glob([
        "memory/allocator/*.cpp",
        "memory/dynamic/*.cpp",
        "memory/managed/*.cpp",
    ]),
    hdrs = glob([
        "memory/allocator/*.hpp",
        "memory/dynamic/*.hpp",
        "memory/managed/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":abi",
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
        "system/*.cpp",
    ]),
    hdrs = glob(
        [
            "system/*.hpp",
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
    srcs = glob([
        "vulkan/*.cpp",
    ]),
    hdrs = glob([
        "vulkan/*.hpp",
    ]),
    includes = ["."],
    linkopts = ["-lvulkan"],
    deps = [
        ":core",
        ":graphics",
        ":memory",
    ],
)

cc_library(
    name = "wayland",
    srcs = glob([
        "system/platform/wayland/*.cpp",
    ]),
    hdrs = glob([
        "system/platform/wayland/*.hpp",
    ]),
    includes = ["."],
    linkopts = ["-lwayland-client"],
    deps = [":core"],
)

cc_library(
    name = "platform",
    srcs = [],
    hdrs = [
        "system/window.hpp",
    ],
    includes = ["."],
    deps = [
        ":core",
        ":wayland",
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
