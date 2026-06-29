// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/literal.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Expression::Literal::inner_text(View::Bytes text) -> View::Bytes {
  if (text.get_size() >= 2 && text[0] == '"' &&
      text[text.get_size() - 1] == '"') {
    return text.slice(1, text.get_size() - 2);
  }

  return text;
}
