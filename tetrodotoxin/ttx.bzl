"""
Starlark rules for the Tetrodotoxin (TTX) language.

The TTX compiler aims to provide a way to compile TTX directly to terminal
artifacts that can be consumed by the regular `cc_toolchain`.

Usage in a BUILD file:

    load("//tetrodotoxin:ttx.bzl", "ttx_library")

    ttx_library(
        name = "my_lib",
        srcs = ["png.ttx"],
    )

A generated header is automatically available to dependents:

    #include "ttx_generated/my_lib.hpp"

Executable application targets should come back after dialect parsing and
lowering can produce real declarations. This file intentionally does not
generate host bridge code that guesses at TTX runtime types.
"""

load("@rules_cc//cc:find_cc_toolchain.bzl", "CC_TOOLCHAIN_ATTRS", "find_cc_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

TtxPackageInfo = provider(
    doc = "Transitive TTX source files visible to Tetrodotoxin package loading.",
    fields = {
        "source_files": "depset of .ttx source files used for package and raw-file loading.",
    },
)

def _collect_ttx_source_files(deps):
    return depset(transitive = [
        dep[TtxPackageInfo].source_files
        for dep in deps
    ])

def _ttx_library_impl(ctx):
    archive = ctx.actions.declare_file(ctx.attr.name + ".a")
    header = ctx.actions.declare_file("ttx_generated/" + ctx.attr.name + ".hpp")
    include_root = ctx.bin_dir.path
    if ctx.label.package:
        include_root = include_root + "/" + ctx.label.package

    args = ["--output", archive.path, "--header", header.path]
    dependency_source_files = _collect_ttx_source_files(ctx.attr.deps)
    for dep_source in dependency_source_files.to_list():
        args.extend(["--dep", dep_source.path])

    for src in ctx.files.srcs:
        args.append(src.path)

    ctx.actions.run(
        inputs = depset(
            direct = ctx.files.srcs,
            transitive = [dependency_source_files],
        ),
        outputs = [archive, header],
        executable = ctx.executable._compiler,
        arguments = args,
        mnemonic = "TtxCompile",
        progress_message = "Compiling TTX library %s" % ctx.label,
    )

    # Wrap the generated archive in CcInfo so cc_binary can depend on this
    # target without any extra boilerplate.
    cc_toolchain = find_cc_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    lib = cc_common.create_library_to_link(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        static_library = archive,
    )

    linking_context = cc_common.create_linking_context(
        linker_inputs = depset([
            cc_common.create_linker_input(
                owner = ctx.label,
                libraries = depset([lib]),
            ),
        ]),
    )

    # Make the generated header available so consumers can write:
    #include "ttx_generated/<name>.hpp"
    compilation_context = cc_common.create_compilation_context(
        headers = depset([header]),
        system_includes = depset([include_root]),
    )

    return [
        CcInfo(
            compilation_context = compilation_context,
            linking_context = linking_context,
        ),
        TtxPackageInfo(
            source_files = depset(
                direct = ctx.files.srcs,
                transitive = [dependency_source_files],
            ),
        ),
        DefaultInfo(files = depset([archive, header])),
    ]

ttx_library = rule(
    implementation = _ttx_library_impl,
    attrs = dict(
        CC_TOOLCHAIN_ATTRS,
        srcs = attr.label_list(
            allow_files = [".ttx"],
            doc = "TTX source files compiled into the library.",
        ),
        deps = attr.label_list(
            providers = [TtxPackageInfo],
            doc = "TTX package dependencies whose source manifests must be visible during package loading.",
        ),
        _compiler = attr.label(
            default = "//tetrodotoxin:ttx",
            executable = True,
            cfg = "exec",
            doc = "The Tetrodotoxin compiler binary.",
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
    doc = "Compiles TTX source files into an x86-64 ELF static library usable by cc_binary.",
)

ttx_package = rule(
    implementation = _ttx_library_impl,
    attrs = dict(
        CC_TOOLCHAIN_ATTRS,
        srcs = attr.label_list(
            allow_files = [".ttx"],
            doc = "Package manifest and owned TTX source files for this module.",
        ),
        deps = attr.label_list(
            providers = [TtxPackageInfo],
            doc = "Dependent TTX packages whose source manifests must be visible while loading this module.",
        ),
        _compiler = attr.label(
            default = "//tetrodotoxin:ttx",
            executable = True,
            cfg = "exec",
            doc = "The Tetrodotoxin compiler binary.",
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
    doc = "Compiles an authored TTX module rooted by dialect : Package; and exports its manifest to dependent TTX targets.",
)
