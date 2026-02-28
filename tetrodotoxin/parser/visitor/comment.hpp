// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"

#include "memory/view/byte.hpp"

namespace Tetrodotoxin::Parser::Visitor {

auto parse_comment(Context& ctx) -> View::Bytes;

}  // namespace Tetrodotoxin::Parser::Visitor
