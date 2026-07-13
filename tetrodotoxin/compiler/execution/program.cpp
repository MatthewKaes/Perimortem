// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/execution/program.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Compiler;

auto Execution::Program::define(
    Ttx::Lexical::Source source,
    View::Bytes symbol,
    const Ttx::Function& function,
    const Body& body) -> Bool {
  if (symbol.is_empty() || body.get_blocks().is_empty()) {
    return False;
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i].get_symbol() == symbol) {
      return False;
    }
  }

  functions.insert(Function(source, symbol, function, body));
  return True;
}
