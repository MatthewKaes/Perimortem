# Embedded Foreign Parser

`Tetrodotoxin::Foreign::Parser` owns the embedded dialect
admitted explicitly by a CPU executable parent. Foreign is not a top level
source envelope.

Foreign therefore has no entry in the top level Language parser map and does
not construct a `Language::Source`. The selected concrete parent parser invokes
the static Foreign parser on the same Cursor while consuming its body.

The parser owns the source local inventory of external data and Callable
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
source local `foreign` surface owned by `Library::Language`:

1. `const` is a read only external `Library::Language::Addressable`, not a
   compile time Constant;
2. `state` is a writable external `Library::Language::Addressable`; and
3. `func` is a bodyless external `Library::Language::Callable` with complete
   parameter and result Layouts.

A dot selects declared external data. An arrow invokes a declared external
Callable. Only declared names resolve; ambient linker symbols cannot legalize
an undeclared source use.

Publication exposes a declaration only on the private Foreign surface. It does
not republish the symbol through the containing source or Package.

## Semantic handoff

Each declaration must become a complete Library Language semantic import owner
while its Tokens are consumed. It retains the ABI selector, symbol, capability,
documentation, attributes, and real Type or Layout edges. It retains no
provider, process address, target relocation, token range, or second placeholder
declaration.

## Status

Foreign is specified but not implemented by a current parser, semantic owner,
Library compiler, or Package Distribution. Library fixtures exercise the
intended grammar only.
