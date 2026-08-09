# Foreign

Foreign embeds an external ABI surface inside a CPU-capable parent Dialect. It
does not create an independent source Dialect or Monograph; its declarations
become semantic facts owned by the surrounding source.

Grammar prototype: [Foreign.g4](grammar/Foreign.g4).

## Foreign block

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

The quoted selector chooses the foreign language and ABI grammar. It does not
select a Package dependency or native provider.

Every declaration names an external semantic contract:

- `const` is a read-only external Addressable. It is not a compile-time
  Constant.
- `state` is an external Addressable with the write capability defined by the
  parent CPU language.
- `func` is a bodyless external Callable with complete parameter and result
  Layouts.

## Access

Foreign data and Callables remain separate query domains:

```ttx
foreign.external_counter = foreign.external_limit;
foreign -> external_add(foreign.external_counter, foreign.external_limit);
```

`.` selects one declared external Addressable. `->` selects and invokes one
declared external Callable. A native symbol that was not declared in the source
cannot make an authored access legal.

## Visibility

Publication inside the block controls visibility on the source-local `foreign`
context. It does not automatically republish an external symbol through the
containing Monograph or Package.

The parent language supplies Documentation, Attributes, Type identity,
writability, and invocation semantics. Foreign retains the ABI selector and
external symbol facts without copying those parent contracts.

## Linking boundary

Provider selection, object files, dynamic libraries, process addresses,
relocations, and native lowering are terminal concerns. They satisfy the
declared Foreign identities after semantic analysis; they do not define source
legality.

See [Library](../library/README.md) for the first CPU language host and
[TTX semantics](../../ttx/ttx_semantics.md) for Addressable and Callable.
