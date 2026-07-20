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
  Bool attributes_consumed = Base::Attribute::consume_all(cursor);
  if (!attributes_consumed) {
    return nullptr;
  }

  if (!cursor.matches(Code::Type::Addressable) ||
      cursor.current().get_text() != "shader"_view) {
    cursor.token_error("Expected `shader` declaration."_view);
    return nullptr;
  }

  cursor.consume();

  const Token* name =
      cursor.require(Code::Type::Type, "Expected shader type name."_view);
  if (name == nullptr) {
    return nullptr;
  }

  const Ttx::Type* contract = Shader::Contract::resolve(cursor, context);
  if (contract == nullptr) {
    return nullptr;
  }

  Bool has_scope = cursor.require(
      Code::Type::ScopeStart, "Expected `{` after shader declaration."_view);
  if (!has_scope) {
    return nullptr;
  }

  Managed::Vector<Ttx::Function> functions(context.get_arena());
  Managed::Vector<const Shader::Block*> function_blocks(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::ScopeEnd)) {
    Ttx::Documentation function_documentation =
        Base::Documentation::evaluate(cursor);
    Bool function_attributes_consumed = Base::Attribute::consume_all(cursor);
    if (!function_attributes_consumed) {
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

    Bool stage_valid =
        Shader::Contract::validate_stage(cursor, *contract, function, *block);
    if (!stage_valid) {
      valid = False;
    }

    if (valid) {
      functions.insert(function);
      function_blocks.insert(block);
    }
  }

  Bool has_scope_end = cursor.require(
      Code::Type::ScopeEnd, "Expected `}` after shader declaration."_view);
  if (!has_scope_end) {
    return nullptr;
  }

  if (!valid || !cursor.matches(Code::Type::Terminal)) {
    return nullptr;
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    Bool implementation_defined =
        context.define_implementation(functions[i], *function_blocks[i]);
    if (!implementation_defined) {
      return nullptr;
    }
  }

  auto& contract_alias = context.get_arena().construct<Ttx::Type>(
      Ttx::Type::alias("Contract"_view, *contract));
  Managed::Vector<Ttx::Type::Reference> types(context.get_arena());
  types.insert(Ttx::Type::Reference(contract_alias));
  return &context.get_arena().construct<Ttx::Type>(
      name->get_text(), View::Vector<Ttx::Member>(), types.get_view(),
      functions.get_view(), View::Vector<Ttx::Function>(), documentation);
}
