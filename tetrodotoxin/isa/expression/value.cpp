// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/expression/value.hpp"

#include "perimortem/serialization/escaped_text.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Expression::Value::evaluate(Cursor& cursor, Context& context) -> Value {
  if (cursor.matches(Class::Type::String)) {
    const Token& token = cursor.consume();
    View::Bytes text = token.get_text();
    if (text.get_size() < 2 || text[0] != '"' ||
        text[text.get_size() - 1] != '"') {
      cursor.range_error(token, token, "Malformed string literal."_view);
      return Value();
    }

    return Value::string(EscapedText::decode(
        context.get_arena(), text.slice(1, text.get_size() - 2)));
  }

  if (cursor.matches(Class::Type::Addressable)) {
    return Value::reference(cursor.consume().get_text());
  }

  cursor.token_error("Unsupported expression value."_view);
  return Value();
}
