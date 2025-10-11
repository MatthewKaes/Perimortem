// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/ast/context.hpp"

#include <optional>

namespace Tetrodotoxin::Language::Parser {

class Init {
 public:
  static auto parse(Context& ctx) -> std::unique_ptr<Init>;

  std::string body;
};

}  // namespace Tetrodotoxin::Language::Parser
