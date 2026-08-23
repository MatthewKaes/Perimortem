// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
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
  Perimortem::Core::Static::Vector<
      Ttx::Concept::Reference<Tetrodotoxin::Language::Dialect>, 1>
      installed = {{dialect}};

  Count error_count = errors.get_size();
  auto interpreted = Tetrodotoxin::Language::Dialect::interpret_source(
      installed.get_view(), cursor, dialect);
  if (!interpreted || errors.get_size() != error_count ||
      !interpreted->is<Tetrodotoxin::Package::Language::Monograph>()) {
    return {};
  }

  return static_cast<Tetrodotoxin::Package::Language::Monograph&>(
      *interpreted);
}

}  // namespace Validation
