# Cross-language Abstract conformance

This fixture implements the Chapters 1–3 carrier in C, C++, and Rust without
making any language's type system, object address, or representation the graph's
semantic authority.

## The graph under test

Rust owns three unrelated contracts:

- `Echo` owns the `print` operation.
- `Value` owns the `to_string` operation.
- `View::Bytes` proves that a Terminal may project borrowed bytes from a
  candidate.

The independently added Rust policy owns a fourth contract, `Visibility`, and
its `permit` operation. It does not share an API or implementation module with
`Echo`.

The C++ root contains this mixed graph:

```text
C++ route host
├─ direct    → Rust Echo
│              └─ value → C Integer(4)
├─ layered   → C++ Echo simulacrum
│              └─ echo  → Rust Echo
│                          └─ value → C Bool(true)
├─ cpp-value → Rust Echo
│              └─ value → C++ Value
│                          └─ text → C++ byte producer
├─ twice     → C++ Echo simulacrum → C++ Echo simulacrum → Rust Echo
├─ <empty>   → C++ Echo simulacrum → Unknown
├─ <binary>  → Rust Visibility policy
└─ not-echo  → None
```

C walks the root's complete concept routes, negotiates each candidate against
the exact Rust-owned `Echo` requirement, and invokes every positive result. It
does not know which route selects which language, how many routes exist, or
which concrete owners sit below a C++ layer.

## Values stay in the graph

The integer and Boolean are not copied into a Rust enum or converted to raw
bytes at the language boundary. Each is a real C-owned Abstract that separately
satisfies `Value` and `View::Bytes`. Invoking `Value.to_string` returns a
Context-retained Pack containing that same value producer.

The C++ example makes the separation harder to fake. Its `Value` stores another
Abstract for its text. `to_string` returns a Pack containing that text producer,
and only then does Rust negotiate `View::Bytes` against the Pack's exact
producer. This proves that `Value`, the returned producer, and its byte
representation need not share an identity or native object model.

After semantic negotiation succeeds, the bytes Terminal asks the candidate for
its typed byte projection. Each value supplies that projection through its own
operations. The Terminal assembles borrowed chunks and passes the complete bytes
to Rust for printing. Adding a value needs no provider inventory or native
object lookup in the consumer. The C++ value returns a separate text producer,
while the C values return themselves, so the path exercises both relationships.

The expected output is:

```text
Hello from [Rust]: 4
C++ Simulacra layering: Hello from [Rust]: true
Hello from [Rust]: value from C++
C++ Simulacra layering: C++ Simulacra layering: Hello from [Rust]: true
C++ Simulacra layering: C++ Simulacra points to Unknown
```

## The drivers do not own the domains

`rust/ttx.rs` translates Rust mechanics into the C17 handles, sinks, immutable
operation tables, Layouts, and Packs. The C++ owners use `ttx/concept/abstract.*`,
`ttx/concept/interface.*`, and the standard Layout implementations under
`ttx/model/layouts/`. Both Abstract and Interface cross the ABI as one pointer
to a capability containing its operation table. Private state belongs to that
capability's implementation, and only its callbacks recover that state. Neither
language driver switches over Echo, Value, View::Bytes, or Visibility identities.

The canonical C++ Context retains every Pack for its caller. The integer and
Boolean live in separate source files over a reusable value owner, so adding the
Boolean did not change the integer. The second Rust Abstract is registered from
`rust/providers.rs`, which lets a new owner join without editing the ABI driver or
the original `Echo` owner.

The C++ Echo simulacrum keeps its selected child behind a private C++ pointer.
That storage choice remains unobservable. Alias forwards negotiation to the
referent, while an Addressable preserves its visible candidate when it projects
that referent's capabilities. The test places a C++ Addressable around a Rust
Echo and verifies that the new identity satisfies the same requirement. The
proxy's independent identity remains distinct from behavioral substitution.

C, C++, and Rust all keep synchronous callback state in the requesting call
frame. The borrowed sink operation table is each implementation's private route
back to that frame, so nested observations require no thread-local callback
registry, erased context pointer, fixed recursion depth, or graph-visible token.

Finally, C sends a Pack containing the C++ Value through the unrelated Rust
Visibility policy. The returned Pack retains the exact C++ producer, proving
that an independently introduced contract can compose with values it was never
written to recognize.

This proves contract-scoped simulacra and synthetic Interface composition. It
does not yet claim perfect equivalence across arbitrary stateful histories,
failures, concurrency, or reconstruction.

The canonical C++ Context now owns every retained Pack. Each language transfers
an owned Layout snapshot through the same C ABI, and the temporary C++ source
Layout is destroyed before the Pack is inspected. A partial Named snapshot also
outlives both temporary child Layouts while preserving Unknown, None, and one
exact route at their original paths. Composite snapshots preserve their two
child boundaries, retain partial fitted children, and reject an implicit
reassociation even when the enumerable leaves match. Ranged proves repeated,
zero, mismatched, and unsettled cardinalities through the narrow Extent
Constant contract. Reindexed preserves repeated, reordered producer occurrences
and their mappings after the original source Layout has ended. An additional
Rust module constructs a temporary Layout whose vectors are destroyed before
C++ inspects the retained Pack. Graph Text observes the same mixed graph and
preserves its structured projections in deterministic output.

The Interface cases separately cover Unknown and rejection without execution,
changed policies, mismatched witness candidates, transparent Alias forwarding,
temporary Callable views, support allocation failures, and Addressable Route
and Extent views. The query helpers copy transient support inside the callback,
while Packs continue to borrow the original semantic producers.

The proof runs with the rest of the repository contract suite:

```sh
bazel run //validation:unit_tests
```
