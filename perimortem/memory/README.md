# Perimortem Memory

Perimortem Memory provides allocation and reference counted managed Object
storage for the runtime. Languages describe value construction and destruction,
while Memory retains each live allocation until its final reference is released.

These runtime details stay out of the language model. A program works with one
nonnull Object identity rather than an allocator record. Copying its one word
handle retains the same identity, so every copy observes the same mutations.
Destroying a handle releases one reservation. The final release runs the
payload destructor before returning the allocation to Bibliotheca.

## Worker ownership

Managed allocations remain on the worker that created them. An Object handle is
not a worker transfer mechanism and may not be retained or released by another
worker.

A worker boundary can borrow a read only View while the producing worker or
another explicit owner guarantees that storage remains alive for the complete
call. Data that must outlive that borrow is copied into storage owned by the
receiver. This keeps thread handoff visible without adding shared heap guards, root
registries, or a moving collector to every Object access.

Reference cycles must be avoided through ordinary ownership design. Views are
the borrowed edge for temporary observation. Graphs with longer lifetimes keep
a clear direction for their owning Object references.

## Allocation and failure

Bibliotheca, Perimortem's page allocator, supplies Object storage local to one
worker.
The runtime may ask for cleared storage when that makes initialization faster,
but the language still defines each Type's default value.

An Object becomes visible only after all of its fields are initialized. If the
runtime cannot allocate the required storage, it reports a fatal process error.
No partly initialized Object becomes visible, so there is nothing for source
code to undo.

The worker that creates an Object remains responsible for its references and
underlying storage until final release.
