// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Language {

// Binding gives an authored language name and documentation to one selected
// graph identity. It remains visible so source tools can observe the spelling,
// then resolve() reaches the selected context. This is intentionally different
// from TTX Alias, whose indirection cannot be observed or negotiated.
//
// Import, Reference, and concrete definitional aliases specialize how the
// target is selected. A staged binding begins at Unknown and accepts one target
// from its source transaction; later callers re-resolve the same language owner
// instead of retaining a copied answer.
class Binding : public Ttx::Concept::Abstract {
 public:

  Binding(
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Abstract& target);

  Binding(
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Abstract& target,
      const Ttx::Concept::Documentation& documentation);

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto resolve() const -> const Abstract& override {
    if (!target) {
      return Ttx::Concept::Unknown::get_unknown();
    }

    return (**target).visit<Binding>(
        [](const Binding& binding) -> const Ttx::Concept::Abstract& {
          return binding.resolve();
        },
        [](const Ttx::Concept::Abstract& direct)
            -> const Ttx::Concept::Abstract& { return direct; });
  }

  // The authored name selects a target but does not inherit that target's
  // contextual routes. A consumer resolves the Binding before directing a
  // question to the selected owner.
  constexpr auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Abstract& override {
    return target ? static_cast<const Abstract&>(Ttx::Concept::None::get_none())
                  : static_cast<const Abstract&>(
                        Ttx::Concept::Unknown::get_unknown());
  }

 protected:
  Binding(
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation)
      : name(name), documentation(documentation) {}

  constexpr auto bind_target(const Ttx::Concept::Abstract& selected) -> Bool {
    if (target) {
      return *target == &selected;
    }

    target = &selected;
    return True;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<const Ttx::Concept::Abstract*> target;
  const Ttx::Concept::Documentation& documentation;
};

}  // namespace Tetrodotoxin::Language
