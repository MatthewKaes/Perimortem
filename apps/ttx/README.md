# TTX applications

From the repository root:

```sh
puffer/package.sh --debug
.bin/puffer-sdk/bin/puffer apps/ttx/echo/build.ttx
.bin/puffer-sdk/bin/puffer apps/ttx/scene_lifetime/build.ttx

apps/ttx/echo/build/Application
apps/ttx/scene_lifetime/build/SceneLifetime
```

The existing Echo and Scene Lifetime VS Code launch configurations run the same
SDK preparation and Build commands. Echo reads lines until `quit`, `exit`, or
EOF. Scene Lifetime fades the splash into its animated title. Shift restarts
the splash and Space exits from the title.

Each Build selects a Package export, SDK providers, and a native Terminal. Its
Environment output directory is relative to the Build source. The Package and
application sources contain no Bazel target names.

Bazel builds Puffer with LLVM and embedded LLD, plus the Perimortem runtime
archive. Puffer acquires standard Package sources, emits native and shader
objects through the retained Terminals, and invokes Clang for the generated C++
entry and platform startup arguments. Clang calls Puffer itself as the linker.

This SDK supports the Linux x86_64 native target and the statically installed
providers named by these examples. The local package script links the standard
source repository into `.bin/puffer-sdk`. It requires this checkout, Clang,
platform development libraries, and Wayland/Vulkan for the graphical app.
Independent SDK distribution and dynamically loaded providers remain separate
work. The Build invocation recompiles its source closure and writes products
directly, without an incremental cache or transactional publication of a group
of executables.
