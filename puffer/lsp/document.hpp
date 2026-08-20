// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/record.hpp"

#include "puffer/lsp/semantic_workspace.hpp"

namespace Puffer::Lsp {

// Document retains protocol text and its discovered Package membership. A
// standalone source owns one lazy analysis, while Package analysis lives on
// the shared Documents session selected by package_root.
class Document {
 public:
  Bool active = False;
  Perimortem::Memory::Dynamic::Bytes uri;
  Perimortem::Memory::Dynamic::Bytes text;
  Perimortem::Memory::Dynamic::Bytes package_root;
  Perimortem::Memory::Dynamic::Bytes logical_route;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<SemanticWorkspace>>
      standalone_semantics;
};

}  // namespace Puffer::Lsp
