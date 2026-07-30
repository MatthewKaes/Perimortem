# Environment

`tetrodotoxin/environment` owns the host that assembles interpreted source into
one queryable TTX environment. The complete folder builds as
`//tetrodotoxin:environment`.

The current owner is `Environment::Workspace`. There is no separate Source,
Container, Namespace, or finalized Graph object.

## Workspace lifetime

Workspace owns one Arena containing installed Dialect instances, retained
source bytes, parser products, and retained Monographs. It retains Dialect
instances in installation order, binds exact authored names to them, and binds
successful direct semantic source names to their Monographs. The accepted
production transaction later adds the staged source queue, restored dependency
Monographs, and ordered post pass.

Installed Dialects may retain state across interpretations. Workspace explicitly
destroys every retained Monograph before destroying every installed Dialect.
Both phases finish before Arena release, so Monograph and Dialect destructors
may still use their Arena backed state.

Package input owns filesystem acquisition and confinement. It gives Workspace
each authored semantic name, diagnostic path, and byte result. Workspace owns
the byte lifetime for every staged and retained source so semantic values never
borrow from a transient filesystem or process buffer.

The direct import API copies each diagnostic path and source body into the
Workspace Arena before tokenization. A future staged transaction supplies those
same direct inputs after confined Package acquisition.

## Dialect installation

The intended toolchain contract installs concrete Dialects through the type
system:

```text
workspace.install_dialect<Package::Dialect>("Package")
```

Workspace rejects an exact duplicate before construction, copies the authored
name into its Arena, constructs one concrete Dialect with itself as the TTX
registry, and retains the resulting instance. Installation reports true only
for that successful publication. Installing the same concrete C++ Dialect
under another name creates another distinct stateful instance.

The name map is a real Environment dispatch surface. It is not a TTX class
registry or a copied semantic model.

Unknown Dialect diagnostics enumerate the retained authored names in
installation order. No concrete Package installation or interpretation is
established by this Environment contract.

## Source import

Direct import receives an exact semantic name, diagnostic path, source bytes,
and accumulated Errors. Its implemented transaction is:

```text
retain the diagnostic path and source bytes
-> reject an already published exact semantic name
-> create Tokenizer and Cursor with the diagnostic path
-> require opening comment Documentation
-> parse `dialect : Type;`
-> select the exact installed Dialect
-> call its interpret operation with the same Cursor and Arena
-> copy and publish the semantic name only for an engaged Monograph
```

Failed envelope parsing, unknown Dialect dispatch, and failed interpretation
publish no semantic name. Duplicate semantic imports leave the first Monograph
unchanged, and a failed name may be retried.

The accepted future production path stages the same semantic name and
diagnostic path. Its expanded transaction is:

```text
stage exact semantic name and package path
-> ask Package input for one confined same object read
-> retain the returned bytes
-> reject a duplicate semantic source name
-> create Tokenizer and Cursor with the diagnostic path
-> require opening comment Documentation
-> parse `dialect : Type;`
-> select the exact installed Dialect
-> call its interpret operation with the same Cursor and Arena
-> retain the resulting Monograph by authored semantic name
-> stage Package Source bindings in authored order
-> restore dependency Monographs from exact Package Archives
-> invoke one post pass on every retained Monograph in retained order
```

Environment owns the universal source envelope because it already owns the
Dialect family and every lifetime produced by dispatch. A concrete Dialect owns
only its body grammar and concrete Monograph.

Unknown Dialects and malformed envelopes are source errors. A failed
interpretation publishes no source name binding.

No current Workspace queue drains Package members, restores dependency
Archives, or invokes a Monograph post pass. The current Package interpreter
cannot yet complete a valid Package import. Those expanded steps remain a
planned owner contract rather than current behavioral evidence.

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
runtime values, linker objects, Archive envelope, or package repository search.

Package input performs confined reads and supplies each member's authored local
name, actual diagnostic path, and bytes. Workspace owns staging and retains the
bytes; Package owns path and repository policy. Concrete Dialects may retain
their own lookup and completion structures inside their Monographs without
adding a generic Environment Namespace.
