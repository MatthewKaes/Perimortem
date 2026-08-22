"""
Starlark rules for the Tetrodotoxin (TTX) language.

The TTX compiler emits semantic Package Archives and native objects consumed
by the regular `cc_toolchain`.

Usage in a BUILD file:

    load(
        "//toolchain:tetrodotoxin.bzl",
        "ttx_library",
        "ttx_native_provider",
        "ttx_package",
    )

    ttx_library(
        name = "my_lib",
        library_name = "Example.MyLib",
        srcs = ["library.ttx"],
    )

    ttx_package(
        name = "my_package",
        manifest = "package.ttx",
        package_name = "Example.MyPackage",
        version = [1, 0],
        deps = [":my_dependency"],
        native_deps = [":host_services"],
    )

    ttx_native_provider(
        name = "host_services",
        provider_name = "Example.Host",
        dep = ":host_services_cc",
        functions = ["example_host_read"],
    )

A generated header is automatically available to dependents. The authored
unit name owns the module directory, so its logical include is independent of
the Bazel package or repository that built it:

    #include "Example.MyLib/c_abi.h"

Standalone Library actions use `//puffer:puffer` and the in process LLVM
backend. Puffer emits one object and C header. Bazel's selected C++ toolchain
owns archive creation and publishes the resulting CcInfo.

Package dependency semantics travel through Interface Archives. Complete
Archives preserve the root Package, while LLVM IR and native objects remain
separate target products. The Package manifest owns its semantic Source table;
the public macro discovers candidate `.ttx` files beneath that manifest and
never repeats member names in BUILD syntax.
"""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load(
    "@rules_cc//cc:find_cc_toolchain.bzl",
    "CC_TOOLCHAIN_ATTRS",
    "find_cc_toolchain",
)
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

TtxPackageInfo = provider(
    doc = "Semantic and target identity published by one TTX Package.",
    fields = {
        "identity": "Exact authored Package identity.",
        "version": "Authored Package [major, minor] version.",
        "complete_archive": "Complete semantic Package Archive.",
        "interface_archive": "Interface semantic Package Archive.",
        "transitive_interfaces": "Dependency-first Interface Archive depset.",
        "abi_manifest": "Native ABI Manifest for the selected artifact.",
        "transitive_abi_manifests": "Dependency-first native ABI Manifest depset.",
        "artifact_id": "Exact native artifact identifier.",
    },
)

TtxNativeProviderInfo = provider(
    doc = "One explicit logical provider for target selected Foreign imports.",
    fields = {
        "identity": "Stable logical provider identity.",
        "artifact_id": "Exact native artifact identifier.",
        "bindings": "Provider, target, kind, and symbol records for Puffer.",
    },
)

def _ttx_native_provider_impl(ctx):
    artifact_id = "x86_64-sysv-linux"
    if not ctx.attr.provider_name:
        fail("ttx_native_provider requires one provider_name")

    bindings = []
    declared = {}
    categories = [
        ("function", ctx.attr.functions),
        ("readonly", ctx.attr.readonly_states),
        ("writable", ctx.attr.writable_states),
    ]
    for kind, symbols in categories:
        for symbol in symbols:
            if not symbol or "|" in symbol:
                fail("ttx_native_provider symbols must be nonempty and cannot contain |")
            if symbol in declared:
                fail("ttx_native_provider declares symbol %s more than once" % symbol)
            declared[symbol] = True
            bindings.append("%s|%s|%s|%s" % (
                ctx.attr.provider_name,
                artifact_id,
                kind,
                symbol,
            ))

    return [
        TtxNativeProviderInfo(
            identity = ctx.attr.provider_name,
            artifact_id = artifact_id,
            bindings = bindings,
        ),
        ctx.attr.dep[CcInfo],
    ]

_ttx_native_provider = rule(
    implementation = _ttx_native_provider_impl,
    attrs = {
        "provider_name": attr.string(mandatory = True),
        "dep": attr.label(mandatory = True, providers = [CcInfo]),
        "functions": attr.string_list(),
        "readonly_states": attr.string_list(),
        "writable_states": attr.string_list(),
    },
    doc = "Selects one native CcInfo provider for a fixed target import set.",
)

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
        DefaultInfo(files = depset(output_files)),
    ]

