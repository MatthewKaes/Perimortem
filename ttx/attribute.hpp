// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Ttx {

// Attribute carries one named scalar fact that belongs to a source identity
// but is interpreted by a layer above the core Type model.
//
// The value is deliberately a closed scalar union. Separate attributes express
// separate facts, so `@slot(0)` and `@space(1)` remain easier to query and
// serialize than a structured metadata object. Layout is not reused here
// because a Layout describes typed members, construction shape, and fitting.
// An attribute has none of those semantics. If metadata eventually needs a
// typed record, that feature should define a real Layout contract rather than
// growing an untyped object inside Attribute.
class Attribute {
 public:
  enum class Kind : Bits_8 {
    Empty,
    Bytes,
    Unsigned,
    Signed,
    Real,
    Boolean,
  };

  constexpr Attribute() = default;
  explicit constexpr Attribute(Perimortem::Core::View::Bytes key) : key(key) {}
  constexpr Attribute(
      Perimortem::Core::View::Bytes key,
      Perimortem::Core::View::Bytes value)
      : key(key), kind(Kind::Bytes), value(value) {}
  constexpr Attribute(Perimortem::Core::View::Bytes key, Bits_64 value)
      : key(key), kind(Kind::Unsigned), value(value) {}
  constexpr Attribute(Perimortem::Core::View::Bytes key, Signed_64 value)
      : key(key), kind(Kind::Signed), value(value) {}
  constexpr Attribute(Perimortem::Core::View::Bytes key, Real_64 value)
      : key(key), kind(Kind::Real), value(value) {}
  constexpr Attribute(Perimortem::Core::View::Bytes key, Bool value)
      : key(key), kind(Kind::Boolean), value(value) {}

  constexpr auto get_key() const -> Perimortem::Core::View::Bytes {
    return key;
  }

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_bytes() const -> Perimortem::Core::View::Bytes {
    return kind == Kind::Bytes ? value.bytes : Perimortem::Core::View::Bytes();
  }

  constexpr auto get_unsigned() const -> Bits_64 {
    return kind == Kind::Unsigned ? value.unsigned_value : Bits_64(0);
  }

  constexpr auto get_signed() const -> Signed_64 {
    return kind == Kind::Signed ? value.signed_value : Signed_64(0);
  }

  constexpr auto get_real() const -> Real_64 {
    return kind == Kind::Real ? value.real_value : Real_64(0);
  }

  constexpr auto get_boolean() const -> Bool {
    return kind == Kind::Boolean ? value.boolean_value : False;
  }

  constexpr auto has_value() const -> Bool { return kind != Kind::Empty; }
  constexpr auto is_empty() const -> Bool {
    return key.is_empty() && kind == Kind::Empty;
  }

  constexpr auto operator==(const Attribute& rhs) const -> Bool {
    if (key != rhs.key || kind != rhs.kind) {
      return False;
    }

    switch (kind) {
    case Kind::Empty:
      return True;
    case Kind::Bytes:
      return value.bytes == rhs.value.bytes;
    case Kind::Unsigned:
      return value.unsigned_value == rhs.value.unsigned_value;
    case Kind::Signed:
      return value.signed_value == rhs.value.signed_value;
    case Kind::Real:
      return value.real_value == rhs.value.real_value;
    case Kind::Boolean:
      return value.boolean_value == rhs.value.boolean_value;
    }

    return False;
  }

 private:
  union Value {
    constexpr Value() : unsigned_value(0) {}
    constexpr Value(Perimortem::Core::View::Bytes value) : bytes(value) {}
    constexpr Value(Bits_64 value) : unsigned_value(value) {}
    constexpr Value(Signed_64 value) : signed_value(value) {}
    constexpr Value(Real_64 value) : real_value(value) {}
    constexpr Value(Bool value) : boolean_value(value) {}

    Perimortem::Core::View::Bytes bytes;
    Bits_64 unsigned_value;
    Signed_64 signed_value;
    Real_64 real_value;
    Bool boolean_value;
  };

  Perimortem::Core::View::Bytes key;
  Kind kind = Kind::Empty;
  Value value;
};

}  // namespace Ttx
