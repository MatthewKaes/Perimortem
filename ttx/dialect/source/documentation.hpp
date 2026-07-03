// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Ttx::Dialect::Source {

// The source representation of TTX documentation.
//
// Parsed as a continous block of comment lines.
class Documentation {
 public:
  static auto parse(Lexical::Cursor& cursor) -> Ttx::Documentation;
};

}  // namespace Ttx::Dialect::Source
