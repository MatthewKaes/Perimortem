// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/lexical/class.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Base {

// Parses the common access and storage word at the front of an ISA declaration.
// The lexical class is returned directly so the parser does not manufacture a
// wrapper object or a second semantic enum. Unknown reports that evaluation
// failed and the cursor already contains the diagnostic.
class Modifier {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Vector<Ttx::Lexical::Class::Type> allowed,
      Perimortem::Core::View::Bytes error_message) -> Ttx::Lexical::Class::Type;
};

}  // namespace Tetrodotoxin::Isa::Base
