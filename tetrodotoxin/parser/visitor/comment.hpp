// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"

#include "memory/byte_view.hpp"

namespace Tetrodotoxin::Parser::Visitor {

auto parse_comment(Context& ctx) -> Perimortem::Memory::ByteView;

}  // namespace Tetrodotoxin::Parser::Visitor
