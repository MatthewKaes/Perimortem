// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/lifecycle.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/layout/evaluator.hpp"

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
    Base::Context& context,
    Ttx::Documentation documentation,
    const Ttx::Type* owner) -> Ttx::Function {
  const Token& name = cursor.current();
  if (!is_root(name.get_text())) {
    cursor.token_error(
        "Scene root addressable must be on_start, on_update, or on_exit."_view);
    return Ttx::Function();
  }

  cursor.consume();

  Managed::Vector<Ttx::Member> parameters(context.get_arena());
  Bool parameters_evaluated = Base::Layout::Evaluator::evaluate_bracketed(
      cursor, context, parameters, owner);
  if (!parameters_evaluated) {
    return Ttx::Function();
  }

  if (parameters.is_empty() || parameters[0].get_name() != "self"_view ||
      !parameters[0].references(owner)) {
    cursor.token_error("Scene lifecycle layout must begin with `self`."_view);
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> result(context.get_arena());
  Bool body_consumed = Base::Expression::Evaluator::consume_block(
      cursor, "Expected `{` after Scene lifecycle declaration."_view,
      "Expected `}` after Scene lifecycle body."_view);
  if (!body_consumed) {
    return Ttx::Function();
  }

  return Ttx::Function(
      name.get_text(), Ttx::Layout(parameters.get_view()),
      Ttx::Layout(result.get_view()), documentation);
}
