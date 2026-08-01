// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Import retains one exact Package local route authored by a Library source.
// Package owns route interpretation while Library owns declaration expansion.
class Import {
 public:
  constexpr Import(Perimortem::Core::View::Bytes route) : route(route) {}

  // Consumes one complete using statement and keeps only its durable route.
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Utility::Option<Import>;

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

 private:
  Perimortem::Core::View::Bytes route;
};

}  // namespace Tetrodotoxin::Library::Language
