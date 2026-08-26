# Render

CPU code, GPU code, and a renderer all need to agree about the data that crosses
their boundary. Render gives that agreement an authored language. A Render
source names the values, resources, and Stages that make up a format, and a
Shader supplies an implementation of that contract.

Tetrodotoxin checks the agreement before choosing a GPU representation. CPU
code, Shader code, and different graphics backends can therefore share one
meaning without treating reflection data from a particular backend as the
source of truth.

Canonical grammar reference: [Render.g4](grammar/Render.g4).

Render uses the same Definition envelope as Library and Shader. Documentation,
Attributes, visibility, modifiers, name, and `:` are interpreted once. The
qualifier that follows selects `struct`, `stage`, `resource`, `push`, or another
Render declaration, leaving each concrete parser responsible only for the
meaning it adds.

```ttx
// Rendering interface.
dialect : Render;
```

## One meaning, three responsibilities

Render stays useful across editors, Archives, and GPU targets because those
jobs meet at the language rather than blending together.

The Interpreter reads authored Render syntax and creates the corresponding
semantic objects. The Language owns the lasting interface: contract Structures,
required values, resources, Stages, Layouts, and Attributes that another
language or tool can query. It owns no Shader expressions or execution model.
Archive support preserves enough of that meaning to rebuild an
equivalent Render graph without turning the stored bytes into the graph itself.

That separation leaves target work with Terminals. The Vulkan Terminal can
choose SPIR V storage classes and decorations from a completed Render graph,
while another GPU target can make different choices from the same contract.

## Render contracts

A Render contract can declare:

* ordinary values
* constant and push data
* resources and their binding Attributes
* required Shader Stages
* parameter and result Layouts for each Stage
* built in values, locations, sets, slots, and access capabilities
* portable topology, blend, geometry, vertex count, and host input roles

Each declaration stays independently inspectable. For example, a resource
binding that needs both a set and a slot uses two Attributes instead of hiding
them inside one backend specific record.

Render defines these Attribute keys:

* `@set(number)`, `@slot(number)`, and `@location(number)` carry unsigned
  indices.
* `@builtin("name")`, `@address_space("name")`, and
  `@capability("name")` carry names interpreted by Render.
* `@read` and `@write` state independent resource capabilities.
* `@topology("triangle_list")`, `@blend("alpha")`,
  `@geometry("unit_quad_2d")`, and `@vertex_count(number)` describe fixed
  portable pipeline agreement on the enclosing contract.
* `@host("transform_x")` and `@host("transform_y")` identify the base values
  supplied by graphics placement.

Each Attribute has at most one scalar value. Duplicate keys on one declaration
are invalid. Shader inherits these facts from the selected contract rather than
authoring them again. Other Dialects do not acquire these meanings merely
because they share TTX Attribute storage.

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

Render rejects an Attribute on the wrong kind of declaration, a value of the
wrong kind, a duplicate key, or a combination that cannot be used together.
Shader must satisfy the declared contract. It cannot hide a mismatch by choosing
a convenient target binding later.

Stage entry Attributes precede the named Layout entry:

```ttx
public fragment : stage [
  @location(0) .color : Math::Vec4D,
] -> [
  @location(0) .color : Math::Vec4D,
];
```

## Layout and representation

Stage parameters and results use TTX Layouts. A matching shape is necessary,
but shape alone does not say whether a value is a vector, resource, address, or
part of a particular ABI. Render contracts and Attributes describe those
additional requirements. TTX Interface negotiation combines that meaning with
Layout evidence when a concrete Shader Stage attempts to satisfy the contract.

A contract may keep related data requirements beneath its own semantic name.
The standard two dimensional contract uses `TexturedQuad2D::Inputs`. During the
cross Dialect composition barrier, Shader generates the exact executable Library
projection needed by Stage bodies and retains its relationship to the real
Render declaration. Authors do not repeat the Type or its Fields.

Fields, Types, and Stage Callables use their corresponding access domains:

* named values are selected with `.`
* nested Types are selected with `::`
* Stage Callables are selected and invoked with `->` where the consuming
  language permits invocation.

## Shader relationship

Render owns the interface. [Shader](../shader/README.md) selects one Render
contract, organizes its stages, and supplies the implementation. The Render
Monograph remains an ordinary Workspace identity owned by its source. Shader
retains the exact contract edge while its real Library child owns generated
executable projections, Stage Functions, expressions, and Flow. Composition
runs in installed Dialect dependency order before ordinary linking, so Package
source order does not change inherited binding availability.

Tools inspect the Render contract and Shader implementation through those real
identities. No empty child Monograph or copied interface graph is needed to make
their relationship visible.

Render does not know about Shader or Vulkan. Shader builds on Render, and a
graphics Terminal later turns the completed facts into target specific
bindings and resources.

Runtime graphics submission is a separate consumer of completed render facts.
It does not redefine Render grammar or Shader identity.

## Persistence

Render can be stored in a Package Archive and reconstructed without its source
file. A Complete payload keeps its public and private contracts, while a
Contract payload keeps the public Attributes and Layouts another Package can
negotiate against. The Package envelope pairs those semantics with any compiled
artifact locations. Shader Bridges and bodies remain with Shader. Render does
not store chosen GPU storage classes, target bindings, SPIR-V words, live
backend handles, or source level debugging data in either profile.
