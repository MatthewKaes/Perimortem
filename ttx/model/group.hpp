// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"

namespace Ttx::Model {

// Group is a durable named context over an ordered set of Abstracts. It gives
// containment one shared TTX representation without turning every collection
// into a Type, Layout, Pack, or interpreter-specific resolver.
//
// Group resolves one borrowed route against its direct children. Child names
// are unique in a valid graph. A missing or ambiguous name returns the global
// Invalid object. The graph owner retains storage and rejects duplicate names
// before publishing a Group.
class Group final : public Concept::Abstract {
 public:
  using ContractOwner = Group;
  static constexpr Perimortem::System::Uuid contract_id{
    0x7987566b34c24442,
    0xaf18910bfe5fb8c7,
  };

  Group(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
          abstracts,
      Concept::Documentation documentation = Concept::Documentation());

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto get_abstracts() const
      -> Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>;

  auto get_documentation() const -> Concept::Documentation;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Concept::Abstract& override;

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
      abstracts;
  Concept::Documentation documentation;
};

}  // namespace Ttx::Model
