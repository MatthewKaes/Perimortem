# Embedded Foreign Parser

Foreign is an embedded Dialect admitted explicitly by a CPU-executable parent.
It is not a top-level source envelope.

The parser owns the source-local inventory of external data and Callable
requirements. Provider selection, object files, process addresses, relocation,
native lowering, and linking remain outside the parser.

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
package or provider.

Every accepted block contributes ordered declarations to one private
source-local `foreign` surface:

- `const` is a read-only external Addressable, not a compile-time Constant;
- `state` is a writable external Addressable; and
- `func` is a bodyless external Callable with complete parameter and result
  Layouts.

A dot selects declared external data. An arrow invokes a declared external
Callable. Only declared names resolve; ambient linker symbols cannot legalize
an undeclared source use.

Publication exposes a declaration only on the private Foreign surface. It does
not republish the symbol through the containing source or Package.

## Semantic handoff

Each declaration must become a complete semantic import owner while its Tokens
are consumed. It retains the ABI selector, symbol, capability, documentation,
attributes, and real Type or Layout edges. It retains no provider, process
address, target relocation, token range, or second placeholder declaration.

## Status

Foreign is specified but not implemented by the current parser, semantic model,
native compiler, or Archiver. Library fixtures exercise the intended grammar
only.
