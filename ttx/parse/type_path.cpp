// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/type_path.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/memory/managed/vector.hpp"

auto Ttx::Parse::TypePath::parse(Cursor& cursor) -> TypePath {
  TypePath result;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> segments(
      cursor.get_arena());

  // The envelope path grammar is intentionally only PascalCase segments joined
  // by `::`. Parameter layouts and richer type expressions need imported names
  // and therefore belong to dialect or resolution code.
  Count starting_error_count = cursor.get_errors().get_size();
  const Lexical::Token& first_segment = cursor.require(
      Lexical::Class::Type::Type, "Expected TTX type path."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    return result;
  }

  segments.insert(first_segment.get_text());
  while (cursor.matches(Lexical::Class::Type::TypeAccessOp)) {
    cursor.consume();
    const Lexical::Token& segment = cursor.require(
        Lexical::Class::Type::Type, "Expected type path segment after `::`."_view);
    if (cursor.get_errors().get_size() != starting_error_count) {
      return result;
    }

    segments.insert(segment.get_text());
  }

  result.segments = segments.get_view();
  return result;
}
