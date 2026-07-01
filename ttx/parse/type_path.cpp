// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/type_path.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/memory/managed/vector.hpp"

auto Ttx::Parse::TypePath::parse(Cursor& cursor) -> TypePath {
  TypePath result;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> segments(
      cursor.get_arena());

  const Lexical::Token& first_segment = cursor.require(
      Lexical::Class::Type::Type, "Expected TTX type path."_view);
  if (cursor.has_error()) {
    return result;
  }

  segments.insert(first_segment.get_text());
  while (cursor.matches(Lexical::Class::Type::TypeAccessOp)) {
    cursor.consume();
    const Lexical::Token& segment = cursor.require(
        Lexical::Class::Type::Type, "Expected type path segment after `::`."_view);
    if (cursor.has_error()) {
      return result;
    }

    segments.insert(segment.get_text());
  }

  result.segments = segments.get_view();
  return result;
}

auto Ttx::Parse::TypePath::get_name() const -> Perimortem::Core::View::Bytes {
  if (segments.is_empty()) {
    return Perimortem::Core::View::Bytes();
  }

  return segments[segments.get_size() - 1];
}

auto Ttx::Parse::TypePath::get_segments() const
    -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
  return segments;
}

auto Ttx::Parse::TypePath::get_segment_count() const -> Count {
  return segments.get_size();
}

auto Ttx::Parse::TypePath::is_empty() const -> Bool {
  return segments.is_empty();
}
