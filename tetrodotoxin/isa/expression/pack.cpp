// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/expression/pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Expression::Pack::evaluate(Cursor& cursor, Context& context)
    -> const Pack* {
  if (!cursor.require(
          Class::Type::PackingStart,
          "Expected `(` before expression pack."_view)) {
    return nullptr;
  }

  Managed::Vector<Value> values(context.get_arena());
  while (!cursor.matches(Class::Type::PackingEnd)) {
    Value value = Value::evaluate(cursor, context);
    if (value.is_empty()) {
      return nullptr;
    }
    values.insert(value);

    if (!cursor.matches(Class::Type::PackingOp)) {
      break;
    }
    cursor.consume();
  }

  if (!cursor.require(
          Class::Type::PackingEnd,
          "Expected `)` after expression pack."_view)) {
    return nullptr;
  }

  return &context.get_arena().construct<Pack>(values.get_view());
}
