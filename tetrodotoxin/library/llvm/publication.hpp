// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Publication connects one Package-reachable semantic identity to the native
// symbol defined by this member object. Package later supplies the member and
// host route framing while this record preserves exact identity.
class Publication {
 public:
  constexpr Publication(
      const Ttx::Concept::Abstract& semantic,
      Perimortem::Core::View::Bytes symbol)
      : semantic(semantic), symbol(symbol) {}

  constexpr auto get_semantic() const -> const Ttx::Concept::Abstract& {
    return semantic.get();
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> semantic;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Library::Llvm
