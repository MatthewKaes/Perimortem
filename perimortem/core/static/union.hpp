// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Static {

// A tagged union type that allows null tagging to represent no value.
// Each possible type must be unique and values are managed using byte
// laundering so destructable types aren't supported.
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

  static consteval auto storage_size() -> Count {
    Count size = 0;
    ((size = size < sizeof(Types) ? sizeof(Types) : size), ...);
    return size;
  }

  static consteval auto storage_alignment() -> Count {
    Count alignment = 0;
    ((alignment = alignment < alignof(Types) ? alignof(Types) : alignment),
     ...);
    return alignment;
  }

  template <typename Type>
  static consteval auto type_tag() -> Bits_8 {
    Bits_8 result = 0;
    Bits_8 candidate = 1;
    ((result = __is_same(Type, Types) ? candidate : result, candidate++), ...);
    return result;
  }

  template <typename Type>
  using Alternative = __remove_cvref(Type);

  template <typename Candidate>
  static consteval auto constructible_count() -> Count {
    return (Count(__is_constructible(Types, Candidate&&)) + ...);
  }

  template <typename Candidate, typename Type>
  static consteval auto selects() -> bool {
    // An exact alternative always wins. Otherwise the source must construct
    // exactly one alternative, allowing `Union<Bits_64>` to accept an integer
    // literal without making a multi-numeric Union guess its intended type.
    constexpr Count exact = type_count<Alternative<Candidate>>();
    if constexpr (exact != 0) {
      return __is_same(Alternative<Candidate>, Type) &&
             __is_constructible(Type, Candidate&&);
    }

    return constructible_count<Candidate>() == 1 &&
           __is_constructible(Type, Candidate&&);
  }

  template <typename Candidate>
  static consteval auto accepts() -> bool {
    return (Count(selects<Candidate, Types>()) + ...) == 1;
  }

  template <typename Type, typename... Arguments>
  auto construct(Arguments&&... arguments) -> Type& {
    Type& value = *new (storage) Type(static_cast<Arguments&&>(arguments)...);
    tag = type_tag<Type>();
    return value;
  }

  template <typename Type, typename... Rest, typename Candidate>
  auto construct_candidate(Candidate&& value) -> void {
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
  auto active() -> Type& {
    return *Data::cast<Type>(storage);
  }

  template <typename Type>
  auto active() const -> const Type& {
    return *Data::cast<const Type>(storage);
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

  alignas(storage_alignment()) Bits_8 storage[storage_size()];
  Bits_8 tag = 0;

 public:
  constexpr Union() = default;

  template <typename Candidate>
    requires(accepts<Candidate>())
  Union(Candidate&& value) {
    construct_candidate<Types...>(static_cast<Candidate&&>(value));
  }

  constexpr Union(const Union& source) {
    dispatch(
        source, []() {},
        [this](const auto& value) -> void {
          this->template construct<Alternative<decltype(value)>>(value);
        });
  }

  constexpr Union(Union&& source) {
    dispatch(
        source, []() {},
        [this](auto& value) -> void {
          this->template construct<Alternative<decltype(value)>>(
              Data::take(value));
        });
  }

  constexpr auto operator=(const Union& source) -> Union& {
    if (this == &source) {
      return *this;
    }

    tag = 0;
    dispatch(
        source, []() {},
        [this](const auto& value) -> void {
          this->template construct<Alternative<decltype(value)>>(value);
        });
    return *this;
  }

  constexpr auto operator=(Union&& source) -> Union& {
    if (this == &source) {
      return *this;
    }

    tag = 0;
    dispatch(
        source, []() {},
        [this](auto& value) -> void {
          this->template construct<Alternative<decltype(value)>>(
              Data::take(value));
        });
    return *this;
  }

  constexpr auto operator==(const Union& rhs) const -> Bool {
    if (tag != rhs.tag) {
      return False;
    }

    return dispatch(
        *this, []() { return True; },
        [&rhs](const auto& value) {
          using Type = Alternative<decltype(value)>;
          return Bool(value == rhs.template active<Type>());
        });
  }

  constexpr auto operator!=(const Union& rhs) const -> Bool {
    return !(*this == rhs);
  }

  template <typename Type>
  constexpr auto find() const -> const Type* {
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
