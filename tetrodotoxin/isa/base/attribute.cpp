// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/attribute.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Base::Attribute::consume_all(Cursor& cursor) -> Bool {
  while (cursor.matches(Class::Type::Attribute)) {
    if (!consume(cursor)) {
      return False;
    }
  }

  return True;
}

auto Base::Attribute::evaluate_all(
    Cursor& cursor,
    Perimortem::Memory::Managed::Vector<Ttx::Attribute>& attributes) -> Bool {
  while (cursor.matches(Class::Type::Attribute)) {
    if (!evaluate(cursor, attributes)) {
      return False;
    }
  }

  return True;
}

auto Base::Attribute::append_all(
    View::Vector<Ttx::Attribute> source,
    Perimortem::Memory::Managed::Vector<Ttx::Attribute>& target) -> void {
  for (Count i = 0; i < source.get_size(); i++) {
    target.insert(source[i]);
  }
}

auto Base::Attribute::key(View::Bytes source) -> View::Bytes {
  return !source.is_empty() && source[0] == '@' ? source.slice(1) : source;
}

auto Base::Attribute::consume_arguments(Cursor& cursor, View::Bytes& value)
    -> Bool {
  if (!cursor.matches(Class::Type::PackingStart)) {
    return True;
  }

  Count depth = 0;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    const Class::Type type = cursor.current().get_class().get_type();
    if (cursor.matches(Class::Type::PackingStart)) {
      depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingEnd)) {
      cursor.consume();
      depth--;
      if (depth == 0) {
        return True;
      }

      continue;
    }

    switch (type) {
    case Class::Type::Addressable:
    case Class::Type::Type:
    case Class::Type::String:
    case Class::Type::Numeric:
    case Class::Type::Float:
      if (depth == 1 && value.is_empty()) {
        value = cursor.current().get_text();
      }

      break;

    default:
      break;
    }

    cursor.consume();
  }

  cursor.token_error("Expected `)` after attribute."_view);
  return False;
}

auto Base::Attribute::consume(Cursor& cursor) -> Bool {
  if (!cursor.require(Class::Type::Attribute, "Expected attribute."_view)) {
    return False;
  }

  View::Bytes value;
  return consume_arguments(cursor, value);
}

auto Base::Attribute::evaluate(
    Cursor& cursor,
    Perimortem::Memory::Managed::Vector<Ttx::Attribute>& attributes) -> Bool {
  const Token* token =
      cursor.require(Class::Type::Attribute, "Expected attribute."_view);
  if (token == nullptr) {
    return False;
  }

  View::Bytes value;
  if (!consume_arguments(cursor, value)) {
    return False;
  }

  attributes.insert(Ttx::Attribute(key(token->get_text()), value));
  return True;
}
