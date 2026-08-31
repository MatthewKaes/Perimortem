# Perimortem Memory

Tetrodotoxin tools and the programs they generate both need fast ownership that
can be understood at the language boundary. Perimortem Memory gives them a
shared allocation foundation through the same C ABI used by generated code.

Core owns the language visible Object carrier and its physical contract.
Generated native products call that C surface directly. The ABI Terminal owns
the external symbol spellings it emits, so Perimortem does not need a second
set of C++ to C bridge classes merely to describe its own functions.

`Memory::Dynamic::Record<T>` currently builds on the same carrier for compiler
and tooling state whose lifetime extends beyond one Arena transaction. It
constructs `T` in the payload and lets the final Record release the carrier.
Record is a remaining C++ migration owner, not part of the Object ABI.

Record remains a C++ lifetime tool rather than another language Type or Object
representation. Generated code can rely on the Core Object descriptor and the
control data beside each allocation, while tooling gains a convenient owner for
longer lived state.

The C Object handle is the physical ABI carrier used by Library `Object[T]`,
generated authored Objects, and Record. It is one pointer whose adjacent
Bibliotheca preface identifies the immutable runtime descriptor and current
reservation count. A null pointer represents absence and is not an Object.

Object itself does not provide typed buffer policy. Memory Buffer can grow or
clone one fixed allocation, while owners such as Dynamic Bytes retain their own
logical size and copy on write rules. Growing replaces only the selected handle
because aliases continue to name their original fixed allocation.

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
