# Foreign

Foreign is an embedded FFI declaration block, similar to an `extern` block in a
systems language. It adds external variables and functions to the semantic
context of a parent Dialect that supports CPU execution.

Use Foreign when Library or another CPU capable parent Dialect needs a
statically declared external ABI surface. The surrounding source owns the
resulting semantic facts. Provider selection and native linking happen later,
which keeps one declaration meaningful across targets without turning Foreign
into another top level source Dialect or Monograph.

A Library Source owns exactly one Foreign context. It parses the possible block
Comment once and passes that Documentation to the context. Repeated blocks with
the same ABI merge atomically into the same identity. Exact repeated State or
Callable declarations retain the first identity, while a changed declaration
rejects the later block without publishing any of its new declarations.

Canonical grammar fragment: [Foreign.g4](grammar/Foreign.g4).

## Foreign block

```ttx
foreign "C" {
  expose state external_readonly : Unsigned_64;
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

* `public state` is an external Addressable that the parent may read and write.
* `expose state` is an external Addressable that the parent may read but cannot
  write.
* `func` is a bodyless external Callable with complete parameter and result
  Layouts.

Foreign rejects `const`. Importing a compile time value requires a separate
loader or embedding contract rather than treating external storage as a
Constant. Private declarations are unreachable because Foreign has no member
authority of its own.

## Access

Foreign data and Callables remain separate query domains:

```ttx
foreign.external_counter = foreign.external_readonly;
foreign -> external_add(foreign.external_counter, foreign.external_readonly);
```

`.` selects one declared external Addressable. `->` selects and invokes one
declared external Callable. A native symbol that was not declared in the source
cannot make an authored access legal.

## Visibility

State visibility controls write authority through the source local `foreign`
context. Functions are public. No Foreign declaration is automatically
republished through the containing Monograph or Package.

State and Callable names occupy separate query categories, so one external
symbol spelling may support both `.` and `->`. A same category repeat must be
the exact declaration retained earlier. Separate Sources retain separate
Foreign identities, so matching declarations across sources do not collide at
the semantic layer.

The parent language supplies Documentation, Type identity, writability, and
invocation semantics. Foreign retains the ABI selector and external symbol
facts without copying those parent contracts.

## Linking boundary

Provider selection, object files, dynamic libraries, process addresses,
relocations, and native lowering are target facts used to produce and consume
native Terminal products. They satisfy the declared Foreign identities after
semantic analysis. They do not define source legality.

Library compilation publishes each unresolved State or Function as one Linker
Import. The build request supplies target-specific logical Providers, and
Package compilation requires exactly one provider for every Import before it
publishes native artifacts or Archives. The resulting ABI Manifest and Package
artifact record retain the same selected provider without adding it to Foreign
or the semantic graph.

See [Library](../library/README.md) for a CPU language host and
[TTX semantics](../../ttx/ttx_semantics.md) for Addressable and Callable.
