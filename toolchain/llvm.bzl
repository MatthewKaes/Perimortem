"""Provides a pinned hermetic LLVM distribution for the host platform."""

_LLVM_VERSION = "22.1.4"

def _llvm_repository_impl(ctx):
    windows = ctx.os.name.lower().startswith("windows")
    if windows:
        archive = "clang+llvm-%s-x86_64-pc-windows-msvc.tar.xz" % _LLVM_VERSION
        sha256 = "ed775bdaea7087c6c1aeac9498352cfcd8610d92dc4fe9eda9aecb15ce712a2c"
        strip_prefix = "clang+llvm-%s-x86_64-pc-windows-msvc" % _LLVM_VERSION
        target_os = "windows"
    else:
        archive = "LLVM-%s-Linux-X64.tar.xz" % _LLVM_VERSION
        sha256 = "cdf232e3bc5d9909ddcf8cb7016802c6745a01e69a596747c684caa894a11567"
        strip_prefix = "LLVM-%s-Linux-X64" % _LLVM_VERSION
        target_os = "linux"

    ctx.download_and_extract(
        url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-%s/%s" % (_LLVM_VERSION, archive),
        sha256 = sha256,
        stripPrefix = strip_prefix,
    )

    ctx.file("BUILD.bazel", """
load("@rules_cc//cc/toolchains:cc_toolchain.bzl", "cc_toolchain")
load("@perimortem//toolchain:cc_toolchain_config.bzl", "cc_toolchain_config")
load("@perimortem//toolchain:executable_file.bzl", "executable_file")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "llvm_files",
    srcs = glob(["**"], exclude = ["BUILD.bazel"]),
)

executable_file(
    name = "clang-format",
    src = "bin/clang-format%s",
)

cc_toolchain_config(
    name = "host_x86_64_toolchain_config",
    target_os = "%s",
    tool_root = "bin/",
)

cc_toolchain(
    name = "host_x86_64_toolchain",
    toolchain_identifier = "%s_x86_64-clang-toolchain",
    toolchain_config = ":host_x86_64_toolchain_config",
    all_files = ":llvm_files",
    compiler_files = ":llvm_files",
    dwp_files = ":llvm_files",
    linker_files = ":llvm_files",
    objcopy_files = ":llvm_files",
    strip_files = ":llvm_files",
    supports_param_files = %s,
)

toolchain(
    name = "cc_toolchain_for_host_x86_64",
    toolchain = ":host_x86_64_toolchain",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:%s",
    ],
    target_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:%s",
    ],
)
""" % (
        ".exe" if windows else "",
        target_os,
        target_os,
        "1" if windows else "0",
        target_os,
        target_os,
    ))

_llvm_repository = repository_rule(
    implementation = _llvm_repository_impl,
)

def _llvm_extension_impl(_ctx):
    _llvm_repository(name = "llvm")

llvm = module_extension(
    implementation = _llvm_extension_impl,
)
