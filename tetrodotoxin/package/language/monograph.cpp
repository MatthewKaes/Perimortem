// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/language/monograph.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin;

static auto is_resource_route(View::Bytes route) -> Bool {
  return route.get_size() >= 3 && route[0] == '$' && route[1] == '[' &&
         route[route.get_size() - 1] == ']';
}

auto Package::Language::Monograph::create_authored(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context,
    View::Vector<Dependency> dependencies,
    View::Vector<Source> sources) -> Option<Monograph&> {
  if (sources.is_empty()) {
    return {};
  }

  return arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        arena, language, documentation, context, dependencies, sources);
  });
}

auto Package::Language::Monograph::create_synthetic(
    Allocator::Arena& arena,
    const Abstract& language,
    Abstract& context,
    View::Vector<Dependency> dependencies) -> Monograph& {
  Monograph& monograph = arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        arena, language, Documentation::get_empty(), context, dependencies, {});
  });

  // Restored Packages have no authored route acquisition phase. Seal before
  // publishing the Monograph so later owners cannot attach physical Storage.
  monograph.resources.seal();
  return monograph;
}

static auto retain_dependencies(
    Managed::Vector<Package::Language::Dependency>& retained,
    View::Vector<Package::Language::Dependency> source) -> void {
  for (const Package::Language::Dependency& dependency : source) {
    retained.insert(dependency);
  }
}

static auto retain_sources(
    Managed::Vector<Package::Language::Source>& retained,
    View::Vector<Package::Language::Source> source) -> void {
  for (const Package::Language::Source& authored : source) {
    retained.insert(authored);
  }
}

Package::Language::Monograph::Monograph(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context,
    View::Vector<Dependency> authored_dependencies,
    View::Vector<Source> authored_sources)
    : Tetrodotoxin::Language::Monograph(
          arena,
          language,
          documentation,
          context),
      dependencies(domain),
      sources(domain),
      resources(domain),
      scope(domain, "Package"_view) {
  retain_dependencies(dependencies, authored_dependencies);
  retain_sources(sources, authored_sources);
}

auto Package::Language::Monograph::Scope::bind(
    const Parser::Name& route,
    const Abstract& target) -> Option<Alias&> {
  BAIL_IF(route.get_size() == 0);

  Scope* selected = this;
  for (Count index = 0; index + 1 < route.get_size(); index++) {
    View::Bytes segment = route.get_segment(index);
    auto existing = selected->bindings.find(segment);
    if (existing) {
      auto nested = existing->value.select<Scope>();
      BAIL_IF(!nested);
      selected = &*nested;
      continue;
    }

    Scope& nested = arena.construct<Scope>(arena, segment);
    selected->bindings.launder(segment, nested);
    selected = &nested;
  }

  View::Bytes leaf = route.get_segment(route.get_size() - 1);
  BAIL_IF(leaf.is_empty() || selected->bindings.contains(leaf));
  Alias& alias = arena.construct<Alias>(leaf, target);
  selected->bindings.launder(leaf, alias);
  return alias;
}

auto Package::Language::Monograph::Scope::resolve_context(
    View::Bytes selected_name) const -> const Abstract& {
  return bindings.visit(
      selected_name,
      [](const Abstract& selected) -> const Abstract& { return selected; },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}

auto Package::Language::Monograph::bind_member(
    const Parser::Name& local_name,
    const Tetrodotoxin::Language::Monograph& member) -> Bool {
  // Every rejection happens before either inventory changes, so exact lookup
  // and member order preserve the first completed edge.
  if (local_name.get_size() == 0 ||
      dependencies.get_view().contains([&](const Dependency& dependency) {
        return dependency.get_local_name() == local_name.get_view();
      }) ||
      (!sources.is_empty() &&
       !sources.get_view().contains([&](const Source& source) {
         return source.get_local_name() == local_name.get_view();
       })) ||
      &member == this) {
    return False;
  }

  auto alias = scope.bind(local_name, member);
  BAIL_IF(!alias);
  return True;
}

auto Package::Language::Monograph::bind_dependency(
    const Dependency& dependency,
    const Monograph& package) -> Bool {
  const Parser::Name& local_name = dependency.get_local_route();

  // A caller cannot manufacture another alias spelling for a retained request.
  // Source inventory checks happen before construction so staging order never
  // decides which cross kind meaning survives.
  if (local_name.get_size() == 0 ||
      !dependencies.get_view().contains([&](const Dependency& retained) {
        return &retained == &dependency;
      }) ||
      sources.get_view().contains([&](const Source& source) {
        return source.get_local_name() == local_name.get_view();
      }) ||
      &package == this) {
    return False;
  }

  return Bool(scope.bind(local_name, package));
}

auto Package::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // The delimiters reserve one complete resource route. Malformed or partial
  // spellings continue through exact Package lookup so this branch never
  // becomes a second Embedded parser.
  if (is_resource_route(route)) {
    return resources.resolve(route.slice(2, route.get_size() - 3));
  }

  const Abstract& local = scope.resolve_context(route);
  if (!local.is<Invalid>()) {
    return local;
  }

  return Tetrodotoxin::Language::Monograph::resolve_context(route);
}

auto Package::Language::Monograph::get_name() const -> View::Bytes {
  return "Package"_view;
}

auto Package::Language::Monograph::get_dependencies() const
    -> View::Vector<Dependency> {
  return dependencies;
}

auto Package::Language::Monograph::get_sources() const -> View::Vector<Source> {
  return sources;
}

auto Package::Language::Monograph::get_resources() -> Package::Resources& {
  return resources;
}

auto Package::Language::Monograph::get_resources() const
    -> const Package::Resources& {
  return resources;
}
