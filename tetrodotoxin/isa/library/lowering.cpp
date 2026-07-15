// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/abi/linkage.hpp"
#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/isa/library/virtual_machine.hpp"
#include "tetrodotoxin/isa/lowering/context.hpp"
#include "tetrodotoxin/isa/lowering/input.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

auto Isa::Library::VirtualMachine::lower_functions(
    Isa::Lowering::Context& context,
    const Isa::Lowering::Input& input,
    View::Vector<Ttx::Function> functions) -> Bool {
  const Isa::Base::Implementation& implementation = input.implementation;
  for (Count i = 0; i < functions.get_size(); i++) {
    const auto* body =
        implementation.find<Compiler::Execution::Body>(functions[i]);
    if (body == nullptr) {
      continue;
    }

    const Abi::Linkage* linkage = implementation.find_linkage(functions[i]);
    if (linkage == nullptr) {
      context.get_errors().insert(
          input.source, "Library function has no published linkage."_view);
      return False;
    }

    Bool published = context.publish_function(
        input.source, linkage->get_symbol(), functions[i], *body);
    if (!published) {
      context.get_errors().insert(
          input.source, "Library function could not be published."_view);
      return False;
    }
  }

  return True;
}

auto Isa::Library::VirtualMachine::lower_type(
    Isa::Lowering::Context& context,
    const Isa::Lowering::Input& input,
    const Ttx::Type& type) -> Bool {
  Bool lowered_type =
      lower_functions(context, input, type.get_type_functions());
  if (!lowered_type) {
    return False;
  }

  Bool lowered_addressable =
      lower_functions(context, input, type.get_addressable_functions());
  if (!lowered_addressable) {
    return False;
  }

  // A nested type is a real linkage owner, not only a name exported by the
  // package root. Walk the authored type tree so constructors and methods are
  // emitted with the same implementation facts used by root functions.
  View::Vector<Ttx::Type::Reference> nested = type.get_types();
  for (Count i = 0; i < nested.get_size(); i++) {
    Bool lowered_nested = lower_type(context, input, nested[i].get_type());
    if (!lowered_nested) {
      return False;
    }
  }

  return True;
}

auto Isa::Library::VirtualMachine::lower(
    Isa::Lowering::Context& context,
    const Isa::Lowering::Input& input) -> Bool {
  return lower_type(context, input, input.type);
}
