# Perimortem Memory

Perimortem Memory provides allocation domains and containers for C++ systems.
Core owns the language-visible `Object<T>` value and its physical contract.
`Abi::Core` exposes that contract to generated code while Memory may compose
the carrier into internal convenience owners.

`Memory::Dynamic::Record<T>` adapts C++ lifetime management for tooling
and compiler state whose owner cannot use one Arena transaction. It retains one
`Core::Object<>` carrier, constructs `T` in its payload, and releases the carrier
when the final Record is destroyed.

Record is not a language Type, ABI carrier, control block, or alternate Object
representation. Generated code never names it. The Core Object descriptor and
the control data adjacent to each allocation remain authoritative for both
generated Objects and Records.

`Core::Object<T>` is the C++ reference for Library `Object[T]`. Both use one
empty-capable word, recover element capacity from Bibliotheca, and expose the
same writable buffer through every alias. `reserve` replaces only the selected
handle when it must grow, `is_shared` reports another owned handle, and `clone`
performs an explicit independent copy. The erased `Core::Object<>`
specialization is the physical ABI carrier used by generated authored Objects
and Record. Containers such as Dynamic Bytes may build their own copy-on-write
policy from those primitives; Object does not impose one.

## Worker ownership

Dynamic Records remain on the worker that created them. A Record is not a worker
transfer mechanism and may not be copied or destroyed by another worker.

A worker boundary can borrow a read only View while the producing worker or
another explicit owner guarantees that storage remains alive for the complete
call. Data that must outlive that borrow is copied into storage owned by the
receiver. This keeps thread handoff visible without adding shared heap guards,
root registries, or a moving collector to every Object access.

Reference cycles must be avoided through ordinary ownership design. Views are
the borrowed edge for temporary observation. Graphs with longer lifetimes keep
a clear direction for their owning Record references.

## Allocation and failure

Bibliotheca, Perimortem's page allocator, supplies Object and Record storage
local to one worker. Core interprets the adjacent Object descriptor when the final
reservation is released.
The runtime may ask for cleared storage when that makes initialization faster,
but the language still defines each Type's default value.

A Record becomes visible only after its value is initialized. If the runtime
cannot allocate the required storage, it reports a fatal process error.

The worker that creates a Record remains responsible for its references and
underlying storage until final release.
