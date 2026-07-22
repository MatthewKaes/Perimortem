// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

namespace Perimortem::Utility {

// None is the explicit unit value carried by an Option with no reference.
class None final {};

inline constexpr None none;

// Option represents either one borrowed object or None in one pointer-sized
// value. Its template argument must be an lvalue reference so ownership and
// borrowing remain visible at every API boundary.
//
// Observation is deliberately restricted to visit(). Option does not expose a
// pointer, boolean test, fallback object, or unchecked accessor that could turn
// absence back into nullable control flow. The selected callback receives the
// original reference, while the absent callback receives the explicit None
// object.
template <typename Reference>
class Option {
  static_assert(
      __is_lvalue_reference(Reference),
      "Option requires an lvalue reference type such as `const Type&`.");
};

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
