// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "puffer/lsp/document.hpp"
#include "ttx/concept/abstract.hpp"

namespace Puffer::Lsp {

// Documents owns the bounded set of files opened by one LSP session. Protocol
// updates replace complete text values, while semantic work stays lazy until a
// diagnostic, token, or hover request needs the matching snapshot.
class Documents {
 public:
  auto upsert(
      Perimortem::Core::View::Bytes uri,
      Perimortem::Core::View::Bytes source) -> void;
  auto erase(Perimortem::Core::View::Bytes uri) -> void;
  auto get_text(Perimortem::Core::View::Bytes uri) const
      -> Perimortem::Memory::Dynamic::Bytes;
  auto find_semantic(
      Perimortem::Core::View::Bytes uri,
      Count line,
      Count utf_16_character)
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;
  auto get_errors(Perimortem::Core::View::Bytes uri)
      -> Perimortem::Core::Option<const Ttx::Lexical::Errors&>;

 private:
  auto find(Perimortem::Core::View::Bytes uri) const -> Count;

  Perimortem::Core::Static::Vector<Document, 64> records;
};

}  // namespace Puffer::Lsp
