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
successful direct and staged semantic source names to their Monographs. The
accepted production transaction later adds restored dependency Monographs and
ordered post pass.

Installed Dialects may retain state across interpretations. Workspace explicitly
destroys every retained Monograph before destroying every installed Dialect.
Both phases finish before Arena release, so Monograph and Dialect destructors
may still use their Arena backed state.

Package Storage owns filesystem acquisition, confinement, logical route
normalization, and successful read caching. Workspace keeps each authored
semantic name separate and supplies its Arena so every successful Content path
and byte view remains valid for the semantic island lifetime.

`import_source` copies each semantic name, diagnostic path, and source body
into the Workspace Arena before interpretation. It returns the published
Monograph reference on success. The staged Package operation opens Storage
with that Arena and enters its retained Content views directly into the same
semantic transaction. This private retained input path exists because a raw
View does not identify its allocator. It avoids copying every Package source a
second time while keeping direct caller input safe.

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
retain the semantic name, diagnostic path, and source bytes
-> reject an already published exact semantic name
-> create Tokenizer and Cursor with the diagnostic path
-> require opening comment Documentation
-> parse `dialect : Type;`
-> select the exact installed Dialect
-> call its interpret operation with the same Cursor and Arena
-> publish the semantic name only for an engaged Monograph
```

Failed envelope parsing, unknown Dialect dispatch, and failed interpretation
publish no semantic name. Duplicate semantic imports leave the first Monograph
unchanged, and a failed name may be retried.

The implemented local Package path receives the package root, root semantic
name, root logical route, and accumulated Errors. Its transaction is:

```text
stage exact semantic name and package path
-> ask Package Storage for one confined same object read
-> submit the retained Content path and bytes to the retained import transaction
-> reject a duplicate semantic source name
-> create Tokenizer and Cursor with the diagnostic path
-> require opening comment Documentation
-> parse `dialect : Type;`
-> select the exact installed Dialect
-> call its interpret operation with the same Cursor and Arena
-> retain the resulting Monograph by authored semantic name
-> stage Package Source bindings in authored order
```

Environment owns the universal source envelope because it already owns the
Dialect family and every lifetime produced by dispatch. A concrete Dialect owns
only its body grammar and concrete Monograph.

Unknown Dialects and malformed envelopes are source errors. A failed
interpretation publishes no source name binding. Staged acquisition, envelope,
dispatch, and interpretation failures do not stop later FIFO entries. The
operation reports failure only for failures encountered during that call, so
preexisting diagnostics do not reject an otherwise successful Package. A
complete staged transaction returns its retained root Monograph.

Workspace does not yet restore dependency Archives or invoke a Monograph post
pass. Those completion steps remain W02 work.

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

Package Storage performs confined reads and supplies each retained diagnostic
path and byte view. Package Source retains the separate authored local name and
logical route. Workspace owns staging; Package owns path and repository policy.
Concrete Dialects may retain their own lookup and completion structures inside
their Monographs without adding a generic Environment Namespace.
