// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Field owns one authored member shared by Library composite Type systems. It
// retains source facts until linking can construct the real TTX Addressable
// projection with one exact Type identity.
class Field {
 private:
  constexpr Field(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes type_route,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Anchor type_anchor)
      : name(name),
        type_route(type_route),
        documentation(documentation),
        visibility(visibility),
        anchor(anchor),
        type_anchor(type_anchor) {}

 public:
  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation)
      -> Perimortem::Utility::Option<Field>;

  auto link(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& context) -> Bool;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_type_route() const -> Perimortem::Core::View::Bytes {
    return type_route;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& {
    return documentation;
  }

  constexpr auto get_visibility() const -> Visibility { return visibility; }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto get_type_anchor() const -> Ttx::Lexical::Anchor {
    return type_anchor;
  }

  auto get_type() const -> Perimortem::Utility::Option<const Ttx::Model::Type&>;

  auto get_addressable() const
      -> Perimortem::Utility::Option<const Ttx::Model::Addressable&>;

  constexpr auto is_linked() const -> Bool { return stage == Stage::Linked; }

 private:
  enum class Stage : Unsigned_8 {
    Authored,
    Linked,
  };

  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes type_route;
  const Ttx::Concept::Documentation& documentation;
  Visibility visibility;
  Ttx::Lexical::Anchor anchor;
  Ttx::Lexical::Anchor type_anchor;
  Perimortem::Utility::Option<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      addressable;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language
