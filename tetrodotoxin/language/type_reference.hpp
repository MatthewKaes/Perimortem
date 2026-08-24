// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Language {

// TypeReference keeps one authored Type route without Generic arguments until
// the surrounding semantic island is ready to answer it. It retains only the
// source spelling and Anchor, so each consuming Dialect remains responsible for
// its Type system.
class TypeReference {
 public:
  static constexpr auto create(
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Anchor anchor) -> TypeReference {
    return TypeReference(route, anchor);
  }

  auto resolve(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  constexpr TypeReference(
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Anchor anchor)
      : route(route), anchor(anchor) {}

  Perimortem::Core::View::Bytes route;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Language
