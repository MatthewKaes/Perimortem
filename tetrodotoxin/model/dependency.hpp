// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/dialect.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/exports.hpp"

namespace Tetrodotoxin::Model {

// Dependency is the open contract for one resolved import instruction. It
// retains the root Dialect used to interpret the dependency and owns the Alias
// that binds its resulting Exports surface into the importing Source. Alias is
// the closed binding contract shared by every dependency, so locator subtypes
// supply its authored facts rather than virtualizing or reimplementing the
// binding.
//
// How the resolver finds the producer belongs to a narrower Dependency
// contract such as Dependencies::Source or Dependencies::Package. There is no
// kind tag: other loading systems can add contracts without extending a closed
// classification here.
//
// Dependency itself has no documentation. Authored import documentation
// belongs to the bound Alias, which is the named semantic object consumers see
// through Source resolution.
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

}  // namespace Tetrodotoxin::Model
