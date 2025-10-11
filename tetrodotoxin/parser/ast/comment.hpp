// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <optional>
#include "parser/ast/context.hpp"

namespace Tetrodotoxin::Language::Parser {

class Comment {
 public:
  static auto parse(Context& ctx) -> std::optional<Comment>;

  std::string body;
};

}  // namespace Tetrodotoxin::Language::Parser
