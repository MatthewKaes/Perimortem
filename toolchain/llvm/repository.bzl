# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"Repository rule for the immutable LLVM Terminal development SDK."

_LLVM_VERSION = "22.1.8"
_ARCH_PACKAGE_VERSION = "22.1.8-2"

_LLVM_URL = (
    "https://archive.archlinux.org/packages/l/llvm/" +
    "llvm-22.1.8-2-x86_64.pkg.tar.zst"
)
_LLVM_SHA256 = "df30290f4af86681bd06a5a48ffda1e78ce6d9abd911fb62faa053a750759ea5"

_LLVM_LIBS_URL = (
    "https://archive.archlinux.org/packages/l/llvm-libs/" +
    "llvm-libs-22.1.8-2-x86_64.pkg.tar.zst"
)
_LLVM_LIBS_SHA256 = "ad4454d1b969192e7a878066400a583b984210322d67c6fa8958ad1175342658"

def _require_file(repository_ctx, relative_path):
    path = repository_ctx.path(relative_path)
    if not path.exists:
        fail("pinned LLVM SDK is missing expected file: {}".format(relative_path))
    return path

def _verify_package(repository_ctx, root, package_name):
    metadata_path = _require_file(repository_ctx, root + "/.PKGINFO")
    metadata = repository_ctx.read(metadata_path)
    expected_name = "pkgname = {}".format(package_name)
    expected_version = "pkgver = {}".format(_ARCH_PACKAGE_VERSION)
    if expected_name not in metadata or expected_version not in metadata:
        fail(
            "unexpected {} package metadata; expected {} {}".format(
                package_name,
                expected_name,
                expected_version,
            ),
        )

def _llvm_sdk_repository_impl(repository_ctx):
    # The packages stay in separate roots because their Arch metadata files have
    # the same names.  This also makes the header/runtime boundary reviewable in
    # the generated external repository rather than depending on extraction
    # order or overwritten files.
    repository_ctx.download_and_extract(
        url = _LLVM_URL,
        output = "headers",
        sha256 = _LLVM_SHA256,
    )
    repository_ctx.download_and_extract(
        url = _LLVM_LIBS_URL,
        output = "runtime",
        sha256 = _LLVM_LIBS_SHA256,
    )

    _verify_package(repository_ctx, "headers", "llvm")
    _verify_package(repository_ctx, "runtime", "llvm-libs")

    config_path = _require_file(
        repository_ctx,
        "headers/usr/include/llvm/Config/llvm-config.h",
    )
    config = repository_ctx.read(config_path)
    expected_define = '#define LLVM_VERSION_STRING "{}"'.format(_LLVM_VERSION)
    if expected_define not in config:
        fail("LLVM headers do not report the pinned version {}".format(_LLVM_VERSION))

    _require_file(repository_ctx, "runtime/usr/lib/libLLVM.so.22.1")
    repository_ctx.symlink(repository_ctx.attr.build_file, "BUILD.bazel")

llvm_sdk_repository = repository_rule(
    implementation = _llvm_sdk_repository_impl,
    attrs = {
        "build_file": attr.label(
            default = "//toolchain/llvm:archive.BUILD.bazel",
            allow_single_file = True,
        ),
    },
)
