// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace Tetrodotoxin::Language::Parser {
using Byte = uint8_t;
using Bytes = std::vector<Byte>;
using ByteView = std::span<const Byte>;

struct Location {
  uint32_t source_index = 0;
  uint32_t parse_index = 0;
  uint32_t line = 1;
  uint32_t column = 1;
};

} // namespace Tetrodotoxin::Language::Parser