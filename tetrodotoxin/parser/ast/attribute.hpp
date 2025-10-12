// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/ast/context.hpp"
#include "parser/tokenizer.hpp"

#include <optional>

namespace Tetrodotoxin::Language::Parser {

class Attribute {
 public:
  Attribute(std::string_view name) : name(name) {};
  static auto parse(Context& ctx) -> std::optional<Attribute>;

  std::string_view name;
  std::string_view value;
};

}  // namespace Tetrodotoxin::Language::Parser