def _ttx_package_impl(ctx):
    if len(ctx.attr.version) != 2:
        fail("ttx_package version must contain [major, minor]")
    major = ctx.attr.version[0]
    minor = ctx.attr.version[1]
    if major < 0 or major > 65535:
        fail("ttx_package major must fit in U16")
    if minor < 0 or minor > 65535:
        fail("ttx_package minor must fit in U16")
    if major == 0 and minor == 0:
        fail("ttx_package version 0.0 is reserved for an unset version")
    if not ctx.files.sources:
        fail("ttx_package requires at least one candidate source")

    artifact_id = "x86_64-sysv-linux"
    artifact_root = "%s/%d.%d/" % (
        ctx.attr.package_name,
        major,
        minor,
    )
    complete_archive = ctx.actions.declare_file(artifact_root + "complete.txa")
    interface_archive = ctx.actions.declare_file(artifact_root + "interface.txa")
    abi_manifest = ctx.actions.declare_file(artifact_root + "abi.manifest")
    header = ctx.actions.declare_file(artifact_root + "c_abi.h")
    arguments = ctx.actions.args()
    arguments.add("-package")
    arguments.add(ctx.file.manifest, format = "-manifest=%s")
    arguments.add("-name=%s" % ctx.attr.package_name)
    arguments.add("-version=%d.%d" % (major, minor))
    arguments.add("-artifact=%s" % artifact_id)
    arguments.add("-debug=%s" % ctx.attr.debug)
    arguments.add(complete_archive, format = "-complete=%s")
    arguments.add(interface_archive, format = "-interface=%s")
    arguments.add(header, format = "-header=%s")
    arguments.add(abi_manifest, format = "-abi-manifest=%s")

    dependency_interfaces = depset(
        direct = [
            dep[TtxPackageInfo].interface_archive
            for dep in ctx.attr.deps
        ],
        transitive = [
            dep[TtxPackageInfo].transitive_interfaces
            for dep in ctx.attr.deps
        ],
        order = "postorder",
    )
    for interface in dependency_interfaces.to_list():
        arguments.add(interface, format = "-dep=%s")

    dependency_abi_manifests = depset(
        direct = [
            dep[TtxPackageInfo].abi_manifest
            for dep in ctx.attr.deps
        ],
        transitive = [
            dep[TtxPackageInfo].transitive_abi_manifests
            for dep in ctx.attr.deps
        ],
        order = "postorder",
    )
    for manifest in dependency_abi_manifests.to_list():
        arguments.add(manifest, format = "-dep-abi=%s")

    native_cc_infos = []
    for provider in ctx.attr.native_deps:
        native = provider[TtxNativeProviderInfo]
        if native.artifact_id != artifact_id:
            fail("ttx_package native provider target does not match its artifact")
        for binding in native.bindings:
            arguments.add("-native-provider=%s" % binding)
        native_cc_infos.append(provider[CcInfo])

    llvm_ir = []
    object_files = []
    outputs = [complete_archive, interface_archive, abi_manifest, header]
    for index, source in enumerate(ctx.files.sources):
        source_name = source.basename[:-4]
        unit_name = "unit_%d_%s" % (index, source_name)
        ir = ctx.actions.declare_file(artifact_root + unit_name + ".ll")
        object_file = ctx.actions.declare_file(
            artifact_root + unit_name + ".o",
        )
        arguments.add_joined(
            [source.path, ir.path, object_file.path],
            join_with = "|",
            format_joined = "-unit=%s",
        )
        llvm_ir.append(ir)
        object_files.append(object_file)
        outputs.extend([ir, object_file])

    ctx.actions.run(
        inputs = depset(
            direct = [ctx.file.manifest] + ctx.files.sources,
            transitive = [dependency_interfaces, dependency_abi_manifests],
        ),
        outputs = outputs,
        executable = ctx.executable._compiler,
        arguments = [arguments],
        mnemonic = "TtxPackageCompile",
        progress_message = "Compiling TTX Package %s" % ctx.label,
    )

    cc_toolchain = find_cc_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    compilation_outputs = cc_common.create_compilation_outputs(
        objects = depset(object_files),
        pic_objects = depset(object_files),
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
    generated_cc_info = CcInfo(
        compilation_context = compilation_context,
        linking_context = linking_context,
    )
    dependency_cc_infos = [dep[CcInfo] for dep in ctx.attr.deps]
    package_interfaces = depset(
        direct = [interface_archive],
        transitive = [dependency_interfaces],
        order = "postorder",
    )
    package_abi_manifests = depset(
        direct = [abi_manifest],
        transitive = [dependency_abi_manifests],
        order = "postorder",
    )
    library = linking_outputs.library_to_link
    if library.static_library:
        outputs.append(library.static_library)
    if library.pic_static_library and library.pic_static_library != library.static_library:
        outputs.append(library.pic_static_library)

    return [
        cc_common.merge_cc_infos(
            direct_cc_infos = [generated_cc_info],
            cc_infos = dependency_cc_infos + native_cc_infos + [ctx.attr._runtime[CcInfo]],
        ),
        TtxPackageInfo(
            identity = ctx.attr.package_name,
            version = ctx.attr.version,
            complete_archive = complete_archive,
            interface_archive = interface_archive,
            transitive_interfaces = package_interfaces,
            abi_manifest = abi_manifest,
            transitive_abi_manifests = package_abi_manifests,
            artifact_id = artifact_id,
        ),
        DefaultInfo(files = depset(outputs)),
    ]

_ttx_library = rule(
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
                "TTX package dependencies whose Interface Archives must be " +
                "visible during loading."
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
            default = "//perimortem:abi_core",
            providers = [CcInfo],
            doc = "Perimortem ABI linked by generated native values.",
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
    doc = (
        "Compiles TTX source files into an x86-64 ELF static library usable " +
        "by cc_binary."
    ),
)

_ttx_package = rule(
    implementation = _ttx_package_impl,
    attrs = dict(
        CC_TOOLCHAIN_ATTRS,
        manifest = attr.label(
            mandatory = True,
            allow_single_file = [".ttx"],
            doc = "The one explicit package.ttx manifest.",
        ),
        sources = attr.label_list(
            mandatory = True,
            allow_files = [".ttx"],
            doc = "Candidate source files rooted beside the Package manifest.",
        ),
        deps = attr.label_list(
            providers = [TtxPackageInfo],
            doc = (
                "Dependent TTX Packages whose Interface Archives and native " +
                "libraries are consumed by this Package."
            ),
        ),
        native_deps = attr.label_list(
            providers = [TtxNativeProviderInfo, CcInfo],
            doc = "Target selected native providers for authored Foreign imports.",
        ),
        package_name = attr.string(
            mandatory = True,
            doc = (
                "Authored TTX package name. Package terminals are emitted " +
                "beneath its explicit version directory."
            ),
        ),
        version = attr.int_list(
            mandatory = True,
            doc = "Authored Package version as [major, minor].",
        ),
        debug = attr.string(
            default = "none",
            values = ["none", "line", "full"],
            doc = "LLVM debug information mode for every member object.",
        ),
        _compiler = attr.label(
            default = "//puffer:puffer",
            executable = True,
            cfg = "exec",
            doc = "The Tetrodotoxin compiler binary.",
        ),
        _runtime = attr.label(
            default = "//perimortem:abi_core",
            providers = [CcInfo],
            doc = "Perimortem ABI linked by generated native values.",
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
    doc = (
        "Compiles one authored TTX Package into Complete and Interface " +
        "Archives plus separate native member objects."
    ),
)

def ttx_native_provider(name, **kwargs):
    """Publishes one target selected native provider and its symbol inventory."""
    _ttx_native_provider(
        name = name,
        **kwargs
    )

def ttx_library(name, **kwargs):
    """Builds one standalone Library with mode-matched debug information."""
    if "debug" not in kwargs:
        kwargs["debug"] = select({
            "//toolchain:debug_mode": "full",
            "//conditions:default": "none",
        })
    _ttx_library(
        name = name,
        **kwargs
    )

def ttx_package(name, manifest, version, **kwargs):
    """Builds one manifest-owned Package without duplicating its Source table."""
    if type(manifest) != "string":
        fail("ttx_package manifest must be one package-relative path")

    segments = manifest.split("/")
    root = "/".join(segments[:-1])
    prefix = root + "/" if root else ""
    sources = native.glob(
        [prefix + "**/*.ttx"],
        exclude = [manifest],
    )
    if "debug" not in kwargs:
        kwargs["debug"] = select({
            "//toolchain:debug_mode": "full",
            "//conditions:default": "none",
        })
    _ttx_package(
        name = name,
        manifest = manifest,
        sources = sources,
        version = version,
        **kwargs
    )

def _ttx_application_entry_impl(ctx):
    package = ctx.attr.package[TtxPackageInfo]
    if package.artifact_id != "x86_64-sysv-linux":
        fail("ttx_application requires the x86_64-sysv-linux artifact")

    artifact_root = ctx.label.name + "/"
    llvm_ir = ctx.actions.declare_file(artifact_root + "entry.ll")
    object_file = ctx.actions.declare_file(artifact_root + "entry.o")
    arguments = ctx.actions.args()
    arguments.add("-application")
    arguments.add(package.complete_archive, format = "-complete=%s")
    arguments.add(package.abi_manifest, format = "-abi-manifest=%s")
    arguments.add("-app-member=%s" % ctx.attr.app_member)
    arguments.add("-artifact=%s" % package.artifact_id)
    arguments.add(llvm_ir, format = "-ir=%s")
    arguments.add(object_file, format = "-object=%s")
    dependency_interfaces = [
        interface
        for interface in package.transitive_interfaces.to_list()
        if interface.path != package.interface_archive.path
    ]
    for interface in dependency_interfaces:
        arguments.add(interface, format = "-dep=%s")

    dependency_abi_manifests = [
        manifest
        for manifest in package.transitive_abi_manifests.to_list()
        if manifest.path != package.abi_manifest.path
    ]
    for manifest in dependency_abi_manifests:
        arguments.add(manifest, format = "-dep-abi=%s")

    ctx.actions.run(
        inputs = depset(
            [package.complete_archive, package.abi_manifest] +
            dependency_interfaces + dependency_abi_manifests,
        ),
        outputs = [llvm_ir, object_file],
        executable = ctx.executable._compiler,
        arguments = [arguments],
        mnemonic = "TtxApplicationEntry",
        progress_message = "Compiling TTX Application entry %s" % ctx.label,
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
    linking_context, _ = cc_common.create_linking_context_from_compilation_outputs(
        actions = ctx.actions,
        name = ctx.label.name,
        compilation_outputs = compilation_outputs,
        cc_toolchain = cc_toolchain,
        feature_configuration = feature_configuration,
        disallow_dynamic_library = True,
    )
    entry_cc_info = CcInfo(linking_context = linking_context)

    return [
        cc_common.merge_cc_infos(
            direct_cc_infos = [entry_cc_info],
            cc_infos = [ctx.attr.package[CcInfo]],
        ),
        DefaultInfo(files = depset([llvm_ir, object_file])),
    ]

_ttx_application_entry = rule(
    implementation = _ttx_application_entry_impl,
    attrs = dict(
        CC_TOOLCHAIN_ATTRS,
        package = attr.label(
            mandatory = True,
            providers = [TtxPackageInfo, CcInfo],
            doc = "The root Package supplying the Complete Archive and native objects.",
        ),
        app_member = attr.string(
            mandatory = True,
            doc = "The exact member containing the App policy.",
        ),
        _compiler = attr.label(
            default = "//puffer:puffer",
            executable = True,
            cfg = "exec",
            doc = "The Tetrodotoxin compiler binary.",
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
)

def ttx_application(name, package, app_member, **kwargs):
    """Links one source-free App policy through Bazel's C++ toolchain."""
    entry_name = name + "_entry"
    _ttx_application_entry(
        name = entry_name,
        package = package,
        app_member = app_member,
    )
    cc_binary(
        name = name,
        deps = [":" + entry_name],
        **kwargs
    )
