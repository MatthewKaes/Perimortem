// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/source/dialect.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/exports.hpp"

namespace Tetrodotoxin::Model::Source {

// Dependency is the contract for one resolved external graph edge. It retains
// the root Dialect associated with the edge and owns the Alias that binds its
// resulting Exports surface into an Environment. Alias remains the closed
// binding contract, while the concrete edge retains durable resolver facts.
//
// Dependency itself has no documentation. Authored resolution documentation
// belongs to the bound Alias, which is the named semantic object consumers see
// through Environment resolution.
class Dependency : public Ttx::Concept::Abstract {
 public:
  using ContractOwner = Dependency;
  static constexpr Perimortem::System::Uuid contract_id{
    0xcbb8c4ccf2544d67,
    0xb9caf93a5453c9de,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return binding.get_name();
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_root_dialect() const -> const Dialect& {
    return root_dialect;
  }

  constexpr auto get_binding() const -> const Ttx::Model::Alias& {
    return binding;
  }

 protected:
  constexpr Dependency(
      const Dialect& root_dialect,
      Perimortem::Core::View::Bytes local_name,
      const Ttx::Model::Exports& target,
      const Ttx::Concept::Documentation& documentation)
      : root_dialect(root_dialect),
        binding(local_name, target, documentation) {}

 private:
  const Dialect& root_dialect;
  Ttx::Model::Alias binding;
};

}  // namespace Tetrodotoxin::Model::Source
