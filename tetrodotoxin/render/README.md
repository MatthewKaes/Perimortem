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
