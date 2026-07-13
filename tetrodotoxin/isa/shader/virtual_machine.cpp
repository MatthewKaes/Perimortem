// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/shader/contract.hpp"
#include "tetrodotoxin/isa/shader/function.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::VirtualMachine::evaluate(Cursor& cursor, Base::Context& context)
    -> Ttx::Type* {
  Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
  if (!Base::Attribute::consume_all(cursor)) {
    return nullptr;
  }

  if (!cursor.matches(Class::Type::Addressable) ||
      cursor.current().get_text() != "shader"_view) {
    cursor.token_error("Expected `shader` declaration."_view);
    return nullptr;
  }

  cursor.consume();

  const Token* name =
      cursor.require(Class::Type::Type, "Expected shader type name."_view);
  if (name == nullptr) {
    return nullptr;
  }

  const Ttx::Type* contract = Shader::Contract::resolve(cursor, context);
  if (contract == nullptr) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after shader declaration."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Function> functions(context.get_arena());
  Managed::Vector<const Shader::Block*> function_blocks(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation function_documentation =
        Base::Documentation::evaluate(cursor);
    if (!Base::Attribute::consume_all(cursor)) {
      return nullptr;
    }

    const Shader::Block* block = nullptr;
    Ttx::Function function = Shader::Function::evaluate(
        cursor, context, function_documentation, block);
    if (function.is_empty()) {
      return nullptr;
    }

    for (Count i = 0; i < functions.get_size(); i++) {
      if (functions[i].get_name() == function.get_name()) {
        cursor.token_error("Shader stage name is already defined."_view);
        valid = False;
      }
    }

    if (!Shader::Contract::validate_stage(
            cursor, *contract, function, *block)) {
      valid = False;
    }

    if (valid) {
      functions.insert(function);
      function_blocks.insert(block);
    }
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after shader declaration."_view)) {
    return nullptr;
  }

  if (!valid || !cursor.matches(Class::Type::EndOfStream)) {
    return nullptr;
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    if (!context.define_implementation(functions[i], *function_blocks[i])) {
      return nullptr;
    }
  }

  auto& contract_alias = context.get_arena().construct<Ttx::Type>(
      Ttx::Type::alias("Contract"_view, *contract));
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  types.insert(&contract_alias);
  return &context.get_arena().construct<Ttx::Type>(
      name->get_text(), View::Vector<Ttx::Member>(), types.get_view(),
      functions.get_view(), documentation);
}
