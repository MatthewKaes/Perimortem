// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/lifecycle.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/layout/evaluator.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

constexpr Static::Vector<View::Bytes, 3> lifecycle_roots = {{
  "on_start"_view,
  "on_update"_view,
  "on_exit"_view,
}};

auto Scene::Lifecycle::is_root(View::Bytes name) -> Bool {
  for (Count i = 0; i < lifecycle_roots.get_size(); i++) {
    if (lifecycle_roots[i] == name) {
      return True;
    }
  }

  return False;
}

auto Scene::Lifecycle::evaluate(
    Cursor& cursor,
    Context& context,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  const Token& name = cursor.current();
  if (!is_root(name.get_text())) {
    cursor.token_error(
        "Scene root addressable must be on_start, on_update, or on_exit."_view);
    return Ttx::Type::Function();
  }
  cursor.consume();

  Managed::Vector<Ttx::Type::Member> parameters(context.get_arena());
  if (cursor.matches(Class::Type::IndexStart) &&
      !Layout::Evaluator::evaluate_bracketed(cursor, context, parameters)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> result(context.get_arena());
  Managed::Vector<Ttx::Type::Function::Block> blocks(context.get_arena());
  if (!Expression::consume_block(
          cursor, "Expected `{` after Scene lifecycle declaration."_view,
          "Expected `}` after Scene lifecycle body."_view, blocks)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      name.get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
}
