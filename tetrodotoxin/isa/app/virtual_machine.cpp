// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/app/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/layout/evaluator.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto evaluate_function(
    Context& context,
    Cursor& cursor,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  if (!cursor.require(Class::Type::Func, "Expected `func` in App."_view)) {
    return Ttx::Type::Function();
  }

  const Token* name =
      cursor.require(Class::Type::Addressable, "Expected App function name."_view);
  if (name == nullptr) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> parameters(context.get_arena());
  if (!Layout::Evaluator::evaluate_bracketed(context, cursor, parameters)) {
    return Ttx::Type::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp, "Expected `->` before App function result."_view)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> result(context.get_arena());
  if (!Layout::Evaluator::evaluate(context, cursor, result)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Function::Block> blocks(context.get_arena());
  if (!Expression::consume_block(
          cursor, "Expected `{` after App function signature."_view,
          "Expected `}` after App function body."_view, blocks)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      name->get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
}

auto App::VirtualMachine::evaluate(Context& context, Cursor& cursor)
    -> Ttx::Type* {
  Managed::Vector<Ttx::Type::Function> functions(context.get_arena());
  Bool valid = True;

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }
    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    if (!cursor.require(
            Class::Type::Public, "Expected public App `main` function."_view)) {
      return nullptr;
    }

    Ttx::Type::Function function =
        evaluate_function(context, cursor, documentation);
    if (function.is_empty()) {
      return nullptr;
    }

    if (function.get_name() != "main"_view) {
      cursor.token_error("App root can only define `main`."_view);
      valid = False;
    }

    if (function.get_parameters().get_size() != 0 ||
        function.get_result().get_size() != 0) {
      cursor.token_error("App `main` must use `main[] -> []`."_view);
      valid = False;
    }

    for (Count i = 0; i < functions.get_size(); i++) {
      if (functions[i].get_name() == function.get_name()) {
        cursor.token_error("App function name is already defined."_view);
        valid = False;
      }
    }

    if (valid) {
      functions.insert(function);
    }
  }

  if (!valid || functions.get_size() != 1) {
    if (functions.get_size() == 0 && cursor.get_errors().is_empty()) {
      cursor.error("App source must define public `main[] -> []`."_view);
    }
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({"isa"_view, App::VirtualMachine::get_name()});
  return &context.get_arena().construct<Ttx::Type>(
      App::VirtualMachine::get_name(), View::Vector<Ttx::Type::Member>(),
      View::Vector<const Ttx::Type*>(), functions.get_view(),
      Ttx::Documentation(), attributes.get_view());
}
