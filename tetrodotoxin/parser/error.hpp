// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/source.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace Tetrodotoxin::Language::Parser {

struct Error {
public:
  static bool colorful;

  static std::string error_color_primary;
  static std::string error_color_secondary;
  static std::string error_color_tertiary;
  static std::string error_color_highlight;

  Error(std::string_view source_map, std::string_view details,
        const ByteView &source, std::optional<Location> loc = std::nullopt,
        std::optional<std::string_view> line_range = std::nullopt,
        std::optional<uint32_t> error_range = std::nullopt);

  std::string msg;
};

using Errors = std::vector<Error>;

} // namespace Tetrodotoxin::Language::Parser
