// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/packages/sources.hpp"

#include "perimortem/core/algorithm/sort.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

static auto precedes(View::Bytes left, View::Bytes right) -> Bool {
  Count shared = Math::min(left.get_size(), right.get_size());
  for (Count i = 0; i < shared; i++) {
    if (left[i] != right[i]) {
      return left[i] < right[i];
    }
  }

  return left.get_size() < right.get_size();
}

class DefinitionOrder {
 public:
  DefinitionOrder() = default;
  DefinitionOrder(const Abstract& definition) : definition(&definition) {}

  auto operator>(const DefinitionOrder& other) const -> Bool {
    return precedes(other.definition->get_name(), definition->get_name());
  }

  constexpr auto get() const -> const Abstract& { return *definition; }

 private:
  const Abstract* definition = nullptr;
};

auto Tetrodotoxin::Model::Packages::Sources::construct(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Vector<Reference<Model::Source>> sources,
    const Model::Namespace& exports) -> const Abstract& {
  if (sources.is_empty()) {
    return Invalid::get_invalid();
  }

  Sources* package = arena.reserve<Sources>();
  new (package) Sources(arena, sources, exports);
  if (!package->valid) {
    return Invalid::get_invalid();
  }

  return *package;
}

Tetrodotoxin::Model::Packages::Sources::Sources(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Vector<Reference<Model::Source>> members,
    const Model::Namespace& exports)
    : exports(exports),
      sources(arena),
      source_index(arena),
      dependencies(arena),
      definitions(arena),
      definition_index(arena),
      pending_namespaces(arena) {
  const Model::Environment& environment = members[0].get().get_environment();
  sources.reset(members.get_size());
  source_index.ensure_capacity(members.get_size());
  for (Count i = 0; i < members.get_size(); i++) {
    const Model::Source& source = members[i].get();
    if (&source.get_environment() != &environment ||
        source_index.find(&source) != nullptr) {
      valid = False;
      return;
    }

    source_index.insert(&source, True);
    sources.insert(members[i]);
  }

  const View::Vector<Reference<Model::Package>> packages =
      environment.get_packages();
  dependencies.reset(packages.get_size());
  for (Count i = 0; i < packages.get_size(); i++) {
    dependencies.insert(packages[i]);
  }

  collect_namespace(exports);
  for (Count i = 0; i < pending_namespaces.get_size(); i++) {
    collect_namespace(pending_namespaces[i].get());
  }
  pending_namespaces.clear();
}

auto Tetrodotoxin::Model::Packages::Sources::collect_namespace(
    const Model::Namespace& namespace_object) -> void {
  // Definition IDs are canonical package coordinates, so authored definition
  // order cannot renumber an otherwise identical graph. Each retained scope is
  // traversed by name while the public Exports order remains untouched.
  Count root_count = namespace_object.get_root_count();
  auto* ordered = Data::cast<DefinitionOrder>(
      definitions.get_arena().allocate(sizeof(DefinitionOrder) * root_count));
  Count ordered_count = 0;
  for (Count i = 0; i < root_count; i++) {
    const Abstract& candidate = namespace_object.get_root(i);
    if (definition_index.find(&candidate) == nullptr) {
      new (ordered + ordered_count++) DefinitionOrder(candidate);
    }
  }

  auto sorted =
      Algorithm::sort(Access::Vector<DefinitionOrder>(ordered, ordered_count));
  for (Count i = 0; i < sorted.get_size(); i++) {
    collect_definition(sorted[i].get());
  }
}

auto Tetrodotoxin::Model::Packages::Sources::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return exports.resolve_context(route);
}

auto Tetrodotoxin::Model::Packages::Sources::get_definition(Count id) const
    -> const Abstract& {
  if (id >= definitions.get_size()) {
    return Invalid::get_invalid();
  }

  return definitions.get_view()[id].get();
}

auto Tetrodotoxin::Model::Packages::Sources::get_definition_id(
    const Abstract& definition) const -> Count {
  const DefinitionIndex::Entry* selected = definition_index.find(&definition);
  return selected == nullptr ? Count(-1) : selected->value;
}

auto Tetrodotoxin::Model::Packages::Sources::collect_definition(
    const Abstract& definition) -> void {
  if (definition_index.find(&definition) != nullptr) {
    return;
  }

  definition_index.insert(&definition, definitions.get_size());
  definitions.insert(Reference<Abstract>(definition));
  if (definition.is<Model::Namespace>()) {
    pending_namespaces.insert(
        Reference<Model::Namespace>(definition.assume<Model::Namespace>()));
    return;
  }

  if (!definition.is<Ttx::Model::Alias>()) {
    return;
  }

  const Abstract& target = definition.resolve();
  for (Count i = 0; i < dependencies.get_size(); i++) {
    const Model::Package& package = dependencies[i].get();
    if (&target == &package || package.get_definition_id(target) != Count(-1)) {
      return;
    }
  }

  // The current archive persists Namespace and Alias definitions. Following a
  // local Namespace target makes private source-file roots addressable by a
  // stable package ID even when no public path reaches that Namespace.
  if (target.is<Model::Namespace>()) {
    collect_definition(target);
  }
}
