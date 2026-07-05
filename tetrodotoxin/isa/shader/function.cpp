// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/function.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/layout/evaluator.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Function::evaluate(
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
  if (!Expression::consume_block(
          cursor, "Expected `{` after shader function signature."_view,
          "Expected `}` after shader function body."_view, blocks)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      name->get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
}
