// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Type retains one authored Type route without acquiring semantic identity.
// Each segment is resolved through the preceding Abstract so qualification
// remains useful for any owner that exposes a Type shaped context. Alias edges
// redirect through their exact targets, while the terminal object is returned
// without asking an incomplete Type to resolve away its reserved identity.
class Type {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Type>;

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_root() const -> Perimortem::Core::View::Bytes {
    return route.slice(0, root_size);
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  auto resolve(const Ttx::Concept::Abstract& context) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_from(const Ttx::Concept::Abstract& root) const
      -> const Ttx::Concept::Abstract&;

  // Qualification preserves the original caller scope. Alias redirection may
  // change the selected identity, but it never changes which hosted Type owns
  // private authority over a later segment.
  auto resolve_from(
      const Ttx::Concept::Abstract& root,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

 private:
  constexpr Type(
      Perimortem::Core::View::Bytes route,
      Count root_size,
      Ttx::Lexical::Anchor anchor)
      : route(route), root_size(root_size), anchor(anchor) {}

  Perimortem::Core::View::Bytes route;
  Count root_size;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Library::Language::Access
