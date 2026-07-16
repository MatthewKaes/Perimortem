// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/abstraction/abstract.hpp"

namespace Ttx::Abstraction {

// Alias is the closed named-redirection concept in the TTX graph. It is not a
// Type, owner, container, documentation record, or resolution result. A source
// declaration may attach those facts beside an Alias, but Alias itself only
// preserves the local name and borrows its target.
//
// `get_name()` returns the local alias name. `resolve()` follows the target to
// its represented identity. `resolve_context(route)` first resolves that target
// and then gives it the complete borrowed route, so the target owns all
// parsing, indexing, slicing, and further redirection.
//
// The graph owner must keep the target alive and reject alias cycles before the
// Alias becomes queryable. Alias should gain no additional concepts unless the
// fundamental Abstract contract changes and every Alias must reflect it.
class Alias final : public Abstract {
 public:
  using ContractOwner = Alias;
  static constexpr Perimortem::System::Uuid contract_id{
    0x58e8b4a115e9461a,
    0x88a5767626d4a21a,
  };

  Alias(Perimortem::Core::View::Bytes name, const Abstract& target)
      : name(name), target(target) {}

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  auto resolve() const -> const Abstract& override { return target.resolve(); }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Abstract& override {
    return target.resolve().resolve_context(route);
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Abstract& target;
};

}  // namespace Ttx::Abstraction
