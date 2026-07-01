// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/package.hpp"
#include "tetrodotoxin/syntax/ttx.hpp"

namespace Tetrodotoxin::Format {

auto format(Formatter& formatter, const Syntax::Ttx& package) -> void;
auto format(const Syntax::Ttx& package)
    -> Perimortem::Memory::Dynamic::Bytes;

}  // namespace Tetrodotoxin::Format
