// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "lexical/source.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <filesystem>

namespace Tetrodotoxin::Parser {

struct Error {
 public:
  static bool colorful;

  static std::string error_color_primary;
  static std::string error_color_secondary;
  static std::string error_color_tertiary;
  static std::string error_color_highlight;

  Error(const std::filesystem::path& source_map,
        std::string_view details,
        std::string_view source,
        std::optional<Lexical::Location> loc = std::nullopt,
        std::optional<std::string_view> line_range = std::nullopt,
        std::optional<uint32_t> error_range = std::nullopt);

  inline auto get_message() const -> const std::string& { return msg; };

 private:
  std::string msg;
};

using Errors = std::vector<Error>;

}  // namespace Tetrodotoxin::Parser
