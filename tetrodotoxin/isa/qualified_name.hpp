// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// QualifiedName is a zero-copy view over Type(::Type)* source text.
// It is used for package names and authored export targets before a later
// binding step resolves those tokens into real TTX type facts.
class QualifiedName {
 public:
  QualifiedName() = default;
  QualifiedName(Perimortem::Core::View::Bytes text) : text(text) {}

  static auto evaluate(Ttx::Lexical::Cursor& cursor) -> QualifiedName;

  constexpr auto get_text() const -> Perimortem::Core::View::Bytes {
    return text;
  }
  constexpr auto is_valid() const -> Bool { return !text.is_empty(); }

 private:
  Perimortem::Core::View::Bytes text;
};

}  // namespace Tetrodotoxin::Isa
