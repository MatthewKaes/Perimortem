// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/modifier.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Base::Modifier::evaluate(
    Cursor& cursor,
    View::Vector<Class::Type> allowed,
    View::Bytes error_message) -> Class::Type {
  Class type = cursor.current().get_class();
  if (!type.is_one_of(allowed)) {
    cursor.token_error(error_message);
    return Class::Type::Unknown;
  }

  cursor.consume();
  return type.get_type();
}
