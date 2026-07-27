# Tetrodotoxin

Tetrodotoxin is the concrete host for TTX. TTX supplies lexical bytecode and a
closed target independent semantic substrate. Tetrodotoxin owns the concrete
source Dialects that interpret that substrate, the Environment that hosts them,
and the tooling boundaries for runtime and package artifacts.

## Component map

1. [`language`](language/) defines the stateful Dialect interface, its Monograph
   root, and syntax fragments shared by concrete Dialects.
2. [`environment`](environment/) owns the Workspace that installs Dialects,
   imports authored source, retains graph allocation, and resolves imported
   Monographs by authored source name.
3. [`package`](package/) owns the authored Package Dialect shape, exact
   Dependency requests, exact Source name to path bindings, and the Package
   Monograph model. Its interpreter remains incomplete.
4. [`library`](library/) owns Library language semantics, built in CPU Types,
   native CPU assembly, and the future reusable CPU compiler.
5. [`app`](app/) owns startup profiles, lifecycle policy, and generated platform
   entry semantics.
6. [`scene`](scene/) owns managed state, signals, render facts, and Scene
   lifecycle roles.
7. Top level `render` and `shader` folders own their future concrete Dialects.
   `foreign` owns embedded FFI grammar admitted by CPU capable parent Dialects.
8. [`linker`](linker/) owns source independent objects, symbols, relocations,
   target encoding, and native archive construction.

Path, namespace, and Bazel target describe the same owner. Every top level
target owns the complete `folder/**/*.cpp` and `folder/**/*.hpp` tree.

## Source import

Environment owns the intended source transaction:

```text
authored source name, diagnostic path, and borrowed authored bytes
-> Environment::Workspace::import_source
-> one Tokenizer and forward Cursor
-> required opening Documentation
-> Language::Parser::Dialect
-> exact installed Dialect instance
-> Dialect::interpret
-> concrete Dialect::Monograph
-> Workspace source name lookup
```

`Language::Dialect` is intentionally stateful. Environment constructs each
installed Dialect in its graph Arena, supplies the Workspace as its shared TTX
registry, and keeps the Dialect alive while any of its Monographs remain
queryable.

`Language::Dialect::Monograph` is the common Abstract root for one interpreted
source island. A concrete Monograph owns its Dialect semantics and borrows the
Arena, opening Documentation, and host Dialect retained by Environment.

There is no separate Source lifetime object, static parser function map,
Frontend, Container, or Environment Namespace. Environment owns the transaction
because it already owns the Dialect state, graph allocation, and retained
Monographs that give interpretation its lifetime.

The API shape above is present, but the current Package interpreter cannot yet
complete a valid import. It is an ownership contract, not current behavioral
evidence. The current Workspace import parameter still combines its semantic
lookup key with the Tokenizer diagnostic path. Package integration must keep
those two inputs distinct now that Source names are authored explicitly.

## Semantic ownership

TTX remains the shared vocabulary for Abstract, Type, Value, Addressable,
Callable, Layout, Documentation, Attribute, Alias, Invalid, and their common
supporting models.

`Library::Language` owns the semantics that are not universal across Dialects:
Expression, Binding, Projection, Constant and its value domains, Generic and its
materializations, concrete scalar Types, and Static and Self invocation
distinctions. These owners retain real TTX edges rather than copying the TTX
model.

App, Scene, and other CPU capable Dialects may use Library language contracts
where their authored semantics require them. They do not become Library
Monographs, and Library never builds a shadow graph for them.

## Dependency direction

The active host direction is:

```text
Environment -> Package -> Language -> TTX -> Perimortem
Library -> TTX -> Perimortem
```

Package supplies the first concrete Language Dialect shape for Environment,
although its current import path remains incomplete. Library acquires a
Language dependency when its concrete top level Dialect is exposed. Its current
semantic and assembler surface depends directly on TTX.

## Terminal tools

Library assembly and future compilation consume completed CPU facts retained by
their real Dialect owners. Shader owns SPIR V assembly. Linker owns
source independent object, relocation, target format, and native archive
machinery.

Package will eventually own confined package input and durable package
distribution. Those products are not part of the current Package target and are
not implied by a successfully interpreted Package Monograph.

See [tetrodotoxin_design.md](tetrodotoxin_design.md) for the detailed ownership
and transaction contract.
