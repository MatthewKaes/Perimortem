// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

namespace Tetrodotoxin::Backend::Llvm::Representation {

// Emission identifies whether one native operation addresses the whole module
// or its active Function body. The distinction is backend transaction state
// and never enters the TTX semantic graph.
class Emission {
 public:
  enum class Kind : U8 {
    Module,
    Body,
  };

  constexpr auto get_kind() const -> Kind { return kind; }

 protected:
  constexpr explicit Emission(Kind kind) : kind(kind) {}

 private:
  Kind kind;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Representation
