// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <optional>
#include "parser/context.hpp"

namespace Tetrodotoxin::Language::Parser {

class Comment {
 public:
  static auto parse(Context& ctx) -> std::optional<std::string>;
};

}  // namespace Tetrodotoxin::Language::Parser
