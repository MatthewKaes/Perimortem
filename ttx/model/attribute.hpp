// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/union.hpp"

namespace Ttx::Model {

// Attribute carries one named scalar fact that belongs to a source identity
// but is interpreted by a layer above the core Type model.
//
// Core::Static::Union owns the closed scalar value representation. Attribute
// adds only the name that gives that value meaning; it must not duplicate the
// Union tag, storage, or dispatch machinery. Separate attributes express
// separate facts, so `@slot(0)` and `@space(1)` remain easier to query and
// serialize than a structured metadata object. Layout is not reused here
// because a Layout groups real Abstracts and owns fitting. An attribute has
// neither semantic. If metadata eventually needs a typed record, that feature
// should define real Abstract contracts rather than growing an untyped object
// inside Attribute.
class Attribute {
 public:
  using Value = Perimortem::Core::Static::
      Union<Perimortem::Core::View::Bytes, Bits_64, Signed_64, Real_64, Bool>;

  constexpr Attribute(Perimortem::Core::View::Bytes key = {}, Value value = {})
      : key(key), value(value) {}

  constexpr auto get_key() const -> Perimortem::Core::View::Bytes {
    return key;
  }

  constexpr auto get_value() const -> const Value& { return value; }

  constexpr auto has_value() const -> Bool { return !value.is_null(); }
  constexpr auto is_empty() const -> Bool {
    return key.is_empty() && value.is_null();
  }

  constexpr auto operator==(const Attribute& rhs) const -> Bool {
    return key == rhs.key && value == rhs.value;
  }

 private:
  Perimortem::Core::View::Bytes key;
  Value value;
};

}  // namespace Ttx::Model
