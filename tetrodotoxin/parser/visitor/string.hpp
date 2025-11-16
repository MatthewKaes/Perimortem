// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/context.hpp"

namespace Tetrodotoxin::Parser::Visitor {

auto parse_string(Context& ctx)
    -> std::string_view;

}  // namespace Tetrodotoxin::Parser::Visitor