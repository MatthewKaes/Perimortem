"""
Toolchain configuration for Perimortem

Supports x86-64 Linux and Windows with the pinned LLVM distribution fetched by
//toolchain:llvm.bzl. Bazel materializes the tools in its external
repository area; no system LLVM installation is required. The Clang driver is
used for both compilation and linking.

For debugging use the CodeLLDB extension in VSCode plus the included .vscode launch and task jsons.
"""

load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/toolchains:cc_toolchain_config_info.bzl", "CcToolchainConfigInfo")

cpp_compile_actions = [
    ACTION_NAMES.cpp_compile,
]

c_compile_actions = [
    ACTION_NAMES.c_compile,
]

def _impl(ctx):
    windows = ctx.attr.target_os == "windows"
    llvm_bin = ctx.attr.tool_root
    tool_paths = [
        tool_path(
            # Compiler is referenced by the name "gcc" for historic reasons.
            name = "gcc",
            path = llvm_bin + ("clang++.exe" if windows else "clang++"),
        ),
        tool_path(
            # Compiler is referenced by the name "gcc" for historic reasons.
            name = "g++",
            path = llvm_bin + ("clang++.exe" if windows else "clang++"),
        ),
        tool_path(
            name = "ld",
            path = llvm_bin + ("clang++.exe" if windows else "clang++"),
        ),
        tool_path(
            name = "ar",
            path = llvm_bin + ("llvm-ar.exe" if windows else "llvm-ar"),
        ),
        tool_path(
            name = "cpp",
            path = llvm_bin + ("clang-cpp.exe" if windows else "clang-cpp"),
        ),
        tool_path(
            name = "gcov",
            path = llvm_bin + ("llvm-cov.exe" if windows else "llvm-cov"),
        ),
        tool_path(
            name = "nm",
            path = llvm_bin + ("llvm-nm.exe" if windows else "llvm-nm"),
        ),
        tool_path(
            name = "objdump",
            path = llvm_bin + ("llvm-objdump.exe" if windows else "llvm-objdump"),
        ),
        tool_path(
            name = "strip",
            path = llvm_bin + ("llvm-strip.exe" if windows else "llvm-strip"),
        ),
    ]

    features = [
        feature(
            name = "cpp_compiler_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = cpp_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                # Always Wall and Werror.
                                # The goal is to bootstrap an 100% standard conforming codebase.
                                "-Wall",
                                "-Werror",
                                "-fno-exceptions",
                                "-fno-rtti",
                                "-mavx2",  # AVX2 support required
                                "-mrdrnd",  # _rdrand64_step
                                "-march=x86-64-v3" if windows else "-march=znver4",
                                "-std=c++26",
                                "-no-canonical-prefixes",
                                "-DPERI_WINDOWS" if windows else "-DPERI_LINUX",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "c_compiler_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = c_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                # Always Wall and Werror.
                                # The goal is to bootstrap an 100% standard conforming codebase.
                                "-Wall",
                                "-Werror",
                                "-Wno-character-conversion",  # google-test
                                "-fno-exceptions",
                                "-fno-rtti",
                                "-march=x86-64-v3" if windows else "-march=znver4",
                                "-std=c23",
                                "-no-canonical-prefixes",
                                # Force C builds from external libraries to build in C.
                                "-xc",
                                "-DPERI_WINDOWS" if windows else "-DPERI_LINUX",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = ([
            "C:/Program Files/LLVM/lib/clang/22/include",
            "C:/Program Files/LLVM/lib/clang/21/include",
            "C:/Program Files/LLVM/lib/clang/20/include",
        ] if windows else [
            "/usr/lib/clang/22/include",
            "/usr/lib/clang/21/include",
            "/usr/lib/clang/20/include",
            "/usr/include",
        ]),
        toolchain_identifier = ("windows_x86_64" if windows else "linux_x86_64") + "-clang-toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "x64_windows" if windows else "k8",
        target_libc = "msvcrt" if windows else "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "target_os": attr.string(
            mandatory = True,
            values = ["linux", "windows"],
        ),
        "tool_root": attr.string(default = "bin/"),
    },
    provides = [CcToolchainConfigInfo],
)
