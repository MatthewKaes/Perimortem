// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/qualified_name.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto QualifiedName::evaluate(Cursor& cursor) -> QualifiedName {
  const Token* first_segment = cursor.require(
      Class::Type::Type,
      "Expected qualified name to start with a Type name."_view);
  if (first_segment == nullptr) {
    return QualifiedName();
  }

  const Token* last_segment = first_segment;
  while (cursor.matches(Class::Type::TypeAccessOp)) {
    cursor.consume();
    last_segment = cursor.require(
        Class::Type::Type,
        "Qualified name segments should all be Type names."_view);
    if (last_segment == nullptr) {
      return QualifiedName();
    }
  }

  auto start = first_segment->get_text();
  auto end = last_segment->get_text();
  return QualifiedName(View::Bytes(
      start.get_data(), end.get_data() - start.get_data() + end.get_size()));
}
