// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Library needs to decide whether one authored context may observe a private
// declaration and whether a public declaration can name another value domain.
// Keeping those questions on a separate owner lets generated and nested Types
// reuse the same graph identities without inheriting visibility as part of the
// host-neutral Domain relationship.
class Visibility : public Ttx::Concept::Abstract {
 public:
  TTX_NAME("visibility"_view);
  TTX_EMPTY_DOCUMENTATION();

  virtual auto grants_private_access_to(
      const Ttx::Concept::Abstract& owner) const -> Bool = 0;
  virtual auto exposes(const Ttx::Concept::Abstract& candidate) const
      -> Bool = 0;
};

// A generated Type has no enclosing authored scope. It grants private access
// only to itself and can expose only a route that resolves back to the exact
// candidate, avoiding a synthetic parent or publication registry.
class ExactVisibility final : public Visibility {
 public:
  constexpr explicit ExactVisibility(const Ttx::Concept::Abstract& owner)
      : owner(owner) {}

  auto grants_private_access_to(const Ttx::Concept::Abstract& candidate) const
      -> Bool override;
  auto exposes(const Ttx::Concept::Abstract& candidate) const -> Bool override;

 private:
  const Ttx::Concept::Abstract& owner;
};

auto visibility(const Ttx::Concept::Abstract& candidate)
    -> Perimortem::Core::Option<const Visibility&>;
auto has_private_access_to(
    const Ttx::Concept::Abstract& candidate,
    const Ttx::Concept::Abstract& owner) -> Bool;
auto is_externally_reachable(
    const Ttx::Concept::Abstract& context,
    const Ttx::Concept::Abstract& candidate) -> Bool;

}  // namespace Tetrodotoxin::Library::Language::Model
