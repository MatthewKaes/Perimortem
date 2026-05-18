"""
Starlark rules for the Tetrodotoxin (TTX) language.

The TTX compiler aims to provide a way to compile TTX directly to binary files
which can be pulled in to the regular `cc_toolchain`.

Currently `ttx_binary` is not supported and a `ttx_library` rule needs to be pulled
into a `cc_binary` in order to build.

Usage in a BUILD file:

    load("//tetrodotoxin:ttx.bzl", "ttx_library")

    ttx_library(
        name = "my_lib",
        srcs = ["source.ttx"],
    )

    cc_binary(
        name = "my_app",
        srcs = ["main.cpp"],
        deps = [":my_lib"],
    )

A generated header is automatically available to dependents:

    #include "my_lib.h"   // declares tetrodotoxin_test()

TODO: All source files are currently forwarded to the compiler but all the data
is droped and only the same test library is produced with hard coded machine code.
"""

load("@rules_cc//cc:find_cc_toolchain.bzl", "CC_TOOLCHAIN_ATTRS", "find_cc_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _ttx_library_impl(ctx):
    archive = ctx.actions.declare_file(ctx.attr.name + ".a")
    header = ctx.actions.declare_file("ttx/" + ctx.attr.name + ".hpp")

    args = ["--output", archive.path, "--header", header.path]
    for src in ctx.files.srcs:
        args.append(src.path)

    ctx.actions.run(
        inputs = ctx.files.srcs,
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

    # Make the generated header available as a system include so consumers
    # can write: #include "<name>.h"
    compilation_context = cc_common.create_compilation_context(
        headers = depset([header]),
        system_includes = depset([header.dirname]),
    )

    return [
        CcInfo(
            compilation_context = compilation_context,
            linking_context = linking_context,
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
