# Foreign

Foreign is an embedded FFI declaration block, similar to an `extern` block in a
systems language. It adds external variables and functions to the semantic
context of a parent Dialect that supports CPU execution.

Use Foreign when Library or another CPU capable parent Dialect needs a
statically declared external ABI surface. The surrounding source owns the
resulting semantic facts. Provider selection and native linking happen later,
which keeps one declaration meaningful across targets without turning Foreign
into another top level source Dialect or Monograph.

Canonical grammar fragment: [Foreign.g4](grammar/Foreign.g4).

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

* `const` is an external Addressable that cannot be written. It is not a
  Constant evaluated at compile time.
* `state` is an external Addressable with the write capability defined by the
  parent CPU language.
* `func` is a bodyless external Callable with complete parameter and result
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

Publication inside the block controls which declarations are visible through
the `foreign` context in that source. It does not automatically republish an
external symbol through the containing Monograph or Package.

The parent language supplies Documentation, Attributes, Type identity,
writability, and invocation semantics. Foreign retains the ABI selector and
external symbol facts without copying those parent contracts.

## Linking boundary

Provider selection, object files, dynamic libraries, process addresses,
relocations, and native lowering are target facts used to produce and consume
native Terminal products. They satisfy the declared Foreign identities after
semantic analysis. They do not define source legality.

See [Library](../library/README.md) for a CPU language host and
[TTX semantics](../../ttx/ttx_semantics.md) for Addressable and Callable.
