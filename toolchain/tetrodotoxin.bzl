"""
Starlark rules for the Tetrodotoxin (TTX) language.

The TTX compiler aims to provide a way to compile TTX directly to static
libraries that can be consumed by the regular `cc_toolchain`, alongside
Puffer Buffers that carry terminal source facts for Tetrodotoxin tooling.

Usage in a BUILD file:

    load("//toolchain:tetrodotoxin.bzl", "ttx_library", "ttx_package_folder")

    ttx_library(
        name = "my_lib",
        library_name = "Example.MyLib",
        srcs = ["library.ttx"],
    )

    ttx_package_folder(
        name = "my_package",
        root = "perimortem/graphics",
        package_name = "Perimortem.Graphics",
        major = 1,
        minor = 0,
        deps = [":my_dependency"],
    )

A generated header is automatically available to dependents. The authored
unit name owns the module directory, so its logical include is independent of
the Bazel package or repository that built it:

    #include "Example.MyLib/c_abi.h"

Standalone Library actions use `//puffer:puffer` and the in process LLVM
backend. Puffer emits one object and C header. Bazel's selected C++ toolchain
owns archive creation and publishes the resulting CcInfo.

Executable application targets belong here once App lowering produces real
declarations. Until then, these rules do not generate host bridge code that
guesses at TTX runtime types.
"""

load(
    "@rules_cc//cc:find_cc_toolchain.bzl",
    "CC_TOOLCHAIN_ATTRS",
    "find_cc_toolchain",
)
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

TtxPackageInfo = provider(
    doc = (
        "Transitive package Puffer Buffers visible to Tetrodotoxin."
    ),
    fields = {
        "puffer_buffers": (
            "depset of .puffer Puffer Buffer package interfaces emitted for " +
            "Tetrodotoxin resolution."
        ),
    },
)

def _collect_puffer_buffers(deps):
    return depset(transitive = [
        dep[TtxPackageInfo].puffer_buffers
        for dep in deps
    ])

