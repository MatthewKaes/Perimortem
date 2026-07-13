// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/execution/function.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// The immutable function bodies supplied to a toolchain backend.
class Program {
 public:
  auto define(
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes symbol,
      const Ttx::Function& function,
      const Body& body) -> Bool;

  constexpr auto get_functions() const
      -> Perimortem::Core::View::Vector<Function> {
    return functions;
  }

 private:
  Perimortem::Memory::Dynamic::Vector<Function> functions;
};

}  // namespace Tetrodotoxin::Compiler::Execution
