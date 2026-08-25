# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
Starlark rules for the Tetrodotoxin (TTX) language.

The TTX compiler emits semantic Package Archives and native Package products
consumed by the regular `cc_toolchain`.

Usage in a BUILD file:

    load(
        "//toolchain:tetrodotoxin.bzl",
        "ttx_cpp_api",
        "ttx_native_provider",
        "ttx_package",
    )

    ttx_package(
        name = "my_package",
        cpp_header = "example/my_package.hpp",
        manifest = "package.ttx",
        package_name = "Example.MyPackage",
        version = [1, 0],
        deps = [":my_dependency"],
        native_deps = [":host_services"],
    )

    ttx_cpp_api(
        name = "my_package_cpp",
        package = ":my_package",
    )

    ttx_native_provider(
        name = "host_services",
        provider_name = "Example.Host",
        dep = ":host_services_cc",
        functions = ["example_host_read"],
    )

A generated C header is automatically available to semantic Package
dependents. A Package can also emit a C++ surface that follows its authored
routes, while `ttx_cpp_api` publishes that optional facade to native consumers.
The Package identity and version own the module directory, so its logical
includes are independent of the Bazel package or repository that built it:

    #include "Example.MyPackage/1.0/c_abi.h"
    #include "Example.MyPackage/1.0/api.hpp"