def _ttx_compile_impl(ctx, package):
    unit_name = ctx.attr.package_name if package else ctx.attr.library_name
    if package:
        if ctx.attr.major < 0 or ctx.attr.major > 65535:
            fail("ttx_package major must fit in Unsigned_16")
        if ctx.attr.minor < 0 or ctx.attr.minor > 65535:
            fail("ttx_package minor must fit in Unsigned_16")
        if ctx.attr.major == 0 and ctx.attr.minor == 0:
            fail("ttx_package version 0.0 is reserved for an unset version")
        artifact_root = "%s/%d.%d/" % (
            unit_name,
            ctx.attr.major,
            ctx.attr.minor,
        )
    else:
        artifact_root = unit_name + "/"
    archive = ctx.actions.declare_file(artifact_root + "x86_64.a")
    header = ctx.actions.declare_file(artifact_root + "cpp_abi.hpp")
    puffer_buffer = None
    outputs = [archive, header]
    if package:
        puffer_buffer = ctx.actions.declare_file(
            artifact_root + "binary_archive.puffer",
        )
        outputs.append(puffer_buffer)

    include_root = ctx.bin_dir.path
    if ctx.label.package:
        include_root += "/" + ctx.label.package

    args = [
        "-package" if package else "-library",
        "-output=%s" % archive.path,
        "-header=%s" % header.path,
    ]
    if package:
        args.append("-puffer=%s" % puffer_buffer.path)
        args.append("-major=%d" % ctx.attr.major)
        args.append("-minor=%d" % ctx.attr.minor)

    args.append("-name=%s" % unit_name)

    dependency_puffer_buffers = _collect_puffer_buffers(ctx.attr.deps)
    for dep_buffer in dependency_puffer_buffers.to_list():
        args.append("-dep=%s" % dep_buffer.path)

    for src in ctx.files.srcs:
        args.append("-source=%s" % src.path)

    kind = "package" if package else "library"
    ctx.actions.run(
        inputs = depset(
            direct = ctx.files.srcs,
            transitive = [dependency_puffer_buffers],
        ),
        outputs = outputs,
        executable = ctx.executable._compiler,
        arguments = args,
        mnemonic = "TtxCompile",
        progress_message = "Compiling TTX %s %s" % (kind, ctx.label),
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

    # Each authored unit is a module directory. Exposing the Bazel package's
    # output root keeps the logical include stable when the target moves to a
    # different Bazel package or an external repository.
    compilation_context = cc_common.create_compilation_context(
        headers = depset([header]),
        system_includes = depset([include_root]),
    )

    package_buffers = []
    if package:
        package_buffers.append(puffer_buffer)

    output_files = [archive, header]
    if package:
        output_files.append(puffer_buffer)

    return [
        CcInfo(
            compilation_context = compilation_context,
            linking_context = linking_context,
        ),
        TtxPackageInfo(
            puffer_buffers = depset(
                direct = package_buffers,
                transitive = [dependency_puffer_buffers],
            ),
        ),
        DefaultInfo(files = depset(output_files)),
    ]

def _ttx_library_impl(ctx):
    if len(ctx.files.srcs) != 1:
        fail("ttx_library requires exactly one direct Library source")
    if ctx.attr.deps:
        fail("standalone LLVM ttx_library does not consume Package dependencies")

    unit_name = ctx.attr.library_name
    artifact_root = unit_name + "/"
    llvm_ir = ctx.actions.declare_file(artifact_root + "library.ll")
    object_file = ctx.actions.declare_file(artifact_root + "x86_64.o")
    header = ctx.actions.declare_file(artifact_root + "c_abi.h")
    source = ctx.files.srcs[0]
    arguments = ctx.actions.args()
    arguments.add("-library")
    arguments.add("-backend=llvm")
    arguments.add("-target=x86_64-sysv")
    arguments.add("-debug=%s" % ctx.attr.debug)
    arguments.add("-name=%s" % unit_name)
    arguments.add(source)
    arguments.add(llvm_ir, format = "-ir=%s")
    arguments.add(object_file, format = "-object=%s")
    arguments.add(header, format = "-header=%s")

    ctx.actions.run(
        inputs = [source],
        outputs = [llvm_ir, object_file, header],
        executable = ctx.executable._compiler,
        arguments = [arguments],
        mnemonic = "TtxLlvmCompile",
        progress_message = "Compiling TTX Library %s" % ctx.label,
    )

    cc_toolchain = find_cc_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    compilation_outputs = cc_common.create_compilation_outputs(
        objects = depset([object_file]),
        pic_objects = depset([object_file]),
    )
    linking_context, linking_outputs = (
        cc_common.create_linking_context_from_compilation_outputs(
            actions = ctx.actions,
            name = ctx.label.name,
            compilation_outputs = compilation_outputs,
            cc_toolchain = cc_toolchain,
            feature_configuration = feature_configuration,
            disallow_dynamic_library = True,
        )
    )

    include_root = ctx.bin_dir.path
    if ctx.label.package:
        include_root += "/" + ctx.label.package
    compilation_context = cc_common.create_compilation_context(
        headers = depset([header]),
        system_includes = depset([include_root]),
    )
    output_files = [llvm_ir, object_file, header]
    library = linking_outputs.library_to_link
    if library.static_library:
        output_files.append(library.static_library)
    if library.pic_static_library and library.pic_static_library != library.static_library:
        output_files.append(library.pic_static_library)

    generated_cc_info = CcInfo(
        compilation_context = compilation_context,
        linking_context = linking_context,
    )

    return [
        cc_common.merge_cc_infos(
            direct_cc_infos = [generated_cc_info],
            cc_infos = [ctx.attr._runtime[CcInfo]],
        ),
        TtxPackageInfo(puffer_buffers = depset()),
        DefaultInfo(files = depset(output_files)),
    ]

def _ttx_package_impl(ctx):
    return _ttx_compile_impl(ctx, True)

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
            doc = (
                "TTX package dependencies whose Puffer Buffers must be " +
                "visible during package loading."
            ),
        ),
        library_name = attr.string(
            mandatory = True,
            doc = (
                "Stable semantic name and generated artifact directory for " +
                "this standalone TTX Library."
            ),
        ),
        debug = attr.string(
            default = "none",
            values = ["none", "line", "full"],
            doc = "LLVM debug information mode.",
        ),
        _compiler = attr.label(
            default = "//puffer:puffer",
            executable = True,
            cfg = "exec",
            doc = "The Tetrodotoxin compiler binary.",
        ),
        _runtime = attr.label(
            default = "//perimortem:memory",
            providers = [CcInfo],
            doc = "Perimortem runtime linked by generated managed values.",
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
    doc = (
        "Compiles TTX source files into an x86-64 ELF static library usable " +
        "by cc_binary."
    ),
)

ttx_package = rule(
    implementation = _ttx_package_impl,
    attrs = dict(
        CC_TOOLCHAIN_ATTRS,
        srcs = attr.label_list(
            allow_files = [".ttx"],
            doc = (
                "Package manifest and owned TTX source files for this module."
            ),
        ),
        deps = attr.label_list(
            providers = [TtxPackageInfo],
            doc = (
                "Dependent TTX packages whose Puffer Buffers must be " +
                "visible while loading this module."
            ),
        ),
        package_name = attr.string(
            mandatory = True,
            doc = (
                "Authored TTX package name. Package terminals are emitted " +
                "beneath its explicit version directory."
            ),
        ),
        major = attr.int(
            mandatory = True,
            doc = "Authored package Major version. Zero is valid with a nonzero Minor.",
        ),
        minor = attr.int(
            mandatory = True,
            doc = "Authored package Minor version. Version 0.0 is reserved as unset.",
        ),
        _compiler = attr.label(
            default = "//tetrodotoxin:puffer",
            executable = True,
            cfg = "exec",
            doc = "The Tetrodotoxin compiler binary.",
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
    doc = (
        "Compiles an authored TTX module rooted by dialect : Package and " +
        "exports its manifest to dependent TTX targets."
    ),
)

def ttx_package_folder(
        name,
        root,
        package_name,
        major,
        minor,
        deps = None,
        **kwargs):
    """Compiles a TTX package folder rooted at a package.ttx file.

    Bazel still sees the concrete .ttx files through native.glob, but BUILD
    files can name the source package as the authored folder instead of
    repeating every implementation file.
    """
    ttx_package(
        name = name,
        srcs = native.glob([root + "/**/*.ttx"]),
        deps = deps or [],
        package_name = package_name,
        major = major,
        minor = minor,
        **kwargs
    )
