// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// TypeReference is one authored, identity-free Type route. Each segment keeps
// its exact Token for diagnostics and an Arena-stable spelling for delayed
// lookup. It acquires no semantic edge during parsing: the declaration owner
// resolves the route only after the surrounding Type inventory is complete.
class TypeReference {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<TypeReference>;

  constexpr auto get_size() const -> Count { return tokens.get_size(); }

  constexpr auto get_token(Count index) const -> Ttx::Lexical::Token {
    return tokens.get_data()[index];
  }

  constexpr auto get_name(Count index) const -> Perimortem::Core::View::Bytes {
    return names.get_data()[index];
  }

  constexpr auto get_root() const -> Perimortem::Core::View::Bytes {
    return get_name(0);
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto matches(const TypeReference& other) const -> Bool {
    if (get_size() != other.get_size()) {
      return False;
    }

    for (Count i = 0; i < get_size(); i++) {
      if (get_name(i) != other.get_name(i)) {
        return False;
      }
    }
    return True;
  }

  auto resolve(const Ttx::Concept::Abstract& context) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_from(const Ttx::Concept::Abstract& root) const
      -> const Ttx::Concept::Abstract&;

  // Qualification preserves the original caller scope. Alias resolution may
  // redirect an edge, but it never transfers private authority to the target.
  auto resolve_from(
      const Ttx::Concept::Abstract& root,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

 private:
  constexpr TypeReference(
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Ttx::Lexical::Anchor anchor)
      : tokens(tokens), names(names), anchor(anchor) {}

  Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Library::Language
