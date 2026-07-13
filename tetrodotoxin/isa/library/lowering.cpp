// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/compiler/linkage.hpp"
#include "tetrodotoxin/isa/library/virtual_machine.hpp"
#include "tetrodotoxin/isa/lowering/context.hpp"
#include "tetrodotoxin/isa/lowering/input.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;

auto Isa::Library::VirtualMachine::lower(
    Isa::Lowering::Context& context,
    const Isa::Lowering::Input& input) -> Bool {
  const Ttx::Type& root = input.type;
  const Base::Implementation& implementation = input.implementation;
  View::Vector<Ttx::Function> functions = root.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    const auto* body = implementation.find<Execution::Body>(functions[i]);
    if (body == nullptr) {
      continue;
    }

    Linkage linkage = Linkage::internal(
        context.get_arena(), root, functions[i], input.module);
    if (!linkage.is_valid() ||
        !context.get_program().define(
            input.source, linkage.get_symbol(), functions[i], *body)) {
      context.get_errors().insert(
          input.source, "Library function could not be published."_view);
      return False;
    }
  }

  return True;
}
