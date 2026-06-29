// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Expression {

auto parse_primary(Context& ctx) -> Expression*;

}  // namespace Tetrodotoxin::Syntax::Expression
