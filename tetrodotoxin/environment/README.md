# Environment

`tetrodotoxin/environment` owns the host that assembles interpreted source into
one queryable TTX environment. The complete folder builds as
`//tetrodotoxin:environment`.

`Environment::Workspace` is the public semantic island and import owner. It
composes `Dialects`, `Retention`, and `Resolution` as narrower Environment
state owners. There is no separate Source, Container, Namespace, or finalized
Graph object.

## Workspace lifetime

Workspace owns one Arena, direct source orchestration, confined Package FIFO
orchestration, and the exact global authored name map. Its composed objects
divide the remaining Environment state by lifetime and policy:

* `Dialects` owns installed names, exact lookup, concrete Dialect instances,
  and host destruction order
* `Retention` owns retained Monograph references, authored diagnostic origins,
  first discovery order, frozen link and finalize ranges, and Monograph
  destruction
* `Resolution` owns exact Package key traversal, source free Archive restore,
  Package root caching, Alias binding, and dependency failure attribution

Each object's declarations live in its matching header and its behavior lives
in its matching implementation. `workspace.cpp` contains only Workspace
construction, source and Package import orchestration, and Abstract behavior.
Resolution borrows the Workspace Arena, Dialects, and Retention explicitly. It
does not retain Workspace or Repository and is not an adapter around another
restoration owner.

Resolution exposes only its durable restored Package cache in its private
representation. Traversal keys, diagnostic hops, completed roots, and their
operations live only in `resolution.cpp`. One transaction Arena backs those
temporary vectors, so recursive restoration rents and releases one page chain
instead of repeatedly allocating small dynamic buffers. `Option<T&>` appears
only where absence is meaningful, such as the root stage having no owning
Package.

Installed Dialects may retain state across interpretations. Workspace member
order destroys Resolution, then Retention and its Monographs, then Dialects and
its concrete instances. Every phase finishes before Arena release, so
Monograph and Dialect destructors may still use their Arena backed state.

Each installed Dialect retains Workspace as its shared registry. Interpretation
also receives one source local Abstract context: Workspace for direct sources
and an ownerless root Package, or the exact owning Package Monograph for a
staged member. The two references serve different lookup scopes and neither is
copied into another context model.

Package Storage owns filesystem acquisition, confinement, logical route
normalization, and successful read caching. Workspace keeps each authored
semantic name separate and supplies its Arena so every successful Content path
and byte view remains valid for the semantic island lifetime.

`interpret_source` copies each semantic name, diagnostic path, and source body
into the Workspace Arena before interpretation. It returns the staged
Monograph reference on success. The caller must separately link and finalize
that range before its name becomes terminal publication. The staged Package
operation opens Storage
with that Arena and enters its retained Content views directly into the same
semantic transaction. The private retained input path exists because a raw
View does not identify its allocator. It avoids copying every Package source a
second time while keeping direct caller input safe.

## Dialect installation

The intended toolchain contract installs concrete Dialects through the type
system:

```text
workspace.install_dialect<Package::Dialect>("Package")
```

Workspace delegates installation to Dialects. Dialects rejects an exact
duplicate before construction, copies the authored name into the shared Arena,
constructs one concrete Dialect with Workspace as the TTX registry, and retains
the resulting instance. Installation reports true only for that successful
publication. Installing the same concrete C++ Dialect under another name
creates another distinct stateful instance.

The name map is a real Environment dispatch surface. It is not a TTX class
registry or a copied semantic model.

Unknown Dialect diagnostics enumerate the retained authored names in
installation order. No concrete Package installation or interpretation is
established by the Environment contract.

## Source import

Direct import receives an exact semantic name, diagnostic path, source bytes,
and accumulated Errors. Its implemented transaction is:

```text
retain the semantic name, diagnostic path, and source bytes
-> reject an already published exact semantic name
-> create Tokenizer and Cursor with the diagnostic path
-> require opening comment Documentation
-> parse the universal Dialect declaration
-> select the exact installed Dialect
-> call its interpret operation with the same Cursor, Arena, and Workspace
   interpretation context
-> retain the Monograph and stage its semantic name
-> return so the caller can invoke the separate link and finalize barriers
```

