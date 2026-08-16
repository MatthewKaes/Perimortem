// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"

namespace Ttx::Model {

// Alias is the closed named-redirection method in the TTX graph. It preserves
// the authored local name and documentation while borrowing the Abstract it
// redirects to. It is not a Type, owner, container, or resolution result, and
// its immediate target is deliberately opaque to consumers.
//
// `get_name()` returns the local alias name. `resolve()` alone follows Alias
// edges and returns the first non-Alias identity. It does not ask that identity
// to resolve again. Alias is opaque to every other operation; a consumer
// resolves it and then invokes the selected identity's contract explicitly.
//
// When constructing an Alias `Documentations::Merged` is the preferred model as
// it enables forwarding of the target documentation, however for optimization
// if the Alias provides no wrapping documentation then the target's
// documentation can be passed to the Alias directly without a `Merged`
// wrapper.
//
// A derived graph owner may reserve an Alias identity before its target is
// known, then bind that target once during graph completion. Until then
// resolution returns Invalid. Repeating the exact binding is harmless;
// attempting to change it fails. The graph owner must keep a bound target alive
// and reject Alias cycles before publishing the completed graph.
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

  constexpr auto resolve() const -> const Abstract& override {
    if (!target) {
      return Concept::Invalid::get_invalid();
    }

    return target->get().visit<Alias>(
        [](const Alias& alias) -> const Concept::Abstract& {
          return alias.resolve();
        },
        [](const Concept::Abstract& direct) -> const Concept::Abstract& {
          return direct;
        });
  }

  // Alias never lends its target's context implicitly. The consumer resolves
  // this edge first and then invokes the selected owner's exact operation.
  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Abstract& override {
    return Concept::Invalid::get_invalid();
  }

 protected:
  // Staged construction exists only for a concrete graph owner that cannot
  // know the target before its definition pass completes. Ordinary callers
  // construct an already bound Alias through the public constructors above.
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
