// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"

#include "types/compiler/attribute.hpp"

namespace Tetrodotoxin::Parser::Visitor {

auto parse_attribute(Context& ctx)
    -> Tetrodotoxin::Types::Compiler::Attribute&;

}  // namespace Tetrodotoxin::Parser::Visitor