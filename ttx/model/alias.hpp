// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/none.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/concept/unknown.hpp"

namespace Ttx::Model {

// Alias gives one authored local name and its Documentation to an existing
// semantic identity. Resolution follows an Alias chain to the first concrete
// identity, where the consumer can use that owner's real contract.
//
// Local Documentation leads the target Documentation through Merged. When the
// Alias adds no local prose, it can borrow the target Documentation directly
// and avoid an empty wrapper.
//
// Some declaration owners reserve an Alias before its target has completed.
// They bind that target once during graph construction and keep it alive beside
// the Alias. Repeating the same binding is harmless, while a changed target or
// a cycle makes publication fail.
class Alias : public Concept::Abstract {
 public:
  TTX_CONTRACT(Alias, Abstract);

  constexpr Alias(
      Perimortem::Core::View::Bytes name,
      const Concept::Abstract& target)
      : Alias(name, target, target.get_documentation()) {}

  constexpr Alias(
      Perimortem::Core::View::Bytes name,
      const Concept::Abstract& target,
      const Concept::Documentation& documentation)
      : name(name),
        target(Concept::Reference<const Concept::Abstract>(target)),
        documentation(documentation) {}

  TTX_NAME(name);

  // Local prose leads the target's visible documentation. Because the target
  // may itself be an Alias, this naturally accumulates the complete authored
  // chain without changing identity resolution.
  TTX_DOCUMENTATION(documentation);

  // Preserve every Alias on the way to the implementation. In particular an
  // Import can answer a boundary question even when ordinary resolution would
  // lead through it to the acquired Library.
  auto bind_interface(U64 requested) const
      -> Perimortem::Utility::Result<Concept::Binding,
                                     Concept::Binding::Failure> override {
    if (!target) {
      return Concept::Binding::Failure::Pending;
    }

    return target->get().bind_interface(requested);
  }

  constexpr auto resolve() const -> const Abstract& override {
    if (!target) {
      return Concept::Unknown::get_unknown();
    }

    return target->get().visit<Alias>(
        [](const Alias& alias) -> const Concept::Abstract& {
          return alias.resolve();
        },
        [](const Concept::Abstract& direct) -> const Concept::Abstract& {
          return direct;
        });
  }

  // Context belongs to the selected target rather than its local name.
  // Resolving the Alias first makes that ownership visible to the consumer.
  constexpr auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Abstract& override {
    return target
               ? static_cast<const Abstract&>(Concept::None::get_none())
               : static_cast<const Abstract&>(Concept::Unknown::get_unknown());
  }

 protected:
  // A declaration owner can use staged construction when its target settles in
  // a later definition pass. Callers that already know the target use the
  // complete public constructors.
  constexpr Alias(
      Perimortem::Core::View::Bytes name,
      const Concept::Documentation& documentation)
      : name(name), documentation(documentation) {}

  constexpr auto bind_target(const Concept::Abstract& selected) -> Bool {
    if (target) {
      return &target->get() == &selected;
    }

    target = Concept::Reference<const Concept::Abstract>(selected);
    return True;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<Concept::Reference<const Concept::Abstract>> target;
  const Concept::Documentation& documentation;
};

}  // namespace Ttx::Model
