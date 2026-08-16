# Render

Render is Tetrodotoxin's language for describing the shared interface between a
program and its GPU stages. A Render source names the values, resources, and
stages that make up a rendering format. A Shader then provides one implementation
of that format.

The contract is checked before Tetrodotoxin chooses a GPU representation. This
lets CPU code, Shader code, and different graphics backends agree on the same
meaning without treating one backend's reflection data as the source of truth.

Canonical grammar reference: [Render.g4](grammar/Render.g4).

```ttx
// Rendering interface.
dialect : Render;
```

## Render contracts

A Render contract can declare:

- ordinary values
- constant and push data
- resources and their binding Attributes
- required Shader Stages
- parameter and result Layouts for each Stage
- built-in values, locations, sets, slots, and access capabilities

Each declaration stays independently inspectable. For example, a resource
binding that needs both a set and a slot uses two Attributes instead of hiding
them inside one backend-specific record.

Render defines these Attribute keys:

- `@set(number)`, `@slot(number)`, and `@location(number)` carry unsigned
  indices.
- `@builtin("name")`, `@address_space("name")`, and
  `@capability("name")` carry names interpreted by Render.
- `@read` and `@write` state independent resource capabilities.

Each Attribute has at most one scalar value. Duplicate keys on one declaration
are invalid. Shader uses the same keys when it implements the corresponding
Render fact. Other Dialects do not acquire these meanings merely because they
share TTX Attribute storage.

Placement is part of each Attribute's meaning:

- `@set` and `@slot` apply only to a resource. When either is present, the
  resource supplies both parts of its logical binding.
- `@location` and `@builtin` apply only to a named Stage parameter or result
  entry. One entry cannot declare both.
- `@address_space` applies to a resource or push value whose storage domain is
  part of the Render contract.
- `@capability` applies to a Stage or to the enclosing Render Structure when
  the requirement covers all of its nested Stages.
- `@read` and `@write` apply only to resources. They are independent, so a
  contract can require either direction or both.

Render rejects an Attribute on the wrong kind of declaration, a value of the
wrong kind, a duplicate key, or a combination that cannot be used together.
Shader must satisfy the declared contract. It cannot hide a mismatch by choosing
a convenient target binding later.

Stage entry Attributes precede the named Layout entry:

```ttx
public stage Fragment[
  @location(0) .color : Math::Vec4D,
] -> [
  @location(0) .color : Math::Vec4D,
];
```

## Layout and representation

Stage parameters and results use TTX Layouts. A matching shape is necessary,
but shape alone does not say whether a value is a vector, resource, address, or
part of a particular ABI. Render Types and Attributes describe those additional
requirements.

Fields, Types, and Stage Callables use their corresponding access domains:

- named values are selected with `.`
- nested Types are selected with `::`
- Stage Callables are selected and invoked with `->` where the consuming
  language permits invocation.

## Shader relationship

Render owns the interface. [Shader](../shader/README.md) selects one Render
contract, organizes its stages, and supplies the implementation. Each Shader
contains one Render layer built by the Render language already installed in the
Workspace. That layer holds the GPU Types, resources, Layouts, expressions, and
stage bodies used by the Shader.

Tools can inspect either a top-level Render source or the Render layer inside a
Shader. Both use the same installed Render language, so generic Types and other
shared identities remain consistent. There is no copied Shader model or second
Render language hidden inside Shader.

Render does not know about Shader or Vulkan. Shader builds on Render, and a
graphics backend later turns the completed facts into target-specific bindings
and resources.

Runtime graphics submission is a separate consumer of completed render facts.
It does not redefine Render grammar or Shader identity.

## Persistence

Render can be stored in a Package Archive and rebuilt without its source file.
A Complete payload keeps the public and private declarations, Attributes,
Layouts, expressions, and stage bodies needed to compile it again. An Interface
payload keeps the public contract and Layouts needed by other code, but leaves
out executable bodies.

When Render belongs to a Shader, it uses the same Complete or Interface profile
as its parent. Render does not store chosen GPU storage classes, target bindings,
SPIR-V words, live backend handles, or source-level debugging data in either
profile.
