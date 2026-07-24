// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Utility {

// None is the explicit unit value carried by an Option with no object.
class None {};

inline constexpr None none;

// Option represents either one owned value or None. The value lives directly
// inside the Option so a function can safely return an object created on its
// stack. Construction and destruction follow the selected value's lifetime,
// including for move-only and non-default-constructible types.
//
// Observation is deliberately restricted to visit(). Option does not expose a
// pointer, boolean test, fallback object, or unchecked accessor that could turn
// absence back into nullable control flow.
template <typename Type>
class Option {
 private:
  // State is the sole authority for the union member's lifetime. It changes to
  // Value only after construction and returns to None only after destruction.
  enum class State : Unsigned_8 {
    None,
    Value,
  };

  template <typename Candidate>
  constexpr auto construct(Candidate&& candidate) -> void {
    new (&value) Type(static_cast<Candidate&&>(candidate));
    state = State::Value;
  }

  constexpr auto clear() -> void {
    if (state == State::Value) {
      value.~Type();
      state = State::None;
    }
  }

  union {
    Type value;
  };
  State state = State::None;

 public:
  constexpr Option() {}
  constexpr Option(const None&) {}

  constexpr Option(const Type& selected)
    requires(__is_constructible(Type, const Type&))
      : value(selected), state(State::Value) {}

  constexpr Option(Type&& selected)
    requires(__is_constructible(Type, Type &&))
      : value(static_cast<Type&&>(selected)), state(State::Value) {}

  constexpr Option(const Option& source)
    requires(__is_constructible(Type, const Type&))
  {
    if (source.state == State::Value) {
      construct(source.value);
    }
  }

  constexpr Option(Option&& source)
    requires(__is_constructible(Type, Type &&))
  {
    if (source.state == State::Value) {
      construct(static_cast<Type&&>(source.value));
    }
  }

  constexpr ~Option() { clear(); }

  constexpr auto operator=(const Option& source) -> Option&
    requires(__is_constructible(Type, const Type&))
  {
    if (this == &source) {
      return *this;
    }

    clear();
    if (source.state == State::Value) {
      construct(source.value);
    }
    return *this;
  }

  constexpr auto operator=(Option&& source) -> Option&
    requires(__is_constructible(Type, Type &&))
  {
    if (this == &source) {
      return *this;
    }

    clear();
    if (source.state == State::Value) {
      construct(static_cast<Type&&>(source.value));
    }
    return *this;
  }

  template <typename NoneVisitor, typename ValueVisitor>
  constexpr auto visit(NoneVisitor none_visitor, ValueVisitor value_visitor)
      -> decltype(auto) {
    if (state == State::None) {
      return none_visitor(none);
    }

    return value_visitor(value);
  }

  template <typename NoneVisitor, typename ValueVisitor>
  constexpr auto visit(NoneVisitor none_visitor, ValueVisitor value_visitor)
      const -> decltype(auto) {
    if (state == State::None) {
      return none_visitor(none);
    }

    return value_visitor(value);
  }
};

// A reference Option borrows its selected object and remains pointer-sized. A
// const Option does not change the referent's type because constness belongs to
// the reference declared at the API boundary.
template <typename Type>
class Option<Type&> {
 public:
  constexpr Option() = default;
  constexpr Option(const None&) {}
  constexpr Option(Type& value) : value(&value) {}

  // A const reference can otherwise bind a temporary and leave Option holding
  // a dangling borrow. Requiring an lvalue makes the lifetime decision visible
  // at construction.
  Option(Type&&) = delete;

  template <typename NoneVisitor, typename ReferenceVisitor>
  constexpr auto visit(
      NoneVisitor none_visitor,
      ReferenceVisitor reference_visitor) const -> decltype(auto) {
    if (value == nullptr) {
      return none_visitor(none);
    }

    return reference_visitor(*value);
  }

 private:
  Type* value = nullptr;
};

}  // namespace Perimortem::Utility
