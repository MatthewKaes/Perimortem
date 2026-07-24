// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Puffer::Lsp {

class Document {
 public:
  Bool active = False;
  Perimortem::Memory::Dynamic::Bytes uri;
  Perimortem::Memory::Dynamic::Bytes text;
};

}  // namespace Puffer::Lsp
