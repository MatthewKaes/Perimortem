// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Validation {

// Package semantic tests exercise the real source envelope and Dialect without
// asking Workspace to publish an intentionally incomplete Package. Proxying the
// source first keeps every Documentation line, route, and statement Span in the
// same Arena as the Monograph that retains them.
inline auto interpret_package(
    Perimortem::Memory::Allocator::Arena& arena,
    Tetrodotoxin::Package::Dialect& dialect,
    Ttx::Lexical::Errors& errors,
    Perimortem::Core::View::Bytes source,
    Perimortem::Core::View::Bytes path)
    -> Perimortem::Core::Option<Tetrodotoxin::Package::Language::Monograph&> {
  Perimortem::Core::View::Bytes retained_source = arena.proxy(source);
  Perimortem::Core::View::Bytes retained_path = arena.proxy(path);
  Ttx::Lexical::Tokenizer tokenizer(arena, retained_source, retained_path);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Ttx::Lexical::Cursor cursor(tokenizer, errors, associations);
  Tetrodotoxin::Language::Dialect* installed[] = {&dialect};

  auto interpreted = Tetrodotoxin::Language::Dialect::interpret_source(
      installed, cursor, dialect);
  if (!interpreted ||
      !interpreted->is<Tetrodotoxin::Package::Language::Monograph>()) {
    return {};
  }

  return static_cast<Tetrodotoxin::Package::Language::Monograph&>(*interpreted);
}

}  // namespace Validation
