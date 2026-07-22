// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Puffer::Lsp {

class Document {
 public:
  Bool active = False;
  Perimortem::Memory::Dynamic::Bytes uri;
  Perimortem::Memory::Dynamic::Bytes text;
};

}  // namespace Tetrodotoxin::Puffer::Lsp
