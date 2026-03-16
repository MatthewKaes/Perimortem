// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/standard_types.hpp"

namespace Tetrodotoxin::Lexical {
struct Location {
  UInt source_index = 0;
  UInt parse_index = 0;
  UInt line = 1;
  UInt column = 1;
};

}  // namespace Tetrodotoxin::Lexical