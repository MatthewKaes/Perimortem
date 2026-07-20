// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Base::Documentation::evaluate(Cursor& cursor) -> Ttx::Documentation {
  // Documentation is a TTX fact, not a token fact, so strip the comment tokens
  // down to the source lines that should travel with the evaluated object.
  Managed::Vector<View::Bytes> lines(cursor.get_arena());
  while (cursor.matches(Ttx::Lexical::Code::Type::Comment)) {
    lines.insert(cursor.consume().get_text());
  }

  return Ttx::Documentation(lines.get_view());
}
