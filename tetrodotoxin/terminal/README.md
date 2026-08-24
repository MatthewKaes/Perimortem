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
execution into LLVM modules and CPU objects. A future SPIR-V Terminal can walk
Shader, Render, and their hosted Library graph in parallel. Both start from the
same completed meaning while producing deliberately different outputs.

```text
Dialect and Workspace meaning
              |
              v
      Terminal::Abi
       /          \
      v            v
 C and C++       Terminal::Llvm
 interfaces       CPU objects
```

Archive writers, formatters, and Linker remain with their natural owners. They
are Terminal producers too, but moving every output beneath one directory would
hide the domain that defines its product.
