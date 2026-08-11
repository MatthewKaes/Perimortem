// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// Definition retains the authored prefix shared by concrete Tetrodotoxin
// languages. It owns source shape only. The consumer named by the qualifier
// decides legality, parses the remaining form, and constructs its real object.
class Definition {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Definition&>;

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
      Perimortem::Core::View::Bytes name,
      Ttx::Lexical::Token name_token,
      Ttx::Lexical::Token qualifier,
      Ttx::Lexical::Anchor anchor)
      : documentation(documentation),
        attributes(attributes),
        modifiers(modifiers),
        name(name),
        name_token(name_token),
        qualifier(qualifier),
        anchor(anchor) {}

  const Ttx::Concept::Documentation& documentation;
  Perimortem::Core::View::Vector<Attribute> attributes;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> modifiers;
  Perimortem::Core::View::Bytes name;
  Ttx::Lexical::Token name_token;
  Ttx::Lexical::Token qualifier;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Language
