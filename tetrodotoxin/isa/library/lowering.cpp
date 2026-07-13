// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/compiler/linkage.hpp"
#include "tetrodotoxin/isa/library/virtual_machine.hpp"
#include "tetrodotoxin/terminal/context.hpp"
#include "tetrodotoxin/terminal/input.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;

auto Isa::Library::VirtualMachine::lower(
    Terminal::Context& context,
    const Terminal::Input& input) -> Bool {
  const Ttx::Type& root = input.get_type();
  const Base::Implementation& implementation = input.get_implementation();
  View::Vector<Ttx::Function> functions = root.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    const auto* body = implementation.find<Execution::Body>(functions[i]);
    if (body == nullptr) {
      continue;
    }

    Linkage linkage = Linkage::internal(
        context.get_arena(), root, functions[i], input.get_module());
    if (!linkage.is_valid() ||
        !context.get_program().define(
            input.get_source(), linkage.get_symbol(), functions[i], *body)) {
      context.get_errors().insert(
          input.get_source(), "Library function could not be published."_view);
      return False;
    }
  }

  return True;
}
