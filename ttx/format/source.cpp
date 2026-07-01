// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/source.hpp"

using namespace Tetrodotoxin;

auto Tetrodotoxin::Format::format(const Syntax::Ttx& package)
    -> Perimortem::Memory::Dynamic::Bytes {
  Formatter formatter;
  format(formatter, package);
  return formatter.get_output();
}
