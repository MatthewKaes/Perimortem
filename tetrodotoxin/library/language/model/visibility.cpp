// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/visibility.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

auto Model::ExactVisibility::grants_private_access_to(
    const Abstract& candidate) const -> Bool {
  return Bool(&owner == &candidate);
}

auto Model::ExactVisibility::exposes(const Abstract& candidate) const -> Bool {
  const Abstract& selected = owner.resolve_concept(candidate.get_name());
  return Bool(
      !selected.is<Unknown>() && !selected.is<None>() &&
      &selected.resolve() == &candidate);
}

auto Model::visibility(const Abstract& candidate)
    -> Perimortem::Core::Option<const Model::Visibility&> {
  return candidate.resolve_concept("visibility"_view)
      .select<Model::Visibility>();
}

auto Model::has_private_access_to(
    const Abstract& candidate,
    const Abstract& owner) -> Bool {
  auto policy = visibility(candidate);
  return policy && policy->grants_private_access_to(owner);
}

auto Model::is_externally_reachable(
    const Abstract& context,
    const Abstract& candidate) -> Bool {
  auto policy = visibility(context);
  return policy && policy->exposes(candidate);
}
