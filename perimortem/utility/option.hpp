// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Utility {

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
  template <typename Candidate>
  constexpr auto construct(Candidate&& candidate) -> void {
    new (&value) Type(static_cast<Candidate&&>(candidate));
    set = true;
  }

  constexpr auto clear() -> void {
    if (set) {
      value.~Type();
      set = false;
    }
  }

  union {
    Type value;
  };
  Bool set = false;

 public:
  constexpr Option() {}

  constexpr Option(const Type& selected)
    requires(__is_constructible(Type, const Type&))
      : value(selected), set(true) {}

  constexpr Option(Type&& selected)
    requires(__is_constructible(Type, Type &&))
      : value(static_cast<Type&&>(selected)), set(true) {}

  constexpr Option(const Option& source)
    requires(__is_constructible(Type, const Type&))
  {
    if (source.set) {
      construct(source.value);
    }
  }

  constexpr Option(Option&& source)
    requires(__is_constructible(Type, Type &&))
  {
    if (source.set) {
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
    if (source.set) {
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
    if (source.set) {
      construct(static_cast<Type&&>(source.value));
    }
    return *this;
  }

  operator bool() const { return bool(set); }

  template <typename RejectCallback, typename ValueVisitor>
  constexpr auto visit(
      RejectCallback reject_callback,
      ValueVisitor value_visitor) -> decltype(auto) {
    if (set) {
      return reject_callback();
    }

    return value_visitor(value);
  }

  template <typename RejectCallback, typename ValueVisitor>
  constexpr auto visit(
      RejectCallback reject_callback,
      ValueVisitor value_visitor) const -> decltype(auto) {
    if (set) {
      return reject_callback();
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
  constexpr Option(Type& value) : value(&value) {}

  // A const reference can otherwise bind a temporary and leave Option holding
  // a dangling borrow. Requiring an lvalue makes the lifetime decision visible
  // at construction.
  Option(Type&&) = delete;

  operator bool() const { return value != nullptr; }

  template <typename RejectCallback, typename ReferenceVisitor>
  constexpr auto visit(
      RejectCallback reject_callback,
      ReferenceVisitor reference_visitor) const -> decltype(auto) {
    if (value == nullptr) {
      return reject_callback();
    }

    return reference_visitor(*value);
  }

 private:
  Type* value = nullptr;
};

}  // namespace Perimortem::Utility
