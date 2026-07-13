// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/compiler/backend.hpp"
#include "tetrodotoxin/isa/registry.hpp"

namespace Tetrodotoxin::Puffer {

// Defines the standard body ISAs and host backend composed by Puffer. The
// policy retains no registry or compiler state: callers own the capabilities
// needed by their transaction and may use either half independently. This is
// important for editor operations, which need the language registry without
// constructing a native compiler backend.
class Toolchain {
 public:
  static auto standard_registry() -> Tetrodotoxin::Isa::Registry;
  static auto standard_backend() -> Tetrodotoxin::Compiler::Backend;
};

}  // namespace Tetrodotoxin::Puffer
