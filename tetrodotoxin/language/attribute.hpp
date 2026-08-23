// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"

#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// Attribute is one authored optional prefix fact. Its key and source shaped
// value storage remain in the Cursor's transaction Arena with the source bytes
// and Anchor, while its consumer owns the meaning of the key.
class Attribute {
 public:
  using Value = Perimortem::Core::Static::
      Union<Perimortem::Core::View::Bytes, U64, S64, R64, Bool>;

  // Consumes the complete consecutive Attribute prefix. Absence is valid and
  // leaves the Cursor unchanged. A malformed prefix publishes no partial view.
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::View::Vector<Attribute>;

  static constexpr auto create_synthetic(
      Perimortem::Core::View::Bytes key,
      Value value = {}) -> Attribute {
    return Attribute(
        key, value, Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()));
  }

  constexpr auto get_key() const -> Perimortem::Core::View::Bytes {
    return key;
  }

  constexpr auto get_value() const -> const Value& { return value; }

  constexpr auto has_value() const -> Bool { return !value.is_null(); }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  constexpr Attribute(
      Perimortem::Core::View::Bytes key,
      Value value,
      Ttx::Lexical::Anchor anchor)
      : key(key), value(value), anchor(anchor) {}

  Perimortem::Core::View::Bytes key;
  Value value;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Language
