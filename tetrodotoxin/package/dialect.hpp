// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Package {

// Dialect owns Package body transaction policy and constructs one durable
// Package Monograph from complete Dependency and Source values.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  constexpr Dialect() = default;

  auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& doc,
      Ttx::Concept::Abstract&)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;
};

}  // namespace Tetrodotoxin::Package
