// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Library {

// Dialect owns Library interpretation and exposes the immutable binary wide
// intrinsic vocabulary shared by every Monograph it interprets.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  constexpr Dialect(Ttx::Concept::Abstract& registry)
      : Tetrodotoxin::Language::Dialect(registry) {}

  auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& interpretation_context)
      -> Perimortem::Utility::Option<Monograph&> override;

  auto resolve_intrinsic(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract&;
};

}  // namespace Tetrodotoxin::Library
