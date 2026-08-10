// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Model {

// Alias is the closed named-redirection method in the TTX graph. It preserves
// the authored local name and documentation while borrowing the Abstract it
// redirects to. It is not a Type, owner, container, or resolution result.
//
// `get_name()` returns the local alias name. `resolve()` follows the target to
// its represented identity. `resolve_context(route)` first resolves that target
// and then gives it the complete borrowed route, so the target owns all
// parsing, indexing, slicing, and further redirection.
//
// When constructing an Alias `Documentations::Merged` is the preferred model as
// it enables forwarding of the target documentation, however for optimization
// if the Alias provides no wrapping documentation then the target's
// documentation can be passed to the Alias directly without a `Merged`
// wrapper.
//
// The graph owner must keep the target alive and reject alias cycles before the
// Alias becomes queryable.
class Alias : public Concept::Abstract {
 public:
  TTX_CONTRACT(Alias, Abstract, 0x58e8b4a115e9461a, 0x88a5767626d4a21a);

  constexpr Alias(
      Perimortem::Core::View::Bytes name,
      const Concept::Abstract& target)
      : Alias(name, target, target.get_documentation()) {}

  constexpr Alias(
      Perimortem::Core::View::Bytes name,
      const Concept::Abstract& target,
      const Concept::Documentation& documentation)
      : name(name), target(target), documentation(documentation) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  // Local prose leads the target's visible documentation. Because the target
  // may itself be an Alias, this naturally accumulates the complete authored
  // chain without changing identity resolution.
  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto resolve() const -> const Abstract& override {
    return target.resolve();
  }

  // The immediate target is the exact borrowed graph edge. Unlike resolve this
  // observation neither follows another Alias nor asks whether the target has
  // completed its own semantic contract.
  constexpr auto get_target() const -> const Abstract& { return target; }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Abstract& override {
    return target.resolve().resolve_context(route);
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Abstract& target;
  const Concept::Documentation& documentation;
};

}  // namespace Ttx::Model
