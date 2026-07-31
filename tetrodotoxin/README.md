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
   Monograph model. It also owns confined Package Storage.
4. [`library`](library/) owns Library language semantics, built in CPU Types,
   native CPU assembly, and the future reusable CPU compiler.
5. [`app`](app/) owns startup profiles, lifecycle policy, and generated platform
   entry semantics.
6. [`scene`](scene/) owns state, signals, retained declared children, render
   submission facts, and Scene lifecycle roles.
7. Top level `render` and `shader` folders own their future concrete Dialects.
   `foreign` owns embedded FFI grammar admitted by CPU capable parent Dialects.
8. [`linker`](linker/) owns source independent objects, symbols, relocations,
   target encoding, and native archive construction.
9. A future `graphics` folder owns language neutral retained graphics children
   and ordered submission facts. No current folder or target implements that
   contract.

Path, namespace, and Bazel target describe the same owner. Every top level
target owns the complete `folder/**/*.cpp` and `folder/**/*.hpp` tree.

## Production transaction

The accepted production transaction composes the owners without merging them:

```text
Bazel declares exact inputs and terminal outputs
-> Puffer selects a compile mode and constructs one Workspace
-> Workspace installs the selected concrete Dialects
-> Workspace stages the explicit root name and path
-> Package Storage performs each confined read
-> Workspace retains bytes and parses the universal source envelope
-> the exact installed Dialect constructs its real Monograph
-> Workspace retains the Monograph under the authored semantic name
-> Package stages members and restores dependencies from Package Archives
-> Workspace invokes one ordered post pass after staging drains
-> Puffer stops terminal work when diagnostics exist
-> concrete owners provide typed Package Archive and Linker Object Module data
-> Linker emits the requested native product
-> Puffer writes only declared outputs
```

Textual source diagnostics and lower level validation traces remain separate.
`Ttx::Lexical::Errors::Report` is created only by an owner with an explicit
source name, source body, and token range. Filesystem, Archive, and Repository
owners log the exact local failure facts through `Diagnostics::Log` and return
failure. Workspace or Puffer then uses the authored dependency, source, or
compile request context to publish the user facing error. A low level log does
not substitute for that source diagnostic, and a source diagnostic does not
discard the detailed validation trace.

`Language::Dialect` is intentionally stateful. Environment constructs each
installed Dialect in its graph Arena, supplies the Workspace as its shared TTX
registry, and keeps the Dialect alive while any of its Monographs remain
queryable.

`Language::Dialect::Monograph` is the common Abstract root for one interpreted
source island. A concrete Monograph owns its Dialect semantics and borrows the
Arena, opening Documentation, and host Dialect retained by Environment. The
planned lifecycle gives every retained Monograph one ordered post pass and lets
its concrete Dialect encode and restore only its own opaque durable payload.
Post pass returns failure without receiving `Lexical::Errors`. The concrete
owner logs graph details that would otherwise be lost, and Workspace retains
the authored source identity needed for an actionable diagnostic.

There is no separate Source lifetime object, static parser function map,
Frontend, Container, or Environment Namespace. Environment owns the transaction
because it already owns the Dialect state, graph allocation, and retained
Monographs that give interpretation its lifetime.

Workspace implements direct envelope dispatch and the confined local Package
stage. It drains exact Source names and logical routes in FIFO order, retains
successful Monographs, and continues after independent failures. Dependency
restoration and ordered post pass remain absent, so the transaction above is
not yet complete.

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

Package supplies the first concrete Language Dialect shape and confined local
source path for Environment. Library acquires a Language dependency when its
concrete top level Dialect is exposed. Its current semantic and assembler
surface depends directly on TTX.

## Terminal products

Library assembly and future compilation consume completed CPU facts retained by
their real Dialect owners. Shader owns SPIR V assembly. Linker owns
source independent object, relocation, target format, and native archive
machinery.

`Linker::Object::Module` is the accepted native typed terminal. It owns one
coherent set of sections, symbols, and relocations.
`Package::Archive::Archive` is the separate durable semantic terminal used for
source free restoration and native artifact and symbol location. Package
Archive never owns Linker object bytes.

Package owns confined Storage. Namespace `Package::Archive` owns the completed
value on `Archive`, validated Format 1 decoding through `Reader`, and canonical
encoding through `Writer`. Namespace `Package::Repository` owns the concrete
Repository transaction and its `Input`, `Artifact`, and `Output` declaration
values. It provides exact declared Archive selection, separate native artifact
path lookup, and normalized declared-only publication routes keyed by Package
identity, Version, and artifact ID.
Archive restoration remains planned. Library owns CPU lowering. App and Scene
retain their own completed facts. Linker owns static archives, shared
libraries, and complete executable production. Puffer orchestrates these
owners but does not replace any of them with a generic product registry.

Final ELF linkage remains in repository code. A host linker is only an
independent consumer for a static archive checkpoint, never the production
implementation of a Tetrodotoxin executable.

Package Archive Format 1 can now be constructed, read, and written
independently. Exact Repository selection can retain a successful Archive from
caller-Arena file bytes while leaving native byte loading and output writing to
their later consumers.
None of the complete terminal transaction, source free restore, Puffer compile
orchestration, shared library output, or executable output is implemented by
the current targets.

See [tetrodotoxin_design.md](tetrodotoxin_design.md) for the detailed ownership
and transaction contract.
