// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/source/documentation.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Ttx::Dialect;

auto Source::Documentation::parse(Cursor& cursor) -> Ttx::Documentation {
  // A bit annoying that we have to double comments up in the lexical arena but
  // the TTX model is technically free of lexical analysis so we need to strip
  // away the token data into a proper buffer.
  Managed::Vector<View::Bytes> lines(cursor.get_arena());
  while (cursor.matches(Ttx::Lexical::Class::Type::Comment)) {
    lines.insert(cursor.consume().get_text());
  }

  return Ttx::Documentation(lines.get_view());
}
