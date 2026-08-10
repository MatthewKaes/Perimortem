# Render

Render is Tetrodotoxin's interface language for rendering. It is comparable to a
typed pipeline interface or shader schema. A Render source declares the values,
resources, and Stages a format requires, while a Shader supplies one concrete
implementation.

Use Render when application and Shader code need to agree on a semantic
interface before a GPU representation is chosen. This supports cross language
checking and leaves another backend free to derive its own representation. The
corresponding cost is that a project must define the Render contract instead of
relying on one backend's reflection records as the interface.

Canonical grammar reference: [Render.g4](grammar/Render.g4).

```ttx
// Rendering interface.
dialect : Render;
```

## Render contracts

A Render contract can declare:

* ordinary value Addressables
* constant and push data
* resources and their binding Attributes
* required Shader Stages
* parameter and result Layouts for each Stage
* built in values, locations, sets, slots, and read capabilities

Each fact retains its own semantic identity. A resource binding that needs both
a set and a slot uses two Attributes rather than packing them into one opaque
record.

Render defines these Attribute keys:

* `@set(number)`, `@slot(number)`, and `@location(number)` carry exact unsigned
  indices.
* `@builtin("name")`, `@address_space("name")`, and
  `@capability("name")` carry names interpreted by Render.
* `@read` and `@write` state independent resource capabilities.

Each Attribute has at most one scalar value. Duplicate keys on one declaration
are invalid. Shader uses the same keys when it implements the corresponding
Render fact. Other Dialects do not acquire these meanings merely because they
share TTX Attribute storage.

Placement is part of each Attribute's meaning:

* `@set` and `@slot` apply only to a resource. When either is present, the
  resource supplies both parts of its logical binding.
* `@location` and `@builtin` apply only to a named Stage parameter or result
  entry. One entry cannot declare both.
* `@address_space` applies to a resource or push value whose storage domain is
  part of the Render contract.
* `@capability` applies to a Stage or to the enclosing Render Structure when
  the requirement covers all of its nested Stages.
* `@read` and `@write` apply only to resources. They are independent, so a
  contract can require either direction or both.

Render rejects an Attribute on the wrong semantic carrier, a value with the
wrong scalar kind, a duplicate key, or a mutually exclusive combination.
Shader must satisfy the exact admitted facts and cannot repair a mismatch by
choosing a convenient target binding during lowering.

Stage entry Attributes precede the named Layout entry:

```ttx
public stage Fragment[
  @location(0) .color : Math::Vec4D,
] -> [
  @location(0) .color : Math::Vec4D,
];
```

## Layout and representation

Stage parameters and results use exact TTX Layouts. Matching Layout shape is
necessary for fitting but does not alone establish vector, resource, address
space, or ABI identity. Render Types and Attributes state the additional
semantic requirements explicitly.

Fields, Types, and Stage Callables use their corresponding access domains:

* named value Addressables are selected with `.`
* nested Types are selected with `::`
* Stage Callables are selected and invoked with `->` where the consuming
  language permits invocation.

## Shader relationship

Render owns the interface. [Shader](../shader/README.md) selects one exact
Render contract, provides the required Stage bodies, proves their Types and
Layouts, and lowers the completed result for a GPU target.

Runtime graphics submission is a separate consumer of completed render facts.
It does not redefine Render grammar or Shader identity.

## Persistence

Render is a persistent Dialect. Its payload records the Types, Addressables,
Attributes, Stage Callables, Layouts, and publication facts needed to construct
an equivalent Render contract in a fresh Workspace. GPU storage classes,
bindings chosen by a target, and SPIR-V words remain outside that payload.
