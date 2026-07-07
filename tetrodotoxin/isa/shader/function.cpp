// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/function.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/layout/evaluator.hpp"
#include "tetrodotoxin/isa/shader/block.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Function::evaluate(
    Cursor& cursor,
    Context& context,
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
  if (!Layout::Evaluator::evaluate_bracketed(cursor, context, parameters)) {
    return Ttx::Type::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp,
          "Expected `->` before shader stage result."_view)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> result(context.get_arena());
  if (!Layout::Evaluator::evaluate(cursor, context, result)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Function::Block> blocks(context.get_arena());
  if (!Shader::Block::evaluate(cursor, context, blocks)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      name->get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
}
