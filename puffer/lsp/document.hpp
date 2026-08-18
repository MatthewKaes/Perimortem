// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/object.hpp"

#include "puffer/lsp/semantic_document.hpp"

namespace Puffer::Lsp {

// Document retains protocol text independently from its lazily rebuilt semantic
// snapshot. Replacing text drops the complete snapshot so editor queries never
// observe a graph built from another document version.
class Document {
 public:
  Bool active = False;
  Perimortem::Memory::Dynamic::Bytes uri;
  Perimortem::Memory::Dynamic::Bytes text;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Object<SemanticDocument>>
      semantics;
};

}  // namespace Puffer::Lsp
