// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/query.hpp"

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

  auto resolve(Ttx::Lexical::Cursor& cursor, ttx_abstract context) const
      -> ttx_abstract;

  // A concrete declaration owner may select the first route segment through
  // its private lexical policy, then return to ordinary public Type context
  // queries for every explicit `::` suffix.
  auto resolve_selected(
      Ttx::Lexical::Cursor& cursor,
      ttx_abstract selected_root) const -> ttx_abstract;

  // Archive restoration follows the same semantic route after every owner has
  // reconstructed its identities. It has no authored Cursor to report through,
  // so absence lets the persistent Dialect reject the complete transaction.
  auto resolve_restored(ttx_abstract context) const -> ttx_abstract;

  auto resolve_restored_selected(ttx_abstract selected_root) const
      -> ttx_abstract;

  auto get_root() const -> Perimortem::Core::View::Bytes;

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
