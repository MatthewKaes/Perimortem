# Embedded Foreign Parser

`Tetrodotoxin::Foreign::Parser` will own the embedded FFI grammar admitted by a
CPU capable parent Dialect. Foreign is not an Environment installed top level
Dialect and does not construct an independent Monograph.

The concrete parent Dialect invokes the embedded parser with its current Cursor,
Arena, Documentation context, and semantic owner. Foreign contributes completed
import facts directly to that parent Monograph.

Provider selection, object files, process addresses, relocation, native
lowering, and linking remain outside the parser.

## Source contract

```ttx
foreign "C" {
  public const external_limit : Unsigned_64;
  public state external_counter : Unsigned_64;
  public func external_add[
    .left : Unsigned_64,
    .right : Unsigned_64,
  ] -> Unsigned_64;
}
```

The quoted selector chooses the FFI and ABI grammar. It does not select a
Package Dependency or native provider.

Every accepted block contributes ordered declarations to one private
source local `foreign` surface owned by its CPU parent:

1. A `const` declaration creates a read only external TTX Addressable rather
   than a compile time Constant.
2. `state` is an external TTX Addressable enriched with the parent language's
   write capability.
3. A `func` declaration creates a bodyless external TTX Callable with complete
   parameter and result Layouts.

A dot selects declared external data. An arrow invokes a declared external
Callable. Only declared names resolve. Ambient linker symbols cannot legalize
an undeclared source use.

Publication inside the block exposes a declaration only on the private Foreign
surface. It does not republish the symbol through the containing Monograph or
Package.

## Semantic handoff

Each declaration becomes one complete parent language import owner while its
Tokens are consumed. It retains the ABI selector and external symbol,
capability and publication facts, Documentation and Attributes, and real TTX
Type or Layout edges.

It retains no provider, process address, target relocation, token range, or
second placeholder declaration.

Library is the first intended host and supplies its own Constant, invocation,
and write capability semantics. App and Scene may admit the same embedded
grammar only through their CPU language portion.

## Status

Foreign grammar and acceptance fixtures exist, but no active Foreign parser,
semantic import owner, Library compiler, or Package distribution integration is
implemented.
