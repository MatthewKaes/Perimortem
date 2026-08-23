# Backends

Once a Tetrodotoxin Workspace understands the complete program, backends answer
the practical question: what should that meaning become?

A backend is a Terminal producer for target representation. It can walk the
real Types, Callables, values, and domain relationships it understands, choose a
representation for one target, and publish a finished product. The languages
remain focused on describing the program, so another backend can make different
physical choices without asking them to change.

Backends form one part of the output side of Toolchain composition. Other
Terminal producers can format source, project editor information, serialize an
Archive, or link native objects. All begin with completed meaning, but each owns
the contract of its particular destination.

The [LLVM backend](llvm/README.md) turns Library meaning into CPU objects today.
A SPIR-V backend can follow Shader, Render, and their hosted Library graph into
a GPU module. Both begin at the same completed Workspace, which gives CPU and
GPU production a common semantic starting point without forcing them through a
common target representation.
