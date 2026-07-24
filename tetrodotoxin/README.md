# Tetrodotoxin

Tetrodotoxin is this repository's concrete host for TTX. It owns source
lifetime, confined loading, exact package resolution, repositories, package
construction, the binding graph, concrete semantic products, parser policy,
compiler inputs, linker integration, materialization, and durable package
representation that TTX leaves host-defined.

TTX remains the authority for Token bytecode and shared Abstract, Type, Layout,
Expression, Addressable, Callable, Documentation, and Body contracts.

## Component map

- [`concept`](concept/) owns Tetrodotoxin-wide values shared across concrete
  components, including exact authored Package Resolution.
- [`parser`](parser/) owns deterministic Cursor consumption. Its README indexes
  one source contract for every top-level Dialect and the embedded Foreign
  Dialect.
- [`parser/package`](parser/package/) owns the root `package.ttx` transaction:
  its Source parser consumes the Package envelope and its Workspace confines
  member loading to one pinned package root.
- [`model`](model/) owns Source lifetime, Environment, concrete Dialect
  identities, Namespace, the parsed and resolved Package models, Render,
  Shader, and other semantic products.
- [`compiler`](compiler/) owns compilation-local target representation and
  terminal production from a finalized semantic graph.
- [`linker`](linker/) owns source-independent object and ELF/archive packaging.
- [`archiver`](archiver/) owns durable package buffers and source-free restored
  Package state.

## Production direction

```text
package directory
-> Parser::Package Workspace and root package.ttx transaction
-> Model-owned Package::Source declarations
-> member Source, Environment, and semantic products
-> finalized Package graph
-> Compiler and Linker terminal products
-> Archiver durable package buffer
```

Each arrow crosses an owner boundary:

- Parser never opens files or searches repositories.
- Puffer is a CLI/LSP client of Tetrodotoxin and owns no reusable Package
  behavior.
- Model never stores parser bookmarks as unfinished semantics.
- Compiler never rebuilds the Package as a shadow Type graph.
- Archiver never owns source loading or runtime objects.

## Active targets

The owner-shaped top-level libraries are:

```text
//tetrodotoxin:concept
//tetrodotoxin:model
//tetrodotoxin:parser
//tetrodotoxin:compiler
//tetrodotoxin:linker
//tetrodotoxin:archiver
```

Target existence proves only that the owner builds. It does not prove that a
documented Dialect parser, semantic evaluator, runtime, target path, or
source-free restoration path executes.

See [tetrodotoxin_design.md](tetrodotoxin_design.md) for the cross-component
transaction. Each linked owner document is authoritative for its narrower
contract.
