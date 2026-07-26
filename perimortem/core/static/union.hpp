// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Static {

// A tagged union type that allows null tagging to represent no value.
// Each possible type must be unique. Value alternatives are managed using byte
// laundering, while reference alternatives store one non-owning pointer and
// preserve the referred object's identity. A reference can only be constructed
// from an lvalue, so the Union cannot retain a temporary through const binding.
// Destructable alternatives aren't supported.
template <typename... Types>
class Union {
 private:
  static_assert(
      sizeof...(Types) != 0,
      "Union requires at least one provided type.");
  static_assert(
      sizeof...(Types) < 255,
      "Union supports at most 254 types plus null.");

  template <typename Type>
  static consteval auto type_count() -> Count {
    return (Count(__is_same(Type, Types)) + ...);
  }

  static_assert(
      ((type_count<Types>() == 1) && ...),
      "All provided Union types must be unique.");
  static_assert(
      (__is_trivially_destructible(Types) && ...),
      "All provided Union types must be trivially destructible.");

  template <typename Type>
  static auto storage_type() -> Type;

  template <typename Type>
    requires(__is_lvalue_reference(Type))
  static auto storage_type() -> __remove_reference_t(Type)*;

  template <typename Type>
  using Storage = decltype(storage_type<Type>());

  static consteval auto storage_size() -> Count {
    Count size = 0;
    ((size = size < sizeof(Storage<Types>) ? sizeof(Storage<Types>) : size),
     ...);
    return size;
  }

  static consteval auto storage_alignment() -> Count {
    Count alignment = 0;
    ((alignment = alignment < alignof(Storage<Types>) ? alignof(Storage<Types>)
                                                      : alignment),
     ...);
    return alignment;
  }

  template <typename Type>
  static consteval auto type_tag() -> Unsigned_8 {
    Unsigned_8 result = 0;
    Unsigned_8 candidate = 1;
    ((result = __is_same(Type, Types) ? candidate : result, candidate++), ...);
    return result;
  }

  template <typename Type>
  using Alternative = __remove_cvref(Type);

  template <typename Candidate, typename Type>
  static consteval auto constructible() -> bool {
    return (!__is_lvalue_reference(Type) || __is_lvalue_reference(Candidate)) &&
           __is_constructible(Type, Candidate&&);
  }

  template <typename Candidate>
  static consteval auto constructible_count() -> Count {
    return (Count(constructible<Candidate, Types>()) + ...);
  }

  template <typename Candidate, typename Type>
  static consteval auto selects() -> bool {
    // An exact alternative always wins. Otherwise the source must construct
    // exactly one alternative, allowing `Union<Unsigned_64>` to accept an
    // integer literal without making a multi-numeric Union guess its intended
    // type.
    constexpr Count exact_reference = type_count<Candidate>();
    if constexpr (exact_reference != 0) {
      return __is_same(Candidate, Type) && constructible<Candidate, Type>();
    }

    constexpr Count exact_value = type_count<Alternative<Candidate>>();
    if constexpr (exact_value != 0) {
      return __is_same(Alternative<Candidate>, Type) &&
             constructible<Candidate, Type>();
    }

    return constructible_count<Candidate>() == 1 &&
           constructible<Candidate, Type>();
  }

  template <typename Candidate>
  static consteval auto accepts() -> bool {
    return (Count(selects<Candidate, Types>()) + ...) == 1;
  }

  template <typename Type, typename Candidate>
  constexpr auto construct(Candidate&& candidate) -> decltype(auto) {
    if constexpr (__is_lvalue_reference(Type)) {
      auto& reference = static_cast<Type>(candidate);
      new (storage) Storage<Type>(&reference);
      tag = type_tag<Type>();
      return reference;
    } else {
      Type& value = *new (storage) Type(static_cast<Candidate&&>(candidate));
      tag = type_tag<Type>();
      return value;
    }
  }

  template <typename Type, typename... Rest, typename Candidate>
  constexpr auto construct_candidate(Candidate&& value) -> void {
    if constexpr (selects<Candidate, Type>()) {
      construct<Type>(static_cast<Candidate&&>(value));
    } else if constexpr (sizeof...(Rest) != 0) {
      construct_candidate<Rest...>(static_cast<Candidate&&>(value));
    } else {
      static_assert(
          selects<Candidate, Type>(),
          "Union construction candidate does not select a supported type.");
    }
  }

