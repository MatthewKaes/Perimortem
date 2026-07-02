// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/symbol_path.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/memory/managed/vector.hpp"

auto Ttx::Parse::SymbolPath::parse(Cursor& cursor) -> SymbolPath {
  SymbolPath result;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> segments(
      cursor.get_arena());

  // The envelope symbol grammar is intentionally only PascalCase segments
  // joined by `::`. Parameter layouts and richer type expressions need
  // imported names and therefore belong to dialect or resolution code.
  Count starting_error_count = cursor.get_errors().get_size();
  const Lexical::Token& first_segment = cursor.require(
      Lexical::Class::Type::Type, "Expected TTX symbol path."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    return result;
  }

  segments.insert(first_segment.get_text());
  while (cursor.matches(Lexical::Class::Type::TypeAccessOp)) {
    cursor.consume();
    const Lexical::Token& segment = cursor.require(
        Lexical::Class::Type::Type,
        "Expected symbol path segment after `::`."_view);
    if (cursor.get_errors().get_size() != starting_error_count) {
      return result;
    }

    segments.insert(segment.get_text());
  }

  result.segments = segments.get_view();
  return result;
}

auto Ttx::Parse::SymbolPath::write(
    Perimortem::Memory::Dynamic::Bytes& output) const -> void {
  output.clear();
  for (Count segment_index = 0; segment_index < segments.get_size();
       segment_index++) {
    if (segment_index != 0) {
      output.concat("::"_view);
    }
    output.concat(segments[segment_index]);
  }
}
