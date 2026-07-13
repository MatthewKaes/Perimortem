// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/function.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/layout/evaluator.hpp"
#include "tetrodotoxin/isa/shader/block.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Function::evaluate(
    Cursor& cursor,
    Base::Context& context,
    Ttx::Documentation documentation,
    const Block*& block) -> Ttx::Function {
  if (!cursor.require(
          Class::Type::Func, "Expected `func` in shader stage."_view)) {
    return Ttx::Function();
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected shader stage name."_view);
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
          "Expected `->` before shader stage result."_view)) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> result(context.get_arena());
  if (!Base::Layout::Evaluator::evaluate(cursor, context, result)) {
    return Ttx::Function();
  }

  block = Shader::Block::evaluate(cursor, context);
  if (block == nullptr) {
    return Ttx::Function();
  }

  return Ttx::Function(
      name->get_text(), Ttx::Layout(parameters.get_view()),
      Ttx::Layout(result.get_view()), documentation);
}
