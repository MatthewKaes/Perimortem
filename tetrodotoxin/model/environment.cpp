// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/environment.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Environment::resolve(
    const Dialect& root_dialect,
    View::Bytes local_name,
    View::Bytes package_name,
    Version version,
    const Package& package,
    const Documentation& documentation) -> Bool {
  if (local_name.is_empty() || package_name.is_empty() || version.is_null() ||
      bindings_by_name.find(local_name) != nullptr) {
    return False;
  }

  const auto* identity =
      dependencies_by_identity.find(Identity(package_name, version));
  const auto* selected_package = dependencies_by_package.find(&package);
  if ((identity == nullptr) != (selected_package == nullptr) ||
      (identity != nullptr && identity->value != selected_package->value)) {
    return False;
  }
  Bool package_present = identity != nullptr;

  const auto& resolution = arena.construct<Dependencies::Package>(
      root_dialect, arena.proxy(package_name), version, arena.proxy(local_name),
      package, documentation);
  resolutions.insert(Reference<Dependencies::Package>(resolution));
  bindings_by_name.insert(
      resolution.get_name(),
      Reference<Ttx::Model::Alias>(resolution.get_binding()));
  if (!package_present) {
    Count dependency_index = dependencies.get_size();
    dependencies.insert(Reference<Dependencies::Package>(resolution));
    packages.insert(Reference<Package>(package));
    dependencies_by_identity.insert(
        Identity(resolution.get_package_name(), resolution.get_version()),
        dependency_index);
    dependencies_by_package.insert(&package, dependency_index);
  }

  return True;
}

auto Tetrodotoxin::Model::Environment::bind(
    View::Bytes local_name,
    const Abstract& target,
    const Documentation& documentation) -> Bool {
  if (local_name.is_empty() || target.is<Invalid>() ||
      bindings_by_name.find(local_name) != nullptr) {
    return False;
  }

  View::Bytes owned_name = arena.proxy(local_name);
  const auto& binding =
      arena.construct<Ttx::Model::Alias>(owned_name, target, documentation);
  bindings_by_name.insert(owned_name, Reference<Ttx::Model::Alias>(binding));
  return True;
}

auto Tetrodotoxin::Model::Environment::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Bindings::Entry* selected = bindings_by_name.find(route);
  if (selected == nullptr) {
    return Invalid::get_invalid();
  }

  return selected->value.get();
}
