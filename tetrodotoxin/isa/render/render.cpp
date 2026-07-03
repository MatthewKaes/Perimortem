// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/render/render.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Render::evaluate(Context& context, Cursor& cursor) -> Bool {
  while (!cursor.matches(Class::Type::EndOfStream)) {
    cursor.consume();
  }

  auto& type = context.get_arena().construct<Ttx::Type>(
      context.get_source_name());
  context.publish(type);
  return True;
}
