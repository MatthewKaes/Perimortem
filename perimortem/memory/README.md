# Perimortem Memory

Perimortem Memory provides allocation and managed-object storage for the
runtime. Languages describe how long their values must remain valid and which
fields refer to other objects. Memory decides how those objects are stored,
traced, and reclaimed.

These runtime details stay out of the language model. A program works with an
Object identity, not an allocator pointer or a garbage-collector record.

## Garbage realms

A garbage realm is a group of managed objects owned by one worker. The worker
may allocate objects, change them, add or remove roots, and run collection. No
other worker changes that realm at the same time.

An Object handle is a nonnull reference to one object in the realm. Copying the
handle refers to the same object, so every copy observes the same changes. The
handle does not become a second language identity.

A realm can move to another worker at a safe handoff point. The runtime pauses
changes, transfers the whole realm, and resumes it on the receiving worker.
Object handles remain valid because the realm moves as one unit.

Individual managed objects do not move between realms. Code that needs data on
another worker has three choices:

* copy the value and create a new identity
* transfer the whole realm
* use immutable storage designed for sharing

Raw pointers and allocator reservations are not worker-transfer mechanisms.

## Roots and tracing

A root keeps an object alive. Stacks, running native calls, and live application
state register the roots they hold with the realm.

Library supplies a compact description of which Object fields may contain more
managed references. The collector follows that description when it traces the
object graph. It does not inspect source files, Package data, or a copied list
of language fields.

The collection strategy can change without changing program behavior. Object
identity, aliases, and field access remain the same whether the runtime uses
reference counts, tracing, or cycle collection.

## Allocation and failure

Bibliotheca, Perimortem's page allocator, supplies the worker-local storage used
by a realm. The realm may ask for cleared storage when that makes initialization
faster, but the language still defines each Type's default value.

An Object becomes visible only after all of its fields are initialized. If the
runtime cannot allocate the required storage, it reports a fatal process error.
No partly initialized Object becomes visible, so there is nothing for source
code to undo.

The worker that owns a realm also reclaims it. After a transfer, the receiving
worker becomes responsible for both the objects and their underlying storage.
