// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/member.hpp"

#include "ttx/type.hpp"

auto Ttx::Member::equivalent_to(const Member& other) const -> Bool {
  return type->equivalent_to(*other.type);
}
