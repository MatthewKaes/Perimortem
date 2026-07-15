"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "diagnostics",
    srcs = [
        "diagnostics/suggestions.cpp",
    ],
    hdrs = [
        "diagnostics/suggestions.hpp",
    ],
    includes = ["."],
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//ttx:model",
    ],
)

cc_library(
    name = "isa",
    srcs = glob(
        [
            "isa/**/*.cpp",
        ],
        exclude = [
            "isa/library/lowering.cpp",
            "isa/shader/lowering.cpp",
        ],
    ),
    hdrs = glob(["isa/**/*.hpp"]),
    includes = ["."],
    deps = [
        ":compiler",
        ":diagnostics",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:utility",
        "//tetrodotoxin/abi",
        "//tetrodotoxin/archiver",
        "//tetrodotoxin/standard:types",
        "//ttx",
    ],
)

alias(
    name = "puffer",
    actual = "//tetrodotoxin/puffer:puffer",
)

cc_library(
    name = "compiler",
    srcs = [
        "compiler/allocation/registers.cpp",
        "compiler/allocation/system_v.cpp",
        "compiler/assembler/spir_v.cpp",
        "compiler/assembler/x86_64.cpp",
        "compiler/engine.cpp",
        "compiler/execution/builder.cpp",
        "compiler/program.cpp",
        "compiler/target/cpp.cpp",
        "compiler/target/system_v.cpp",
    ],
    hdrs = [
        "compiler/allocation/registers.hpp",
        "compiler/allocation/system_v.hpp",
        "compiler/assembler/spir_v.hpp",
        "compiler/assembler/x86_64.hpp",
        "compiler/backend.hpp",
        "compiler/engine.hpp",
        "compiler/execution/addressable.hpp",
        "compiler/execution/binary.hpp",
        "compiler/execution/binding.hpp",
        "compiler/execution/block.hpp",
        "compiler/execution/body.hpp",
        "compiler/execution/builder.hpp",
        "compiler/execution/call.hpp",
        "compiler/execution/constant.hpp",
        "compiler/execution/function.hpp",
        "compiler/execution/operand.hpp",
        "compiler/execution/operation.hpp",
        "compiler/program.hpp",
        "compiler/execution/return.hpp",
        "compiler/symbol.hpp",
        "compiler/target/cpp.hpp",
        "compiler/target/system_v.hpp",
    ],
    includes = ["."],
    deps = [
        ":linker",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:utility",
        "//tetrodotoxin/abi",
        "//ttx",
    ],
)

cc_library(
    name = "isa_lowerers",
    srcs = [
        "isa/library/lowering.cpp",
        "isa/shader/lowering.cpp",
    ],
    includes = ["."],
    deps = [
        ":compiler",
        ":isa",
        "//perimortem:core",
        "//ttx",
    ],
)

cc_library(
    name = "linker",
    srcs = [
        "linker/linker.cpp",
        "linker/target/elf.cpp",
    ],
    hdrs = [
        "linker/linker.hpp",
        "linker/object/relocation.hpp",
        "linker/object/section.hpp",
        "linker/object/symbol.hpp",
        "linker/target/elf.hpp",
        "linker/target/format.hpp",
    ],
    includes = ["."],
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
    ],
)
