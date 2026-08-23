# LLVM backend

The LLVM backend carries completed Library code from semantic meaning to a CPU
product. It follows the real Types, Callables, values, and control flow already
linked in the Workspace, then chooses the LLVM representation that preserves
their behavior. One request can produce readable LLVM IR, an object module,
debug information, and a matching C interface.

This is the point where raising hands the program to lowering. Library can stay
focused on what the program means because LLVM owns the physical choices that
come next. The semantic graph remains useful to editors and other backends, and
the generated LLVM objects leave with the completed product.

LLVM IR is Terminal relative to the Workspace once it can be consumed without
the live semantic graph. It can still continue through LLVM optimization and
machine lowering, so the Terminal boundary does not claim that every external
pipeline has finished.

## Follow one compilation

A request arrives with a completed Library Monograph, its source facts, the
chosen CPU target, and a home for diagnostics and products. From there the
backend:

* selects physical carriers for the exact Library Types
* prepares native Callables and Static storage
* walks declarations and executable bodies in semantic order
* verifies the finished LLVM module
* publishes reviewable IR, object bytes, and any requested C interface

Along the way, physical maps use original semantic identities as temporary
keys. A debug Type, native Function, or storage address can always be traced
back to the meaning that requested it without creating a second Type, Callable,
or control flow model. Those maps and native handles finish with the request.

## Find your way through the source

The source tree follows the journey above. The root namespace welcomes a
compilation request and returns its products. Four internal namespaces make the
middle of that journey easier to follow:

* `Lowering` walks completed Library meaning
* `Representation` owns the LLVM module and its physical facts
* `Emission` creates instructions for individual operations
* `Abi` owns external symbols, package bindings, and C declarations

Their folders use the same names in lowercase, so an include path and a
qualified owner point to the same architectural place.

## Meaning and representation

Library decides that `U32` is an unsigned integer with 32 bit arithmetic
semantics. The backend decides how the selected target represents that Type and
must preserve its observable behavior. The same division applies throughout
the system:

* Library owns Type identity, value flow, fitting, mutation, and control
* LLVM owns ABI carriers, alignment, registers, addresses, instructions, and
  debug encoding
* Linker owns the final symbol set and native program products

The configured target currently covers 64 bit x86 Linux with the ELF System V
ABI. Target selection is explicit rather than inferred from the build host.

## Native interfaces

An authored `@abi("C")` attribute requests a C boundary. The backend applies
the selected ABI, publishes stable symbols, and emits declarations that match
the same carrier decisions used by the object module. Calls without that
attribute remain ordinary Tetrodotoxin calls and may use a private convention.

Object values preserve Library's nonnull shared identity and reference counted
lifetime. Option, Result, View, Access, Fixed, Structure, and scalar values keep
their Library behavior while receiving target specific storage and calling
conventions here.

## The parallel GPU path

`Backend::Spirv` follows the same architectural boundary. Shader owns a real
Render child and a real Library child, so a SPIR-V producer can walk Shader
stages, Render contracts, and the hosted Library execution graph together. It
does not need a translated Library IR or a second body language.

The two backends may choose very different physical representations. They meet
only at the completed semantic graph, which is exactly why the common layer is
meaning rather than representation.
