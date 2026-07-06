// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

namespace Tetrodotoxin::Compiler {

// Context is the request-local state shared by compiler lowerers.
//
// It is intentionally not a semantic context. The source facts already live in
// Ttx::Type and the ISA contexts. Compiler Context only provides stable
// temporary storage for generated names and collects lowering diagnostics so
// the caller can decide how to report them.
class Context {
 public:
  explicit Context(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena) {}

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  constexpr auto get_errors() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return errors;
  }

  constexpr auto has_errors() const -> Bool {
    return errors.get_size() != 0;
  }

  auto report(Perimortem::Core::View::Bytes message) -> Bool {
    errors.insert(message);
    return False;
  }

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Dynamic::Vector<Perimortem::Core::View::Bytes> errors;
};

}  // namespace Tetrodotoxin::Compiler
