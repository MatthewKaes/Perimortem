// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"

namespace Ttx::Concept {

// Alias is the closed named-redirection concept in the TTX graph. It preserves
// the authored local name and documentation while borrowing the Abstract it
// redirects to. It is not a Type, owner, container, or resolution result.
//
// `get_name()` returns the local alias name. `resolve()` follows the target to
// its represented identity. `resolve_context(route)` first resolves that target
// and then gives it the complete borrowed route, so the target owns all
// parsing, indexing, slicing, and further redirection.
//
// Documentation remains local. It explains why this Alias exists without
// changing the target's identity or prose. Tools may inspect it before
// resolution and choose whether to show local, resolved, or stacked context.
//
// The graph owner must keep the target alive and reject alias cycles before the
// Alias becomes queryable. Name, target, and documentation are the complete
// Alias state. Adding another fact is an architectural change to this closed
// concept and needs a corresponding change to the canonical semantics.
class Alias final : public Abstract {
 public:
  using ContractOwner = Alias;
  static constexpr Perimortem::System::Uuid contract_id{
    0x58e8b4a115e9461a,
    0x88a5767626d4a21a,
  };

  Alias(
      Perimortem::Core::View::Bytes name,
      const Abstract& target,
      Documentation documentation = Documentation())
      : name(name), target(target), documentation(documentation) {}

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }

  auto resolve() const -> const Abstract& override { return target.resolve(); }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Abstract& override {
    return target.resolve().resolve_context(route);
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Abstract& target;
  Documentation documentation;
};

}  // namespace Ttx::Concept
