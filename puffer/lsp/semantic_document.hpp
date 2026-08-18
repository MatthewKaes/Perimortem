// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/errors.hpp"

namespace Puffer::Lsp {

// SemanticDocument owns one completed Workspace snapshot for the exact text
// version retained by an open LSP document. Editing replaces the whole
// snapshot, preserving Environment's source-transaction publication barrier.
class SemanticDocument {
 public:
  SemanticDocument(
      Perimortem::Core::View::Bytes uri,
      Perimortem::Core::View::Bytes source);

  auto find(Count byte_offset) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;

  constexpr auto get_errors() const -> const Ttx::Lexical::Errors& {
    return errors;
  }

 private:
  Ttx::Lexical::Errors errors;
  Tetrodotoxin::Environment::Workspace workspace;
  Perimortem::Core::Option<const Ttx::Lexical::Associations&> associations;
};

}  // namespace Puffer::Lsp
