"""
Perimortem::Core - Home to all common utilities and classes
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "perimortem",
    includes = ["."],
    deps = [
        ":core",
        ":graphics",
        ":memory",
        ":platform",
        ":serialization",
        ":system",
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
        ":core",
        ":utility",
    ],
)

cc_library(
    name = "system",
    srcs = glob([
        "system/*.cpp",
        "system/compression/*.cpp",
        "system/compression/bit_stream/*.cpp",
    ]),
    hdrs = glob([
        "system/*.hpp",
        "system/compression/*.hpp",
        "system/compression/bit_stream/*.hpp",
    ]),
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
        "graphics/vulkan/*.cpp",
    ]),
    hdrs = glob([
        "graphics/vulkan/*.hpp",
        "graphics/*.hpp",
    ]),
    includes = ["."],
    linkopts = ["-lvulkan"],
    deps = [
        ":core",
        ":memory",
    ],
)

cc_library(
    name = "xdg_shell_protocol",
    srcs = ["system/platform/wayland/xdg-shell-protocol.c"],
    hdrs = ["system/platform/wayland/xdg-shell-client-protocol.h"],
    # Generated C protocol code; suppress strict-warning enforcement.
    copts = ["-Wno-error"],
    includes = ["."],
    linkopts = ["-lwayland-client"],
)

cc_library(
    name = "platform",
    srcs = glob([
        "system/platform/wayland/*.cpp",
        "system/platform/wayland/*.c",
    ]),
    hdrs = glob([
        "system/platform/wayland/*.hpp",
        "system/platform/wayland/*.h",
    ]) + [
        "system/platform/window.hpp",
    ],
    includes = ["."],
    linkopts = ["-lwayland-client"],
    deps = [
        ":core",
        ":xdg_shell_protocol",
    ],
)

cc_library(
    name = "serialization",
    srcs = glob([
        # "serialization/archive/*.cpp",
        "serialization/*.cpp",
        "serialization/format/*.cpp",
        "serialization/json/*.cpp",
    ]),
    hdrs = glob([
        # "serialization/archive/*.hpp",
        "serialization/*.hpp",
        "serialization/format/*.hpp",
        "serialization/json/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":core",
        ":graphics",
        ":memory",
        ":system",
        ":utility",
    ],
)
