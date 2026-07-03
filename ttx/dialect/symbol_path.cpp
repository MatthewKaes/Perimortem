// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/symbol_path.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Dialect;
using namespace Ttx::Lexical;

auto SymbolPath::parse(Cursor& cursor) -> SymbolPath {
  const Token* first_segment = cursor.require(
      Class::Type::Type,
      "Expected symbol path to start with a Type name."_view);
  if (first_segment == nullptr) {
    return SymbolPath();
  }

  const Token* last_segment = first_segment;
  while (cursor.matches(Class::Type::TypeAccessOp)) {
    cursor.consume();
    last_segment = cursor.require(
        Class::Type::Type,
        "Symbol path segments should all be Type names."_view);
    if (last_segment == nullptr) {
      return SymbolPath();
    }
  }

  auto start = first_segment->get_text();
  auto end = last_segment->get_text();
  return SymbolPath(View::Bytes(
      start.get_data(), end.get_data() - start.get_data() + end.get_size()));
}
