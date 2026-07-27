# Environment

`tetrodotoxin/environment` owns the host that assembles interpreted source into
one queryable TTX environment. The complete folder builds as
`//tetrodotoxin:environment`.

The current owner is `Environment::Workspace`. There is no separate Source,
Container, Namespace, or finalized Graph object.

## Workspace lifetime

Workspace owns one Arena containing installed Dialect instances, parser
products, and retained Monographs. It retains those Dialect instances in order,
binds exact authored names to them, and binds imported semantic source names to
their Monographs.

Installed Dialects may retain state across interpretations. Workspace explicitly
destroys every Dialect before releasing the Arena, so a Monograph never loses
the host context that created it while the Workspace remains alive.

Authored source names, diagnostic paths, and content bytes are borrowed inputs.
The source provider owns their backing storage for as long as retained semantic
values borrow those views. Environment owns graph allocation and interpretation
lifetime, not filesystem acquisition.

## Dialect installation

The intended toolchain contract installs concrete Dialects through the type
system:

```text
workspace.install_dialect<Package::Dialect>("Package")
```

Workspace must reject a duplicate name, construct one concrete Dialect with
itself as the TTX registry, and retain the resulting instance. Installing the
same concrete C++ Dialect under another name creates another distinct stateful
instance.

The name map is a real Environment dispatch surface. It is not a TTX class
registry or a copied semantic model.

The installation template and owner containers are present, but the current
Dialect construction and destruction contracts are incomplete. No successful
Package installation is established yet.

## Source import

`Workspace::import_source` receives a route, authored contents, Documentation
context, and lexical Errors owner. Its intended forward transaction is:

```text
reject duplicate semantic source name
-> create Tokenizer and Cursor
-> require opening comment Documentation
-> parse `dialect : Type;`
-> select the exact installed Dialect
-> call its interpret operation with the same Cursor and Arena
-> retain the resulting Monograph by authored source name
```

Environment owns the universal source envelope because it already owns the
Dialect family and every lifetime produced by dispatch. A concrete Dialect owns
only its body grammar and concrete Monograph.

Unknown Dialects and malformed envelopes are source errors. A failed
interpretation publishes no source name binding.

The Workspace API and ownership shape are present. The current Package
interpreter cannot yet complete a successful import. The current `route`
parameter also supplies both the semantic lookup key and Tokenizer diagnostic
path. Package integration must split those inputs because an authored Source
name is independent of its package path. This transaction is not behavioral
evidence.

## Registry query

Workspace is the shared Abstract registry supplied to every installed Dialect.
Its contextual resolution looks up an imported semantic source name:

```text
workspace.resolve_context(source_name)
-> retained Dialect::Monograph
-> Ttx::Concept::Invalid when absent
```

The returned edge is a real retained Abstract. Missing names use TTX Invalid so
semantic queries remain total and chainable.

## Boundary

Environment owns no concrete Package or Library declarations, target records,
runtime values, linker objects, archive format, or package repository search.

Package or another source provider will eventually perform confined reads. It
must feed each member's authored local name, actual diagnostic path, and bytes
into Workspace while retaining every borrowed buffer. Concrete Dialects may
retain their own lookup and completion structures inside their Monographs
without adding a generic Environment Namespace.
