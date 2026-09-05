// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::App::Language {

// Route keeps one authored path to an application participant. Unlike a Type
// reference, the destination may be a Scene Monograph, a Library source, or
// another contextual identity. Each segment still follows ordinary resolve
// queries, so App never copies Package names into its own symbol table.
class Route {
 public:
  static constexpr auto create_authored(
      Perimortem::Core::View::Bytes spelling,
      Ttx::Lexical::Anchor anchor) -> Route {
    return Route(spelling, anchor);
  }

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes spelling) -> Route;

  auto resolve(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;

  auto resolve_restored(const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;

  constexpr auto get_spelling() const -> Perimortem::Core::View::Bytes {
    return spelling;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  constexpr Route(
      Perimortem::Core::View::Bytes spelling,
      Ttx::Lexical::Anchor anchor)
      : spelling(spelling), anchor(anchor) {}

  Perimortem::Core::View::Bytes spelling;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::App::Language
