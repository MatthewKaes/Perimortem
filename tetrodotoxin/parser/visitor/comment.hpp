// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/view/byte.hpp"

#include "parser/context.hpp"

namespace Tetrodotoxin::Parser::Visitor {

auto parse_comment(Context& ctx) -> Core::View::Bytes;

}  // namespace Tetrodotoxin::Parser::Visitor
