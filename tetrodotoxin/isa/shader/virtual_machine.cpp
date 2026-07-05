// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/layout/evaluator.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto evaluate_body(
    Cursor& cursor,
    Managed::Vector<Ttx::Type::Function::Block>& blocks) -> Bool {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after shader function signature."_view)) {
    return False;
  }

  Count block_start = cursor.get_token_index();
  Count depth = 1;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeStart)) {
      depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::ScopeEnd)) {
      if (depth == 1) {
        Count block_end = cursor.get_token_index();
        blocks.insert(Ttx::Type::Function::Block(
            cursor.get_token_span(block_start, block_end)));
        cursor.consume();
        return True;
      }

      depth--;
      cursor.consume();
      continue;
    }

    cursor.consume();
  }

  cursor.token_error("Expected `}` after shader function body."_view);
  return False;
}

static auto evaluate_function(
    Context& context,
    Cursor& cursor,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  if (!cursor.require(
          Class::Type::Func, "Expected `func` in shader stage."_view)) {
    return Ttx::Type::Function();
  }

  const Token* name =
      cursor.require(Class::Type::Addressable, "Expected shader stage name."_view);
  if (name == nullptr) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> parameters(context.get_arena());
  if (!Layout::Evaluator::evaluate_bracketed(context, cursor, parameters)) {
    return Ttx::Type::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp,
          "Expected `->` before shader stage result."_view)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> result(context.get_arena());
  if (!Layout::Evaluator::evaluate(context, cursor, result)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Function::Block> blocks(context.get_arena());
  if (!evaluate_body(cursor, blocks)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      name->get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
}

static auto members_match(
    View::Vector<Ttx::Type::Member> expected,
    View::Vector<Ttx::Type::Member> actual) -> Bool {
  if (expected.get_size() != actual.get_size()) {
    return False;
  }

  for (Count i = 0; i < expected.get_size(); i++) {
    if (expected[i].get_name() != actual[i].get_name() ||
        !expected[i].equivalent_to(actual[i])) {
      return False;
    }
  }

  return True;
}

static auto validate_stage(
    Cursor& cursor,
    const Ttx::Type& contract,
    const Ttx::Type::Function& function) -> Bool {
  const Ttx::Type::Function* stage =
      contract.find_function(function.get_name());
  if (stage == nullptr) {
    cursor.token_error("Shader stage is not declared by the render contract."_view);
    return False;
  }

  if (!members_match(stage->get_parameters(), function.get_parameters())) {
    cursor.token_error(
        "Shader stage parameters do not match the render contract."_view);
    return False;
  }

  if (!members_match(stage->get_result(), function.get_result())) {
    cursor.token_error(
        "Shader stage result does not match the render contract."_view);
    return False;
  }

  return True;
}

auto Shader::VirtualMachine::evaluate(Context& context, Cursor& cursor)
    -> Ttx::Type* {
  Ttx::Documentation documentation = Documentation::evaluate(cursor);
  if (!Attribute::consume_all(cursor)) {
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

  if (!cursor.require(
          Class::Type::Define,
          "Expected `:` before shader render contract."_view)) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* contract = context.resolve_type(cursor);
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }
  if (contract == nullptr) {
    cursor.token_error("Shader render contract could not be resolved."_view);
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after shader declaration."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Function> functions(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation function_documentation = Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }

    Ttx::Type::Function function =
        evaluate_function(context, cursor, function_documentation);
    if (function.is_empty()) {
      return nullptr;
    }

    for (Count i = 0; i < functions.get_size(); i++) {
      if (functions[i].get_name() == function.get_name()) {
        cursor.token_error("Shader stage name is already defined."_view);
        valid = False;
      }
    }

    if (!validate_stage(cursor, *contract, function)) {
      valid = False;
    }

    if (valid) {
      functions.insert(function);
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

  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({"isa"_view, "Shader"_view});
  attributes.insert({"contract"_view, contract->get_name()});
  return &context.get_arena().construct<Ttx::Type>(
      name->get_text(), View::Vector<Ttx::Type::Member>(),
      View::Vector<const Ttx::Type*>(), functions.get_view(), documentation,
      attributes.get_view());
}
