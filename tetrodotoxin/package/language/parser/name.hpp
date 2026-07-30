// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Package::Language::Parser {

// Parses the exact qualified names authored by Package statements. Every
// segment is a Type name and every separator must touch both neighboring
// segments in the source bytes.
class Name {
 public:
  Name() = delete;

  // Parses one local semantic name with Type access qualification.
  static auto parse_semantic(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::View::Bytes;

  // Parses one external Package identity with address qualification.
  static auto parse_package(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Package::Language::Parser
