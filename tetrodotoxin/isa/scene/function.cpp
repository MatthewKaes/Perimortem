// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/function.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/layout/evaluator.hpp"
#include "tetrodotoxin/isa/scene/lifecycle.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Scene::Function::evaluate(
    Cursor& cursor,
    Base::Context& context,
    Ttx::Documentation documentation) -> Ttx::Function {
  cursor.consume();
  if (!cursor.require(
          Class::Type::Func,
          "Expected `func` after Scene function modifier."_view)) {
    return Ttx::Function();
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected Scene function name."_view);
  if (name == nullptr) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> parameters(context.get_arena());
  if (!Base::Layout::Evaluator::evaluate_bracketed(
          cursor, context, parameters)) {
    return Ttx::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp,
          "Expected `->` before Scene function result."_view)) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> result(context.get_arena());
  if (!Base::Layout::Evaluator::evaluate(cursor, context, result)) {
    return Ttx::Function();
  }

  if (!Base::Expression::Evaluator::consume_block(
          cursor, "Expected `{` after Scene function signature."_view,
          "Expected `}` after Scene function body."_view)) {
    return Ttx::Function();
  }

  Ttx::Function function(
      name->get_text(), Ttx::Layout(parameters.get_view()),
      Ttx::Layout(result.get_view()), documentation);
  if (Lifecycle::is_root(function.get_name())) {
    cursor.token_error(
        "Scene lifecycle roots cannot be declared with `func`."_view);
    return Ttx::Function();
  }

  return function;
}

auto Scene::Function::insert(
    Cursor& cursor,
    Managed::Vector<Ttx::Function>& functions,
    Ttx::Function function) -> Bool {
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
