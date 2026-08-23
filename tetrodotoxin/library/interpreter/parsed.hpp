// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Parsed keeps one real semantic identity together with the outcome of the
// source form that produced it. Recovery can retain the identity for editor
// queries while completion still rejects the malformed source transaction.
template <typename semantic_type>
class Parsed {
 public:
  constexpr Parsed(semantic_type& semantic, Bool accepted)
      : semantic(semantic), accepted(accepted) {}

  constexpr auto get_semantic() const -> semantic_type& {
    return semantic.get();
  }

  constexpr auto is_accepted() const -> Bool { return accepted; }

 private:
  Ttx::Concept::Reference<semantic_type> semantic;
  Bool accepted;
};

}  // namespace Tetrodotoxin::Library::Interpreter
