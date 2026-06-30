// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/syntax/package.hpp"

namespace Tetrodotoxin::Format {

auto format(Formatter& ctx, const Syntax::Package& package) -> void;

}  // namespace Tetrodotoxin::Format
