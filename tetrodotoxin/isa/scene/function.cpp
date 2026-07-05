// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/function.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/layout/evaluator.hpp"
#include "tetrodotoxin/isa/scene/lifecycle.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Scene::Function::evaluate(
    Context& context,
    Cursor& cursor,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  cursor.consume();
  if (!cursor.require(
          Class::Type::Func, "Expected `func` after Scene function modifier."_view)) {
    return Ttx::Type::Function();
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected Scene function name."_view);
  if (name == nullptr) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> parameters(context.get_arena());
  if (!Layout::Evaluator::evaluate_bracketed(context, cursor, parameters)) {
    return Ttx::Type::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp,
          "Expected `->` before Scene function result."_view)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> result(context.get_arena());
  if (!Layout::Evaluator::evaluate(context, cursor, result)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Function::Block> blocks(context.get_arena());
  if (!Expression::consume_block(
          cursor, "Expected `{` after Scene function signature."_view,
          "Expected `}` after Scene function body."_view, blocks)) {
    return Ttx::Type::Function();
  }

  Ttx::Type::Function function(
      name->get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
  if (Lifecycle::is_root(function.get_name())) {
    cursor.token_error("Scene lifecycle roots cannot be declared with `func`."_view);
    return Ttx::Type::Function();
  }

  return function;
}

auto Scene::Function::insert(
    Cursor& cursor,
    Managed::Vector<Ttx::Type::Function>& functions,
    Ttx::Type::Function function) -> Bool {
  if (function.is_empty()) {
    return False;
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i].get_name() == function.get_name()) {
      cursor.token_error("Scene function name is already defined."_view);
      return False;
    }
  }

  functions.insert(function);
  return True;
}
