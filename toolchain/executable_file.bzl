"""Rule for exposing a downloaded executable through `bazel run`."""

def _executable_file_impl(ctx):
    source = ctx.file.src
    executable = ctx.actions.declare_file(source.basename)
    ctx.actions.symlink(
        output = executable,
        target_file = source,
        is_executable = True,
    )
    return DefaultInfo(
        executable = executable,
        files = depset([executable]),
        runfiles = ctx.runfiles(files = [executable, source]),
    )

executable_file = rule(
    implementation = _executable_file_impl,
    attrs = {
        "src": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
    },
    executable = True,
)
