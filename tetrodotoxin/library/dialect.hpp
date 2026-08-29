// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Library {

// Dialect implements the installed Library source protocol. Every semantic
// identity belongs to the Monograph created for that source.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect(Perimortem::Core::View::Bytes name = "Library"_view)
      : Tetrodotoxin::Language::Dialect(name) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  auto encode(const Ttx::Concept::Abstract& monograph) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes> override;

  auto restore(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;
};

}  // namespace Tetrodotoxin::Library
