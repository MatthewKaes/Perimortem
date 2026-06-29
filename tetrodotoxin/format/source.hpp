// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"
#include "tetrodotoxin/syntax/package.hpp"

namespace Tetrodotoxin::Format {

auto format(Formatter& formatter, const Syntax::Package& package) -> void;
auto format(const Syntax::Package& package)
    -> Perimortem::Memory::Dynamic::Bytes;

}  // namespace Tetrodotoxin::Format