  template <typename Type>
  constexpr auto active() -> decltype(auto) {
    if constexpr (__is_lvalue_reference(Type)) {
      return **Data::cast<Storage<Type>>(storage);
    } else {
      return *Data::cast<Type>(storage);
    }
  }

  template <typename Type>
  constexpr auto active() const -> decltype(auto) {
    if constexpr (__is_lvalue_reference(Type)) {
      return **Data::cast<Storage<Type>>(storage);
    } else {
      return *Data::cast<const Type>(storage);
    }
  }

  template <typename Type, typename... Rest, typename Source>
  constexpr auto construct_active(Source& source) -> void {
    if (source.tag == type_tag<Type>()) {
      if constexpr (
          __is_same(Source, const Union) || __is_lvalue_reference(Type)) {
        construct<Type>(source.template active<Type>());
      } else {
        construct<Type>(Data::take(source.template active<Type>()));
      }
      return;
    }

    if constexpr (sizeof...(Rest) != 0) {
      construct_active<Rest...>(source);
    }
  }

  template <typename Type, typename... Rest>
  constexpr auto equals_active(const Union& rhs) const -> Bool {
    if (tag == type_tag<Type>()) {
      if constexpr (__is_lvalue_reference(Type)) {
        return &active<Type>() == &rhs.template active<Type>();
      } else {
        return active<Type>() == rhs.template active<Type>();
      }
    }

    if constexpr (sizeof...(Rest) != 0) {
      return equals_active<Rest...>(rhs);
    }

    __builtin_unreachable();
  }

  template <typename Type, typename... Rest, typename Self, typename Visitor>
  constexpr static auto visit_active(Self& self, Visitor& visitor)
      -> decltype(auto) {
    if (self.tag == type_tag<Type>()) {
      return visitor(self.template active<Type>());
    }

    if constexpr (sizeof...(Rest) != 0) {
      return visit_active<Rest...>(self, visitor);
    }

    __builtin_unreachable();
  }

  template <typename Self, typename... Cases>
  constexpr static auto dispatch(Self& self, Cases... cases) -> decltype(auto) {
    struct Visitor : Cases... {
      using Cases::operator()...;
    } visitor{static_cast<Cases&&>(cases)...};
    if (self.is_null()) {
      return visitor();
    }

    return visit_active<Types...>(self, visitor);
  }

  alignas(storage_alignment()) Unsigned_8 storage[storage_size()];
  Unsigned_8 tag = 0;

 public:
  constexpr Union() = default;

  template <typename Candidate>
    requires(accepts<Candidate>())
  constexpr Union(Candidate&& value) {
    construct_candidate<Types...>(static_cast<Candidate&&>(value));
  }

  constexpr Union(const Union& source) {
    if (!source.is_null()) {
      construct_active<Types...>(source);
    }
  }

  constexpr Union(Union&& source) {
    if (!source.is_null()) {
      construct_active<Types...>(source);
    }
  }

  constexpr auto operator=(const Union& source) -> Union& {
    if (this == &source) {
      return *this;
    }

    tag = 0;
    if (!source.is_null()) {
      construct_active<Types...>(source);
    }
    return *this;
  }

  constexpr auto operator=(Union&& source) -> Union& {
    if (this == &source) {
      return *this;
    }

    tag = 0;
    if (!source.is_null()) {
      construct_active<Types...>(source);
    }
    return *this;
  }

  constexpr auto operator==(const Union& rhs) const -> Bool {
    if (tag != rhs.tag) {
      return False;
    }

    return is_null() ? True : equals_active<Types...>(rhs);
  }

  constexpr auto operator!=(const Union& rhs) const -> Bool {
    return !(*this == rhs);
  }

  template <typename Type>
  constexpr auto find() const {
    static_assert(type_count<Type>() == 1, "Type is not a Union alternative.");
    return tag == type_tag<Type>() ? &active<Type>() : nullptr;
  }

  template <typename... Cases>
  constexpr auto visit(Cases... cases) const -> decltype(auto) {
    return dispatch(*this, static_cast<Cases&&>(cases)...);
  }

  constexpr auto is_null() const -> Bool { return tag == 0; }
};

}  // namespace Perimortem::Core::Static
