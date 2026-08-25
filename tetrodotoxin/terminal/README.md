# Terminal producers

Once a Tetrodotoxin Workspace understands the complete program, Terminals turn
that meaning into products which can leave the live graph. Each producer owns
one destination and can therefore stay focused on the facts its next consumer
actually needs.

The native path begins with the [ABI Terminal](abi/README.md). It projects the
completed graph into the common C representation used for symbols, carriers,
calling boundaries, and cross language headers. This gives every native
producer one agreement without making a particular instruction engine the
source of representation truth.

The [LLVM Terminal](llvm/README.md) consumes that ABI and lowers Library
execution into LLVM modules and CPU objects. The
[SPIR-V Terminal](spirv/README.md) begins from the Library execution graph hosted
by Shader, then follows its exact Render contract and Bridge relationships.
Graphics compiles hosted Scene Fields into compact access behavior, and Vulkan
turns the Shader product into the pipeline description consumed beside each
stable frame submission. Application composition connects those independent
products to the App policy and selected runtime.

An application target also pairs each hosted Type route with one native
Descriptor provider. That pair belongs to build configuration because it
chooses a runtime implementation for the current host. Graphics still proves
the Type against the shared Host Interface, and the generated entry carries the
selected provider beside that proof. The runtime can therefore realize Sprite
today and another hosted Type later without learning either concrete class.

These producers share semantic identities only while the Workspace is alive.
The CPU object, embedded SPIR-V module, Scene access function, Vulkan pipeline
description, and generated process entry are sibling outputs after that handoff.

```text
Dialect and Workspace meaning
       /                 \
      v                   v
Terminal::Abi       Terminal::Spirv
      |               GPU modules
      v                    |
Terminal::Llvm             v
  CPU objects      Terminal::Vulkan
       \                 /
        v               v
       Terminal::Application
          native entry
```

Archive writers, formatters, and Linker remain with their natural owners. They
are Terminal producers too, but moving every output beneath one directory would
hide the domain that defines its product.
