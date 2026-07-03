// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/dialect/source/source.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/dialect/lingua_franca.hpp"
#include "ttx/dialect/source/dialect.hpp"
#include "ttx/dialect/source/documentation.hpp"
#include "ttx/dialect/source/import.hpp"

using namespace Ttx::Dialect;
using namespace Ttx::Lexical;
using namespace Perimortem::Memory;

// Register "Source" with
const static LinguaFranca::Registration registration(
    "Source"_view,
    Source::Source::parse);

auto Source::Source::parse(Lexical::Cursor& cursor) -> void* {
  // Parse the documentation then the package dialect.
  const auto documentation = Ttx::Dialect::Source::Documentation::parse(cursor);

  if (!cursor.require(
          Lexical::Class::Type::Dialect,
          "Expected a dialect to be defined such as `dialect : Library`"_view)) {
    return nullptr;
  }

  const auto dialect = Ttx::Dialect::Source::Dialect::parse(cursor);
  if (!dialect.is_valid()) {
    return nullptr;
  }

  Managed::Vector<Ttx::Dialect::Source::Import> imports(cursor.get_arena());
  while (cursor.matches(Class::Type::Import)) {
    auto import = Ttx::Dialect::Source::Import::parse(cursor);
    if (!import.is_valid()) {
      return nullptr;
    }

    imports.insert(import);
  }

  // TODO: Type erase type that has a cast conversion similar to std::any.
  return &cursor.get_arena().construct<Source>(documentation, dialect, imports);
}
