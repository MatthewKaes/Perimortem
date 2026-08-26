// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/types/implementation.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Implementation is the empty default for one explicit erased value. A real
// Object enters the same Type through receiving Type fitting, so the default
// needs no placeholder candidate or physical Projection.
class Implementation : public Constant {
 public:
  TTX_CONTRACT(Implementation, Constant);

  static auto create_empty(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Implementation& type) -> Implementation&;

  constexpr auto get_type() const -> const Types::Implementation& override {
    return type;
  }

  auto equals(const Constant& rhs) const -> Bool override;

 private:
  constexpr Implementation(
      const Types::Implementation& type,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type) {}

  const Types::Implementation& type;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
