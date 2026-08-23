// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// Definition owns the common declaration facts shared by concrete
// Tetrodotoxin languages. Authored definitions begin with a retained prefix and
// complete their Anchor after the concrete grammar succeeds. Synthetic
// definitions retain the same semantic contract without fabricating lexical
// evidence.
class Definition {
 public:
  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Definition&>;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes reserved_name,
      Visibility visibility,
      Ttx::Lexical::Anchor anchor) -> Definition&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Vector<Attribute> attributes,
      Perimortem::Core::View::Bytes name,
      Visibility visibility) -> Definition&;

  // Creates one complete Definition after a concrete grammar has accepted its
  // exact authored form. This keeps alternate declaration orders in their
  // concrete language without copying common declaration facts into the
  // resulting semantic identity.
  static auto create_authored(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Vector<Attribute> attributes,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> modifiers,
      Visibility visibility,
      Ttx::Lexical::Token visibility_token,
      Perimortem::Core::View::Bytes name,
      Ttx::Lexical::Token name_token,
      Ttx::Lexical::Token qualifier,
      Ttx::Lexical::Anchor anchor) -> Definition&;

  // Completes one authored declaration with its concrete grammar range. A
  // rejected or repeated completion leaves the original source fact intact.
  auto complete(Ttx::Lexical::Token focus, Ttx::Lexical::Token closing) -> Bool;

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& {
    return documentation;
  }

  constexpr auto get_attributes() const
      -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }

  constexpr auto get_modifiers() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Token> {
    return modifiers;
  }

  constexpr auto get_visibility() const -> Visibility { return visibility; }

  constexpr auto get_visibility_token() const -> Ttx::Lexical::Token {
    return visibility_token;
  }

  // Host is the enclosing mutable definition transaction owner and access
  // authority. It is never universal semantic parentage or a required route
  // through the TTX graph.
  constexpr auto get_host() -> Ttx::Concept::Abstract& { return host; }

  constexpr auto get_host() const -> const Ttx::Concept::Abstract& {
    return host;
  }

  constexpr auto is_authored() const -> Bool { return Bool(name_token); }

  constexpr auto is_complete() const -> Bool { return anchor_complete; }

  constexpr auto is_published() const -> Bool {
    return visibility != Visibility::Private;
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_name_token() const -> Ttx::Lexical::Token {
    return name_token;
  }

  constexpr auto get_qualifier() const -> Ttx::Lexical::Token {
    return qualifier;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto get_name_anchor() const -> Ttx::Lexical::Anchor {
    return Ttx::Lexical::Anchor::create(Ttx::Lexical::Span(name_token));
  }

 private:
  constexpr Definition(
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Vector<Attribute> attributes,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> modifiers,
      Visibility visibility,
      Ttx::Lexical::Token visibility_token,
      Perimortem::Core::View::Bytes name,
      Ttx::Lexical::Token name_token,
      Ttx::Lexical::Token qualifier,
      Ttx::Concept::Abstract& host,
      Ttx::Lexical::Anchor anchor,
      Bool anchor_complete)
      : documentation(documentation),
        attributes(attributes),
        modifiers(modifiers),
        visibility(visibility),
        visibility_token(visibility_token),
        name(name),
        name_token(name_token),
        qualifier(qualifier),
        host(host),
        anchor(anchor),
        anchor_complete(anchor_complete) {}

  const Ttx::Concept::Documentation& documentation;
  Perimortem::Core::View::Vector<Attribute> attributes;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> modifiers;
  Visibility visibility;
  Ttx::Lexical::Token visibility_token;
  Perimortem::Core::View::Bytes name;
  Ttx::Lexical::Token name_token;
  Ttx::Lexical::Token qualifier;
  Ttx::Concept::Abstract& host;
  Ttx::Lexical::Anchor anchor;
  Bool anchor_complete;
};

}  // namespace Tetrodotoxin::Language
