# Library parser acceptance inputs

These are validation-only inputs and independent observable contracts for the
Library parser and native-artifact plan. They are not applications, packages,
or examples of the `apps/` directory layout. Parser, semantic, archive, Library
compiler, and Linker implementation slices consume these files unchanged. They
do not weaken an input or expectation to match an implementation.

`broad.ttx` is the broad grammar and graph fixture. `native.ttx` is the
smaller scalar native fixture, and `foreign_triad.ttx` isolates the three
kinds of native import. The other TTX files are one negative contract each.
Tokenization is executable today; semantic acceptance remains owned by the
later plan slices.

## Accepted grammar map

TTX supplies lexical bytecode and its minimal graph kernel. The table records
accepted Tetrodotoxin Library grammar pressure; it does not promote these
Dialect rules into universal TTX semantics.

| Fixture construct | Normative section and frozen meaning |
| --- | --- |
| `dialect : Library;` | Source Envelopes And Dialects selects the real Library Dialect before body evaluation. |
| `//`, including the empty comment line | Documentation preserves the ordered lines and the paragraph break on `Packet`. |
| marker and scalar `@...` forms | Attributes And Directives preserves the six ordered `Packet` attributes and the ordered parameter attributes. Library accepts marker, unsigned, signed, real, Bool, and quoted-byte scalar forms used here. |
| `public`, `expose state`, `private`, and `const` | Definition Modifiers keeps publication separate from evaluation. `observed_total` exports a read-only projection while its writable owner stays retained. Inside an embedded Foreign block, `public` publishes an import to the private source-local `foreign` surface rather than to package exports. |
| `PacketAlias : alias = Packet` | Names And Type Queries and Identity Resolution retain the Alias edge and resolve it to the real `Packet` Type. |
| `struct` and ordinary fields | Definitions and Members And Scoped Builtins construct the real inline `Packet` and `PrivateOps` Types with authored field order. |
| `object` and typed `= new` | Common Semantic Vocabulary and Functions require a real Managed `Session` Type. `new` performs expected-Type-driven empty construction and cannot infer the missing Type in `:= new`. |
| `enum[Unsigned_8]` | Enums retains the storage Type and the ordered decimal and hexadecimal cases. |
| `foreign "C" { ... }` | Embedded Foreign Imports selects the C ABI and explicitly declares the external symbols consumed by this source. `const` declares a read-only external Addressable, `state` declares a writable external Addressable, and `func` declares a bodyless external Callable. |
| state, const, field, and ordinary value definitions | Definitions retains distinct writable storage, stable compile-time results, fields, and ordinary runtime values. |
| same-name `Packet::identity` functions | Functions retains one Static and one Self Callable on separate lookup surfaces. The bare `self` receiver is parameter zero. |
| functions referring to later private owners | Member Order And Late Binding resolves `Packet::finish`, the source-local `foreign` surface, `PrivateOps`, `private_total`, and native `total` only after the declaration graph is complete. |
| direct, empty, unnamed, and named Layouts | Layouts preserves every authored parameter/result shape. Layout-field attributes stay ordered on their real parameter owners. |
| empty, positional, named, and indexed parentheses | Packs preserves the selected mode. Nested slices and swizzles produce positional flow, while named and indexed designators never mix. |
| `0...limit`, logical, comparison, arithmetic, unary, and postfix forms | Expressions and Access Chains apply the normative precedence ladder once and retain the selected Static, Self, field, index, slice, and swizzle operations. |
| `=`, `+=`, and `-=` | Assignment Statements requires the decoded left chain to prove write capability. Assignment never becomes an expression. |
| `return`, `if`, `while`, `for`, `match`, `break`, and `continue` | Statements And Control Flow produces explicit Body blocks and terminators. Empty results use `return Void;`, conditions fit Bool, and `for` reuses Layout grammar. |
| decimal, hexadecimal, real, Bool, quoted bytes, and byte arrays | Literals preserves the value domain without implicit String or NUL semantics. The fixture uses each domain either as an attribute or a semantic value. |

## Broad graph oracle

The root export surface contains the following real edges in authored
publication order within the addressable and function groups:

1. `Packet`, `Mode`, `Session`, and `PacketAlias`.
2. The read-only `observed_total` projection, `signature`, `default_packet`,
   and `dense_defaults`.
3. `broad_entry`, `reset`, `unnamed_layout`, and `classify`.

The retained root surface additionally owns the writable `observed_total`
state, `PrivateOps`, `private_total`, `mask`, `decode_table`, and
`private_packet`. The source also retains its private `foreign` import surface.
None of those private edges becomes a root export. The read-only
`observed_total` export is a distinct Addressable identity with the same name
and Type as the retained writable state.

The frozen Type and Layout expectations are:

- `Packet` is an inline Structure ordered as `width`, `height`, `ready`, then
  private `checksum`. Its Static and Self `identity` Callables are distinct;
  the Self signature includes `.self : Packet` at index zero. Its Body calls
  the later private Static `finish` owner.
