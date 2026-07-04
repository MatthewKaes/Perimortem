// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/definition.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/isa/modifier.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto token_class_message(
    Cursor& cursor,
    View::Bytes prefix,
    View::Vector<Class::Type> allowed) -> View::Bytes {
  Managed::Bytes message(cursor.get_arena());
  message.concat(prefix);
  message.concat(" {"_view);
  for (Count i = 0; i < allowed.get_size(); i++) {
    if (i != 0) {
      message.concat(", "_view);
    }

    View::Bytes source_text = Class::get_source_text(allowed[i]);
    message.concat(
        source_text.is_empty() ? Class(allowed[i]).get_name() : source_text);
  }
  message.concat("}"_view);
  return message.get_view();
}

auto Definition::evaluate(
    Cursor& cursor,
    Ttx::Documentation documentation,
    View::Vector<Class::Type> allowed_modifiers,
    View::Vector<Class::Type> allowed_names,
    View::Vector<Class::Type> allowed_qualifiers) -> Definition {
  Modifier modifier = Modifier::evaluate(
      cursor, allowed_modifiers,
      token_class_message(
          cursor,
          "Expected a definition to start with one of the following modifiers"_view,
          allowed_modifiers));
  if (!modifier.is_valid()) {
    return Definition();
  }

  return evaluate_after_modifier(
      cursor, documentation, modifier.get_type(), allowed_names,
      allowed_qualifiers);
}

auto Definition::evaluate_after_modifier(
    Cursor& cursor,
    Ttx::Documentation documentation,
    Class::Type modifier,
    View::Vector<Class::Type> allowed_names,
    View::Vector<Class::Type> allowed_qualifiers) -> Definition {
  const Token& name = cursor.current();
  if (!name.get_class().is_one_of(allowed_names)) {
    cursor.token_error(token_class_message(
        cursor,
        "Definitions can only be created here for the following types"_view,
        allowed_names));
    return Definition();
  }
  cursor.consume();

  if (!cursor.require(
          Class::Type::Define, "Expected `:` after definition name."_view)) {
    return Definition();
  }

  const Token& kind = cursor.current();
  if (!kind.get_class().is_one_of(allowed_qualifiers)) {
    cursor.token_error(token_class_message(
        cursor,
        "Definition qualifier can only be one of the following types"_view,
        allowed_qualifiers));
    return Definition();
  }

  cursor.consume();
  return Definition(
      documentation, modifier, name.get_class().get_type(),
      name.get_text(), kind.get_class().get_type(), kind.get_text());
}