Package dependency semantics travel through Contract Archives. Complete
Archives preserve the root Package, while native objects remain separate target
products. Shader Programs become SPIR V modules inside the Package native
product without storing target words in the semantic payload.
The Package manifest owns its semantic Source table. The public macro discovers
candidate `.ttx` files beneath that manifest and never repeats member names in
BUILD syntax.
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
        "contract_archive": "Contract semantic Package Archive.",
        "transitive_contracts": "Dependency first Contract Archive depset.",
        "abi_manifest": "Native ABI Manifest for the selected artifact.",
        "transitive_abi_manifests": "Dependency-first native ABI Manifest depset.",
        "artifact_id": "Exact native artifact identifier.",
        "graphics_host": "Configured graphics Host requirement route.",
        "graphics_bindings": "Hosted Type routes paired with native Descriptor providers.",
        "graphics_shader": "Configured Shader Program route.",
        "cpp_compilation_context": "Generated C++ facade headers when requested.",
        "cpp_objects": "Generated C++ facade implementation objects.",
        "cpp_pic_objects": "Generated position independent C++ facade objects.",
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
    for graphics_binding in ctx.attr.graphics_bindings:
        parts = graphics_binding.split("|")
        if len(parts) != 2 or not parts[0] or not parts[1]:
            fail("graphics_bindings entries must be <Type route>|<provider symbol>")

    artifact_id = "x86_64-sysv-linux"
    artifact_root = "%s/%d.%d/" % (
        ctx.attr.package_name,
        major,
        minor,
    )
    complete_archive = ctx.actions.declare_file(artifact_root + "complete.txa")
    contract_archive = ctx.actions.declare_file(artifact_root + "contract.txa")
    abi_manifest = ctx.actions.declare_file(artifact_root + "abi.manifest")
    header = ctx.actions.declare_file(artifact_root + "c_abi.h")
    cpp_header = None
    cpp_source = None
    if ctx.attr.cpp_header:
        if len(ctx.files.sources) != 1:
            fail("The generated C++ Package API currently requires one source")
        if (
            ctx.attr.cpp_header.startswith("/") or
            ".." in ctx.attr.cpp_header.split("/") or
            not ctx.attr.cpp_header.endswith(".hpp")
        ):
            fail("cpp_header must be one repository relative .hpp include path")
        cpp_header = ctx.actions.declare_file(ctx.attr.cpp_header)
        cpp_source = ctx.actions.declare_file(ctx.attr.cpp_header[:-4] + ".cpp")
    arguments = ctx.actions.args()
    arguments.add("-package")
    arguments.add(ctx.file.manifest, format = "-manifest=%s")
    arguments.add("-name=%s" % ctx.attr.package_name)
    arguments.add("-version=%d.%d" % (major, minor))
    arguments.add("-artifact=%s" % artifact_id)
    arguments.add("-spirv-target=%s" % ctx.attr.spirv_target)
    arguments.add("-debug=%s" % ctx.attr.debug)
    arguments.add(complete_archive, format = "-complete=%s")
    arguments.add(contract_archive, format = "-contract=%s")
    arguments.add(header, format = "-header=%s")
    if ctx.attr.cpp_header:
        arguments.add(cpp_header, format = "-cpp-header=%s")
        arguments.add(cpp_source, format = "-cpp-source=%s")
        arguments.add("-cpp-include=%s" % ctx.attr.cpp_header)
        arguments.add("-c-include=%sc_abi.h" % artifact_root)
    arguments.add(abi_manifest, format = "-abi-manifest=%s")
    if ctx.attr.graphics_host:
        arguments.add("-graphics-host=%s" % ctx.attr.graphics_host)
    for graphics_binding in ctx.attr.graphics_bindings:
        arguments.add("-graphics-type=%s" % graphics_binding.split("|")[0])

    dependency_contracts = depset(
        direct = [
            dep[TtxPackageInfo].contract_archive
            for dep in ctx.attr.deps
        ],
        transitive = [
            dep[TtxPackageInfo].transitive_contracts
            for dep in ctx.attr.deps
        ],
        order = "postorder",
    )
    for contract in dependency_contracts.to_list():
        arguments.add(contract, format = "-dep=%s")

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

    object_files = []
    outputs = [complete_archive, contract_archive, abi_manifest, header]
    if ctx.attr.cpp_header:
        outputs.extend([cpp_header, cpp_source])
    for index, source in enumerate(ctx.files.sources):
        source_name = source.basename[:-4]
        unit_name = "unit_%d_%s" % (index, source_name)
        object_file = ctx.actions.declare_file(
            artifact_root + unit_name + ".o",
        )
        arguments.add_joined(
            [source.path, object_file.path],
            join_with = "|",
            format_joined = "-unit=%s",
        )
        object_files.append(object_file)
        outputs.append(object_file)

    if ctx.files.resources:
        resource_object = ctx.actions.declare_file(
            artifact_root + "resources.o",
        )
        arguments.add(resource_object, format = "-resources-object=%s")
        object_files.append(resource_object)
        outputs.append(resource_object)

    ctx.actions.run(
        inputs = depset(
            direct = [ctx.file.manifest] + ctx.files.sources + ctx.files.resources,
            transitive = [dependency_contracts, dependency_abi_manifests],
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
    semantic_compilation_outputs = cc_common.create_compilation_outputs(
        objects = depset(object_files),
        pic_objects = depset(object_files),
    )
    include_root = ctx.bin_dir.path
    if ctx.label.package:
        include_root += "/" + ctx.label.package
    api_compilation_context = None
    api_compilation_outputs = None
    if ctx.attr.cpp_header:
        api_compilation_context, api_compilation_outputs = cc_common.compile(
            actions = ctx.actions,
            name = ctx.label.name + "_api",
            cc_toolchain = cc_toolchain,
            feature_configuration = feature_configuration,
            srcs = [cpp_source],
            public_hdrs = [cpp_header],
            private_hdrs = [header],
            includes = [include_root],
            compilation_contexts = [
                ctx.attr._runtime[CcInfo].compilation_context,
            ],
        )
    compilation_outputs = semantic_compilation_outputs
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

    package_compilation_context = cc_common.create_compilation_context(
        headers = depset([header]),
        system_includes = depset([include_root]),
    )
    generated_cc_info = CcInfo(
        compilation_context = package_compilation_context,
        linking_context = linking_context,
    )
    dependency_cc_infos = [dep[CcInfo] for dep in ctx.attr.deps]
    package_contracts = depset(
        direct = [contract_archive],
        transitive = [dependency_contracts],
        order = "postorder",
    )
    package_abi_manifests = depset(
        direct = [abi_manifest],
        transitive = [dependency_abi_manifests],
        order = "postorder",
    )
    published_outputs = [
        complete_archive,
        contract_archive,
        abi_manifest,
        header,
    ]
    if ctx.attr.cpp_header:
        published_outputs.extend([cpp_header, cpp_source])
    library = linking_outputs.library_to_link
    if library.static_library:
        published_outputs.append(library.static_library)
    if library.pic_static_library and library.pic_static_library != library.static_library:
        published_outputs.append(library.pic_static_library)

    return [
        cc_common.merge_cc_infos(
            direct_cc_infos = [generated_cc_info],
            cc_infos = dependency_cc_infos + native_cc_infos + [ctx.attr._runtime[CcInfo]],
        ),
        TtxPackageInfo(
            identity = ctx.attr.package_name,
            version = ctx.attr.version,
            complete_archive = complete_archive,
            contract_archive = contract_archive,
            transitive_contracts = package_contracts,
            abi_manifest = abi_manifest,
            transitive_abi_manifests = package_abi_manifests,
            artifact_id = artifact_id,
            graphics_host = ctx.attr.graphics_host,
            graphics_bindings = ctx.attr.graphics_bindings,
            graphics_shader = ctx.attr.graphics_shader,
            cpp_compilation_context = api_compilation_context,
            cpp_objects = api_compilation_outputs.objects if api_compilation_outputs else [],
            cpp_pic_objects = api_compilation_outputs.pic_objects if api_compilation_outputs else [],
        ),
        DefaultInfo(files = depset(published_outputs)),
    ]

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
        resources = attr.label_list(
            allow_files = True,
            doc = "Candidate resource files available beneath the Package root.",
        ),
        deps = attr.label_list(
            providers = [TtxPackageInfo],
            doc = (
                "Dependent TTX Packages whose Contract Archives and native " +
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
        spirv_target = attr.string(
            default = "vulkan1.0",
            values = ["vulkan1.0"],
            doc = "SPIR V validation environment for Shader member products.",
        ),
        graphics_host = attr.string(
            doc = "Package contextual route for the selected graphics Host requirement.",
        ),
        graphics_bindings = attr.string_list(
            doc = "Hosted Type routes paired with native Descriptor provider symbols.",
        ),
        graphics_shader = attr.string(
            doc = "Package contextual route for the Shader Program used by hosted draws.",
        ),
        cpp_header = attr.string(
            doc = "Repository relative include path for the generated C++ API.",
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
        "Compiles one authored TTX Package into Complete and Contract " +
        "Archives plus separate native member objects."
    ),
)

def _ttx_cpp_api_impl(ctx):
    package = ctx.attr.package[TtxPackageInfo]
    if package.cpp_compilation_context == None or not package.cpp_objects:
        fail("ttx_cpp_api requires a Package that emits one C++ facade")

    cc_toolchain = find_cc_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    compilation_outputs = cc_common.create_compilation_outputs(
        objects = depset(package.cpp_objects),
        pic_objects = depset(package.cpp_pic_objects),
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
    facade = CcInfo(
        compilation_context = package.cpp_compilation_context,
        linking_context = linking_context,
    )
    outputs = package.cpp_objects + package.cpp_pic_objects
    library = linking_outputs.library_to_link
    if library.static_library:
        outputs.append(library.static_library)
    if library.pic_static_library and library.pic_static_library != library.static_library:
        outputs.append(library.pic_static_library)
    return [
        cc_common.merge_cc_infos(
            direct_cc_infos = [facade],
            cc_infos = [ctx.attr.package[CcInfo]],
        ),
        DefaultInfo(files = depset(outputs)),
    ]

_ttx_cpp_api = rule(
    implementation = _ttx_cpp_api_impl,
    attrs = dict(
        CC_TOOLCHAIN_ATTRS,
        package = attr.label(
            mandatory = True,
            providers = [TtxPackageInfo, CcInfo],
        ),
    ),
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
    doc = "Publishes the generated C++ facade for one TTX Package.",
)

def ttx_native_provider(name, **kwargs):
    """Publishes one target selected native provider and its symbol inventory."""
    _ttx_native_provider(
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
    resources = native.glob(
        [prefix + "**/*"],
        allow_empty = True,
        exclude = [manifest, prefix + "**/*.ttx"],
    )
    if "debug" not in kwargs:
        kwargs["debug"] = select({
            "//toolchain:debug_mode": "full",
            "//conditions:default": "none",
        })
    _ttx_package(
        name = name,
        manifest = manifest,
        resources = resources,
        sources = sources,
        version = version,
        **kwargs
    )

def ttx_cpp_api(name, package, **kwargs):
    """Publishes one Package C++ facade without coupling it to semantic use."""
    _ttx_cpp_api(
        name = name,
        package = package,
        **kwargs
    )

def _ttx_application_entry_impl(ctx):
    package = ctx.attr.package[TtxPackageInfo]
    if package.artifact_id != "x86_64-sysv-linux":
        fail("ttx_application requires the x86_64-sysv-linux artifact")

    artifact_root = ctx.label.name + "/"
    source_file = ctx.actions.declare_file(artifact_root + "entry.cpp")
    arguments = ctx.actions.args()
    arguments.add("-application")
    arguments.add(package.complete_archive, format = "-complete=%s")
    arguments.add(package.abi_manifest, format = "-abi-manifest=%s")
    arguments.add("-app-member=%s" % ctx.attr.app_member)
    arguments.add("-artifact=%s" % package.artifact_id)
    arguments.add(source_file, format = "-source=%s")
    if package.graphics_host:
        arguments.add("-graphics-host=%s" % package.graphics_host)
    for graphics_binding in package.graphics_bindings:
        parts = graphics_binding.split("|")
        arguments.add("-graphics-type=%s" % parts[0])
        arguments.add("-graphics-descriptor=%s" % parts[1])
    if package.graphics_shader:
        arguments.add("-graphics-shader=%s" % package.graphics_shader)
    dependency_contracts = [
        contract
        for contract in package.transitive_contracts.to_list()
        if contract.path != package.contract_archive.path
    ]
    for contract in dependency_contracts:
        arguments.add(contract, format = "-dep=%s")

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
            dependency_contracts + dependency_abi_manifests,
        ),
        outputs = [source_file],
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
    _, compilation_outputs = cc_common.compile(
        actions = ctx.actions,
        name = ctx.label.name + "_native",
        cc_toolchain = cc_toolchain,
        feature_configuration = feature_configuration,
        srcs = [source_file],
        compilation_contexts = [
            ctx.attr._runtime[CcInfo].compilation_context,
        ],
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
            cc_infos = [
                ctx.attr.package[CcInfo],
                ctx.attr._runtime[CcInfo],
            ],
        ),
        DefaultInfo(files = depset([source_file] + compilation_outputs.objects)),
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
        _runtime = attr.label(
            default = "//tetrodotoxin/runtime:application",
            providers = [CcInfo],
            doc = "The runtime realization selected by the App product.",
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