- `Mode` stores `Unsigned_8` and retains `idle = 0`, `active = 1`, and
  `paused = 2` in that order.
- `Session` is a Managed Object. It retains writable `progress` and `token`
  state, exports only the read-only `progress` projection and `advance`, and
  retains private `token_value`.
- `PacketAlias` remains queryable as an Alias and resolves to `Packet`.
- `signature`, `dense_defaults`, both slice results, and `decode_table` use
  supported concrete `Fixed` materializations with their ordered arguments.
- `broad_entry` has a named two-parameter Layout and a direct
  `Unsigned_64` result. `reset` has empty parameter and result Layouts.
  `unnamed_layout` preserves two unnamed parameters and results. `classify`
  has one named parameter and named results ordered `accepted`, `adjusted`.
- `foreign -> library_observe` resolves a complete bodyless Foreign Callable
  whose external C symbol is `library_observe`. No defined Callable is
  bodyless.

A successful broad evaluation completes and seals every owner before the
Namespace is published. No token range, Cursor, incomplete Type, incomplete
Callable, or rejected owner is graph-reachable.

## Foreign import oracle

`foreign_triad.ttx` selects the C ABI and declares every external symbol it
consumes. Its private source-local `foreign` surface owns three ordered import
edges:

1. `imported_constant`, a read-only external Addressable. It is not a
   compile-time materialized Constant.
2. `imported_state`, a writable external Addressable.
3. `imported_function`, a bodyless external Callable with exact parameter and
   result Layouts.

The `public` modifier inside the block publishes those entries to that
source-local surface only. It does not export them from the Library package.
`foreign.imported_constant` and `foreign.imported_state` perform explicit data
lookup, while `foreign -> imported_function(...)` performs explicit Callable
lookup. The `"C"` string selects ABI identity; provider selection remains a
packaging or link concern.

## Native artifact oracle

`native.ttx` deliberately uses only `Unsigned_64` values, a Static local call,
one scalar mutable state owner, and all three C import kinds. `library_native`
is the only global TTX symbol. `PrivateOps::increment` and `total` are local.
The Foreign symbols `library_foreign_bias`, `library_foreign_state`, and
`library_foreign_add` are undefined in the produced object and are supplied by
`native_harness.c`.

The first invocation advances `total` from 0 to 20, writes 20 to the imported
state, reads the imported constant bias of 2, and returns 22. The second
advances `total` from 20 to 40, leaves the imported state at 40, and returns
42. The independently authored C harness prints exactly:

```text
22 42
```

It exits with status 0 only for those two results and the final imported state
value of 40. The eventual archive must expose only `library_native`, keep the
helper and TTX state local, emit a local call relocation for the helper, emit
undefined external object references for the imported constant and state, and
emit an undefined Foreign call relocation for `library_foreign_add`.

## Negative contracts

| File | Required rejection |
| --- | --- |
| `public_parameter_private_type.ttx` | A public parameter Layout cannot expose the private `Hidden` Type. A declaration prepass may reserve its real identity, but publication still rejects the private edge. |
| `public_result_private_type.ttx` | A public result Layout cannot expose the private `Hidden` Type under the same publication rule. |
| `ordinary_bodyless.ttx` | An ordinary Library Callable has no implementation owner and cannot end with `;`. |
| `foreign_with_body.ttx` | A Foreign Callable is a bodyless ABI promise and cannot own an authored Body. |
| `foreign_named_scope.ttx` | Foreign is an embedded Dialect block, not a named Definition kind. The obsolete `private C : foreign` spelling is rejected rather than retained as compatibility syntax. |
| `foreign_undeclared_symbol.ttx` | `foreign -> missing()` cannot fall back to ambient linker search. Every consumed external symbol must be declared in the source-local Foreign block. |
| `duplicate_name.ttx` | The second `duplicate` collides on the same root surface and publishes no edge. |
| `new_without_expected_type.ttx` | `new` cannot supply the Type required by an inferred `:=` declaration. The rejection is not a rejection of inference itself. |
| `bare_return.ttx` | Empty results require `return Void;`; bare `return;` is invalid. |
| `dialect_led_callable.ttx` | A Library source cannot use a Dialect-name-led lifecycle declaration. Scene owns that role-marking continuation. |

Every failure prevents a completed Source/Namespace result. Arena allocation
from a failed private transaction may remain unreachable.

## CPU executable and Managed gates

Library owns the reusable CPU compilation path, but the exact durable
representation for completed body facts remains unresolved. Scene lifecycle
Callables retain their Scene identity. A Static selected by App remains on its
actual producing Namespace, while App generated entry and lifecycle driver
facts remain on App. Those selected CPU facts use the same Library compiler
after Environment finalization. They do not become generated Library Sources
or shadow graphs.

The proof required by `object` also lacks an approved owner. This fixture does
not reopen TTX or assign either gap to a shadow Tetrodotoxin contract. The
frozen `object`, initializer, expression, and control flow source records the
required acceptance input while implementations that require those missing
contracts remain blocked.
