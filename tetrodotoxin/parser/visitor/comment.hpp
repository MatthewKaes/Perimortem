// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"

#include "core/memory/managed_string.hpp"

namespace Tetrodotoxin::Parser::Visitor {

auto parse_comment(Context& ctx) -> Perimortem::Memory::ManagedString;

}  // namespace Tetrodotoxin::Parser::Visitor
