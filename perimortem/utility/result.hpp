// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"

namespace Perimortem::Utility {

// Result selects exactly one value or error. Static Union supplies the compact
// tagged storage, while the missing default constructor and private state keep
// its null tag out of the public contract.
//
// Two case visitation makes error handling visible at the call site. It also
// preserves references and mutable owned values without offering an unchecked
// value shortcut that could discard the error branch.
template <typename value_type, typename error_type>
class Result {
 private:
  using Storage = Core::Static::Union<value_type, error_type>;

  Storage state;

 public:
  Result() = delete;

  template <typename candidate_type>
    requires(__is_constructible(Storage, candidate_type &&))
  constexpr Result(candidate_type&& candidate)
      : state(static_cast<candidate_type&&>(candidate)) {}

  template <typename value_callback, typename error_callback>
  constexpr auto visit(
      value_callback value_visitor,
      error_callback error_visitor) -> decltype(auto) {
    if (state.template is<value_type>()) {
      return value_visitor(*state.template find<value_type>());
    }

    return error_visitor(*state.template find<error_type>());
  }

  template <typename value_callback, typename error_callback>
  constexpr auto visit(
      value_callback value_visitor,
      error_callback error_visitor) const -> decltype(auto) {
    if (state.template is<value_type>()) {
      return value_visitor(*state.template find<value_type>());
    }

    return error_visitor(*state.template find<error_type>());
  }
};

}  // namespace Perimortem::Utility
