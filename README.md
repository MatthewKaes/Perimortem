# Tetrodotoxin

Tetrodotoxin is an extensible language and toolchain platform for building
domain specific systems. It brings source languages, shared semantic meaning,
Packages, compilers, editor tooling, and a native runtime together under one
design.

Large systems often contain several languages even when only one of them looks
like ordinary application code. Package manifests, reusable libraries,
application policy, scene state, render contracts, and shaders each ask
different questions. Tetrodotoxin lets each of those domains keep a language
that fits its work while still participating in one program.

![Tetrodotoxin editor preview](extension/media/ttx-preview.png)

## One platform, several languages

A Tetrodotoxin language is a Dialect. Each Dialect owns its grammar, semantic
objects, diagnostics, and contribution to program completion. TTX supplies the
small vocabulary that lets independently designed Dialects share Types, values,
addresses, Callables, Layouts, source locations, and documentation.

Those shared facts live together in a Workspace. Package gives sources stable
names and reproducible dependencies. Puffer opens the same Workspace for the
command line and editor. Compilers and linkers derive native or GPU products
from the completed program, while Perimortem supplies the runtime foundation
used by generated applications.

This means a new Dialect can join an existing package graph, editor session,
and build without translating its model into a universal declaration tree. It
shares only the contracts that another language or tool can genuinely use.

## The platform

### TTX semantic vocabulary

[TTX](ttx/README.md) defines the shared lexical and semantic contracts. It keeps
exact identity and value flow available across language boundaries while each
Dialect retains its richer model.

### Languages and application models

[Tetrodotoxin](tetrodotoxin/README.md) provides a family of Dialects that can be
used together:

* Package names dependencies, sources, resources, and durable Archives
* Library defines reusable CPU code and data
* App describes startup and application policy
* Scene models long lived interactive state and lifecycle
* Render declares GPU facing contracts
* Shader implements those contracts for GPU execution
* Foreign connects authored CPU code to an external ABI

Projects can add Dialects for their own problem domains without adding another
semantic host around the toolchain.

### Workspace and Packages

Environment gives related source results one lifetime and completes them as a
single semantic island. Package makes that island reproducible through explicit
dependency versions, semantic source names, confined resources, and Archives.
The [standard Packages](packages/ttx/README.md) provide the Memory, Math, System,
and Graphics APIs used by the included application models.

### Developer experience

The [Tetrodotoxin extension](extension/README.md) brings source understanding,
navigation, formatting, diagnostics, and native debugging into Visual Studio
Code. [Puffer](puffer/README.md) is the command and editor host that assembles a
Workspace and coordinates the requested products.

### Compilers and Terminal products

Library can compile CPU code, Shader can produce GPU modules, Package can write
semantic Archives, and Linker can combine native products into programs. Each
component owns its output format. Tetrodotoxin calls an output independent of
the live Workspace a Terminal product.

### Native runtime foundation

Perimortem is the C++ runtime layer beneath Tetrodotoxin. It provides memory,
system, serialization, compression, image, and rendering services used by the
toolchain and generated programs. Its APIs remain useful to ordinary C++
applications, while Tetrodotoxin Packages expose selected runtime services to
authored languages.

The Perimortem name remains in runtime namespaces and Package identities because
it describes that concrete layer. Tetrodotoxin is the product and platform that
brings the complete repository together.

## Explore Tetrodotoxin

* [Tetrodotoxin overview](tetrodotoxin/README.md) introduces Dialects,
  Workspaces, Packages, and Terminal products
* [TTX overview](ttx/README.md) explains the shared semantic vocabulary
* [Language integration](tetrodotoxin/language/README.md) explains how a custom
  Dialect joins the platform
* [Library](tetrodotoxin/library/README.md) documents the reusable CPU language
* [App](tetrodotoxin/app/README.md), [Scene](tetrodotoxin/scene/README.md),
  [Render](tetrodotoxin/render/README.md), and
  [Shader](tetrodotoxin/shader/README.md) describe the included application
  models
* [Puffer](puffer/README.md) documents the command and editor host

## Build and try the editor

The current development environment targets x86 64 Linux with Clang and Bazel.
Windowed applications use Wayland and the Vulkan backend uses the installed
Vulkan loader and driver.

Build the repository with:

```sh
bazel build //...
```

Run the complete unit suite with:

```sh
bazel run //validation:unit_tests --config=debug
```

Build the Visual Studio Code extension with:

```sh
./extension/package.sh
```

Adding `--install` installs the generated VSIX after packaging it.

## Project status

Tetrodotoxin is an active research and development platform. Its distribution
is designed as a self contained SDK for editing, packaging, compiling, linking,
and debugging Tetrodotoxin projects. The supported development host is x86 64
Linux with Wayland and Vulkan.
