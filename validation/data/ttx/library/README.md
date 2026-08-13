# Library fixture reference

These source files provide Library acceptance inputs and focused semantic
rejections. They are fixtures rather than applications; a test that opens one
file defines the exact production behavior it observes.

## Source files

| File | Purpose |
| --- | --- |
| [`source_acceptance.ttx`](source_acceptance.ttx) | focused production Workspace acceptance for completed Definition identities and native Function requests |
| [`value_acceptance.ttx`](value_acceptance.ttx) | focused production Workspace acceptance for defaults, scalar operations, value access, invocation, Address chaining, and Swizzle fitting |
| [`executable_acceptance.ttx`](executable_acceptance.ttx) | focused production Workspace acceptance for executable Block order, Locals, writes, branches, loops, loop control, Match, returns, and invocation statements |
| [`broad.ttx`](broad.ttx) | broad Library source corpus covering declarations, Layouts, access chains, expressions, control flow, Foreign, Struct, Object, and Enumeration syntax |
| [`foreign_triad.ttx`](foreign_triad.ttx) | one exposed State, public State, and Callable under the C ABI |
| [`native.ttx`](native.ttx) | compact unsigned-integer and Foreign source paired with `native_harness.c` |
| [`native_harness.c`](native_harness.c) | C definitions and observation point for the native fixture |

`broad.ttx` intentionally contains more language surface than any one narrow
test needs. It enters acceptance only when a production Workspace can complete
the represented capabilities; tokenization alone is not acceptance evidence.

## Executable acceptance

[`executable_acceptance.ttx`](executable_acceptance.ttx) keeps every completed
Library statement owner in one Function body. Its test opens the source through
the production Workspace, completes every lifecycle barrier, then inspects the
real statement identities and authored order. The fixture does not claim
lowering or runtime execution.

## Access model

The fixtures keep Addressable, Type, and Callable access separate:

```ttx
packet.width                    // Addressable from Packet's named Layout
Graphics::Image                 // Type through contextual resolution
Packet -> identity(limit)       // Static Callable invocation
packet -> identity()            // Self Callable invocation
```

`.` selects one Addressable from an applicable named Layout. The Addressable may
later lower to stack storage, a Struct-relative offset, an Object-relative
offset, or a folded value.

`::` traverses Abstract contexts to a Type. Intermediate Package, Monograph,
Alias, source, and Type contexts remain their real identities.

`->` owns the complete Callable invocation: receiver role, registered Callable
selection, argument Pack fitting, and result Pack flow are one source construct.

`Packet` deliberately declares both Static and Self Callables named `identity`.
Static has no implicit Self parameter. Self reserves parameter entry zero for
the selected receiver value.

## Layouts and Packs

Parameters and results are Layout descriptors, while returns and swizzles
produce Packs whose output Layouts are fitted by their receivers:

```ttx
public classify : func = [.value : Unsigned_64] -> [
  .accepted : Bool,
  .adjusted : Unsigned_64,
]

return (.adjusted = value + 1, .accepted = value > 0);
```

The `:` spellings name descriptor slots; the `=` spellings name produced Pack
slots. Neither is postfix Address access. Positional flow remains positional,
and `packet.[width, height]` selects named values for repacking.

`access[index]` is the optional reference request for `Access[T]`. It does not
create a Library Option Type. Indexed assignment writes through an engaged
reference and leaves the receiver unchanged when it is absent.
`value:[index]` and `value:[start, count]` are safe value forms. Scalar access
supplies the element Type's default when selection misses. Range count must
fold to a supported nonnegative value and produces exactly that many Pack
entries; it has no implicit empty-View fallback.

## Library declarations

`broad.ttx` contains these representative declarations:

- `Packet` is an inline Struct with public Fields, private state, and Static and
  Self Callables.
- `Mode` is an `Unsigned_8` Enumeration with ordered named cases.
- `Session` is an Object whose aliases share nonnull reference identity.
- `PacketAlias` retains an Alias to the real `Packet` Type.
- `PrivateOps` is a private Struct selected as a Type context for Static calls.

Fields keep exposure separate from writability. `expose state progress` permits
external reads of the same Field identity while restricting writes to hosted
code.

## Foreign declarations

[`foreign_triad.ttx`](foreign_triad.ttx) declares the three external categories:

1. `imported_readonly` is an exposed read-only external State.
2. `imported_state` is a writable external Addressable.
3. `imported_function` is a bodyless external Callable.

The fixture selects data with `foreign.name` and invokes the Callable with
`foreign -> imported_function(...)`. The `"C"` selector identifies the ABI;
native provider selection remains outside source lookup.

## Native pair

[`native.ttx`](native.ttx) and [`native_harness.c`](native_harness.c) describe a
two-call interaction. The Library source increments private state by 20, copies
it to imported state, adds an imported bias of 2, and returns the result.

The corresponding C observation is:

```text
22 42
```

After both calls, the imported state is 40. `library_native` is the exported TTX
entry. `PrivateOps`, local state, and helper calls remain source-local, while
the three Foreign names are supplied by the C file.

The standalone Bazel target for `native_harness.c` proves only that the C side
of this contract compiles. Native output evidence links an emitted TTX Terminal
with this harness, executes it, and observes the exit status and exact text. A
source fixture or harness build is not a substitute for that round trip.

## Focused rejection inputs

Each remaining TTX file isolates one source condition:

| File | Condition represented by the source |
| --- | --- |
| [`public_parameter_private_type.ttx`](public_parameter_private_type.ttx) | a public parameter exposes the private `Hidden` Type |
| [`public_result_private_type.ttx`](public_result_private_type.ttx) | a public result exposes the private `Hidden` Type |
| [`ordinary_bodyless.ttx`](ordinary_bodyless.ttx) | an ordinary Library Function ends as a bodyless declaration |
| [`foreign_with_body.ttx`](foreign_with_body.ttx) | a Foreign Callable supplies an authored body |
| [`foreign_named_scope.ttx`](foreign_named_scope.ttx) | Foreign uses the obsolete named-definition spelling `private C : foreign` |
| [`foreign_undeclared_symbol.ttx`](foreign_undeclared_symbol.ttx) | source invokes an undeclared Foreign Callable |
| [`foreign_const.ttx`](foreign_const.ttx) | Foreign attempts to import compile time data without a loader contract |
| [`foreign_private_state.ttx`](foreign_private_state.ttx) | Foreign declares State that its parent cannot reach |
| [`foreign_private_function.ttx`](foreign_private_function.ttx) | Foreign declares a private Callable that its parent cannot reach |
| [`foreign_exposed_function.ttx`](foreign_exposed_function.ttx) | Foreign applies data-only exposed visibility to a Callable |
| [`foreign_exposed_write.ttx`](foreign_exposed_write.ttx) | Library attempts to write an exposed Foreign State |
| [`duplicate_name.ttx`](duplicate_name.ttx) | two root declarations use the same name in one category |
| [`new_without_expected_type.ttx`](new_without_expected_type.ttx) | `new` appears in an inferred declaration without an expected Object Type |
| [`bare_return.ttx`](bare_return.ttx) | bare `return;` appears with an empty Layout result rather than concrete `Void` |
| [`dialect_led_callable.ttx`](dialect_led_callable.ttx) | a Library source uses a Scene-style lifecycle role declaration |

These files preserve the authored distinction being tested without serving as a
general error taxonomy.

See the
[Library language reference](../../../../tetrodotoxin/library/README.md),
[Foreign reference](../../../../tetrodotoxin/foreign/README.md), and
[TTX semantics](../../../../ttx/ttx_semantics.md).
