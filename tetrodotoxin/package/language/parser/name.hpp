// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Package::Language::Parser {

// Name is one source backed semantic route. It preserves the authored spelling
// while exposing each `::` segment independently so Package can build and query
// the same context chain authored by the source.
class Name {
 public:
  constexpr Name(Perimortem::Core::View::Bytes spelling) : spelling(spelling) {}

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return spelling;
  }

  auto get_size() const -> Count;

  auto get_segment(Count index) const -> Perimortem::Core::View::Bytes;

  // Parses one local semantic route with Type access qualification.
  static auto parse_semantic(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Name>;

  // An external Package identity is one repository coordinate rather than a
  // TTX context route, so its complete dotted spelling remains one value.
  static auto parse_package(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::View::Bytes;

 private:
  Perimortem::Core::View::Bytes spelling;
};

}  // namespace Tetrodotoxin::Package::Language::Parser
