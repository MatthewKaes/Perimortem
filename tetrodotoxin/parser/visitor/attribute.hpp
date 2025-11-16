// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"
#include "lexical/tokenizer.hpp"

#include "types/compiler/attribute.hpp"

#include <optional>

namespace Tetrodotoxin::Parser::Visitor {

auto parse_attribute(Context& ctx)
    -> Tetrodotoxin::Types::Compiler::Attribute*;

}  // namespace Tetrodotoxin::Parser::Visitor