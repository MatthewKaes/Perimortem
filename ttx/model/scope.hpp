// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Model {

// Scope composes two existing Abstract contexts without acquiring their data.
// It asks the local context first and forwards the unchanged borrowed route to
// the outer context only when local resolution returns Invalid.
//
// Scope is a transient resolution edge, not durable containment or a second
// child index. Local and outer contexts may use maps, tables, direct parsing,
// or any other resolution policy. The composing owner guarantees that Invalid
// means the route has no local selection. Ambiguous or corrupt local contexts
// are rejected before they become a Scope. Both owners retain their storage and
// lifetime.
class Scope final : public Concept::Abstract {
 public:
  using ContractOwner = Scope;
  static constexpr Perimortem::System::Uuid contract_id{
    0x0e2186cbd28f434a,
    0x97769b1fe9cf5a01,
  };

  Scope(const Concept::Abstract& local, const Concept::Abstract& outer);

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Concept::Abstract& override;

 private:
  const Concept::Abstract& local;
  const Concept::Abstract& outer;
};

}  // namespace Ttx::Model
