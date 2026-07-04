// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::VirtualMachine::evaluate(Context& context, Cursor& cursor)
    -> Ttx::Type* {
  View::Bytes exported_name;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (exported_name.is_empty() &&
        cursor.matches(Class::Type::Type)) {
      exported_name = cursor.current().get_text();
    }

    cursor.consume();
  }

  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  if (!exported_name.is_empty()) {
    auto& exported = context.get_arena().construct<Ttx::Type>(exported_name);
    types.insert(&exported);
  }

  auto& type = context.get_arena().construct<Ttx::Type>(
      Shader::VirtualMachine::get_name(), View::Vector<Ttx::Type::Member>(),
      types.get_view(), View::Vector<Ttx::Type::Function>());
  return &type;
}
