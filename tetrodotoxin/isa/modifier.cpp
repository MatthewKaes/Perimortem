// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/modifier.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Modifier::evaluate(
    Cursor& cursor,
    View::Vector<Class::Type> allowed,
    View::Bytes error_message) -> Modifier {
  Class type = cursor.current().get_class();
  if (!type.is_one_of(allowed)) {
    cursor.token_error(error_message);
    return Modifier();
  }

  cursor.consume();
  return Modifier(type.get_type());
}