Failed envelope parsing, unknown Dialect dispatch, and failed interpretation
stage no semantic name. Duplicate semantic imports leave the first Monograph
unchanged, and a failed name may be retried. Raw lookup can observe a staged
identity so sources in the same batch can link forward. Link or finalize
failure discards the staged publication, allowing the exact name to be retried.

The local Package path receives the physical root, root semantic name, root
logical route, exact Package identity and Version, explicit Repository, and
accumulated Errors. Its transaction is:

```text
stage exact semantic name and package path
-> ask Package Storage for one confined same object read
-> submit the retained Content path and bytes to the retained import transaction
-> reject a duplicate semantic source name
-> create Tokenizer and Cursor with the diagnostic path
-> require opening comment Documentation
-> parse the universal Dialect declaration
-> select the exact installed Dialect
-> call its interpret operation with the same Cursor and Arena, using Workspace
   for the root or the exact owning Package as interpretation context
-> retain only the root by its Workspace global semantic name
-> bind every staged member through its actual owning Package
-> stage nested Package Source bindings in authored FIFO order
-> select each exact dependency Archive without native inputs
-> reconstruct the source free Package root from envelope facts
-> restore members through their installed Dialects in Archive order
-> bind completed dependency and member Alias edges through that Package root
-> freeze the complete retained range
-> link every Monograph once in first discovery order
-> when all links succeed, finalize every Monograph once in that same order
-> publish staged global names only after the complete range finalizes
```

Environment owns the universal source envelope because it already owns the
Dialect family and every lifetime produced by dispatch. A concrete Dialect owns
only its body grammar and concrete Monograph.

Unknown Dialects and malformed envelopes are source errors. A failed
interpretation publishes no source name binding. Staged acquisition failures
log the exact semantic name and logical route because the retained Package
model does not carry the original Source statement token. Envelope, dispatch,
and interpretation failures with retained text remain source errors. Those
failures do not stop later FIFO entries. The operation reports failure only for
failures encountered during that call, so preexisting diagnostics do not
reject an otherwise successful Package. A failed staging transaction abandons
its retained Monograph prefix and never enters dependency traversal. Resolution
therefore receives only complete staged input.

Resolution diagnoses exact active Package keys before selection. A completed
exact key reuses its retained root, while another Version for the same identity
is a conflict. Repository views may belong to another Arena, so Resolution
copies only durable Package identities, Dependencies and nested names, and
member Alias names. Opaque payloads remain borrowed for the concrete Dialect
restore call, which owns every durable fact it returns in the Workspace Arena.

An authored Dependency failure creates an Anchor from its aligned Span. Source
free descendants inherit that same source path, body, and default opening
focus while extending the exact Alias,
identity, and Version chain. One chain publishes at most one lowest Report and
independent authored Dependencies continue. A typed Repository failure without
an authored Anchor creates no source Report. Resolution returns either the root
Monograph or a populated `SelectionError`. It preserves the first exact
Repository category and uses `Unknown` when cycle, binding, restore, link, or
finalize rejection has no Repository category. Every hook in the active stage
continues after an independent failure so later Monographs can publish their
own diagnostics. Any link failure suppresses the complete finalize stage. Any
finalize failure prevents terminal publication of the complete staged range.

## Registry query

Workspace is the shared Abstract registry supplied to every installed Dialect.
That retained registry is independent from the source local Abstract passed to
each interpretation. Workspace contextual resolution looks up an imported
semantic source name:

```text
workspace.resolve_context(source_name)
-> retained Language::Monograph
-> Ttx::Concept::Invalid when absent
```

The returned edge is a real retained Abstract. Missing names use TTX Invalid so
semantic queries remain total and chainable.

## Boundary

Environment owns no concrete Package or Library declarations, target records,
runtime values, linker objects, Archive envelope, or package repository search.

Package Storage performs confined reads and supplies each retained diagnostic
path and byte view. Package Source retains the separate authored local name and
logical route. Repository performs exact selection and Archive owns its
validated envelope. Workspace owns staging, Resolution owns restoration, and
Retention owns the separate link and finalize barriers without retaining
either Package transaction owner.
Concrete Dialects may retain their own lookup and completion structures inside
their Monographs without adding a generic Environment Namespace.
