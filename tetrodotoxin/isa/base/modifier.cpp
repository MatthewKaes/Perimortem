// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/modifier.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Base::Modifier::evaluate(
    Cursor& cursor,
    View::Vector<Code::Type> allowed,
    View::Bytes error_message) -> Code::Type {
  Code type = cursor.current().get_code();
  if (!type.is_one_of(allowed)) {
    cursor.token_error(error_message);
    return Code::Type::Unknown;
  }

  cursor.consume();
  return type.get_type();
}
