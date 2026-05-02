// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"

#include "perimortem/memory/view/byte.hpp"

namespace Tetrodotoxin::Parser::Visitor {

auto parse_comment(Context& ctx) -> Core::View::Bytes;

}  // namespace Tetrodotoxin::Parser::Visitor
