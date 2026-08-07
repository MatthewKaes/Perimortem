// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/language/monograph.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin;

static auto has_dependency(
    View::Vector<Package::Language::Dependency> dependencies,
    const Package::Language::Dependency& candidate) -> Bool {
  for (Count i = 0; i < dependencies.get_size(); i++) {
    if (&dependencies.get_data()[i] == &candidate) {
      return True;
    }
  }

  return False;
}

static auto has_dependency_name(
    View::Vector<Package::Language::Dependency> dependencies,
    View::Bytes local_name) -> Bool {
  for (Count i = 0; i < dependencies.get_size(); i++) {
    if (dependencies.get_data()[i].get_local_name() == local_name) {
      return True;
    }
  }

  return False;
}

static auto has_source_name(
    View::Vector<Package::Language::Source> sources,
    View::Bytes local_name) -> Bool {
  for (Count i = 0; i < sources.get_size(); i++) {
    if (sources.get_data()[i].get_local_name() == local_name) {
      return True;
    }
  }

  return False;
}

static auto is_resource_instruction(View::Bytes route) -> Bool {
  return route.get_size() >= 3 && route[0] == '$' && route[1] == '[' &&
         route[route.get_size() - 1] == ']';
}

auto Package::Language::Monograph::create_authored(
    Allocator::Arena& domain,
    const Documentation& documentation,
    View::Vector<Dependency> dependencies,
    View::Vector<Span> dependency_spans,
    View::Vector<Source> sources) -> Option<Monograph&> {
  if (dependencies.get_size() != dependency_spans.get_size() ||
      sources.is_empty()) {
    return {};
  }

  return domain.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        domain, documentation, dependencies, dependency_spans, sources);
  });
}

auto Package::Language::Monograph::create_synthetic(
    Allocator::Arena& domain,
    const Documentation& documentation,
    View::Vector<Dependency> dependencies) -> Monograph& {
  Monograph& monograph = domain.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        domain, documentation, dependencies, View::Vector<Span>(),
        View::Vector<Source>());
  });

  // Restored Packages have no authored route acquisition phase. Seal before
  // publishing the Monograph so later owners cannot attach physical Storage.
  monograph.resources.seal();
  return monograph;
}

Package::Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Documentation& documentation,
    View::Vector<Dependency> dependencies,
    View::Vector<Span> dependency_spans,
    View::Vector<Source> sources)
    : Tetrodotoxin::Language::Monograph(domain, documentation),
      dependencies(dependencies),
      dependency_spans(dependency_spans),
      sources(sources),
      resources(domain.construct<Package::Resources>(domain)),
      members(domain),
      bindings(domain) {}

auto Package::Language::Monograph::bind_member(
    View::Bytes local_name,
    const Tetrodotoxin::Language::Monograph& member) -> Bool {
  // Every rejection happens before either inventory changes, so exact lookup
  // and member order preserve the first completed edge.
  if (local_name.is_empty() || has_dependency_name(dependencies, local_name) ||
      (!sources.is_empty() && !has_source_name(sources, local_name)) ||
      &member == this || bindings.contains(local_name)) {
    return False;
  }

  Alias& alias = domain.construct<Alias>(local_name, member);
  bindings.launder(local_name, alias);
  members.insert(alias);
  return True;
}

auto Package::Language::Monograph::bind_dependency(
    const Dependency& dependency,
    const Monograph& package) -> Bool {
  const View::Bytes local_name = dependency.get_local_name();

  // A caller cannot manufacture another alias spelling for a retained request.
  // Source inventory checks happen before construction so staging order never
  // decides which cross kind meaning survives.
  if (local_name.is_empty() || !has_dependency(dependencies, dependency) ||
      has_source_name(sources, local_name) || &package == this ||
      bindings.contains(local_name)) {
    return False;
  }

  Alias& alias = domain.construct<Alias>(local_name, package);
  bindings.launder(local_name, alias);
  return True;
}

auto Package::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // The delimiters reserve one complete contextual instruction. Malformed or
  // partial spellings continue through exact Package lookup so this branch
  // never becomes a second Embedded parser.
  if (is_resource_instruction(route)) {
    return resources.resolve(route.slice(2, route.get_size() - 3));
  }

  return bindings.visit(
      route, [](const Alias& selected) -> const Abstract& { return selected; },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}

auto Package::Language::Monograph::get_name() const -> View::Bytes {
  return "Package"_view;
}

auto Package::Language::Monograph::get_dependencies() const
    -> View::Vector<Dependency> {
  return dependencies;
}

auto Package::Language::Monograph::get_dependency_spans() const
    -> View::Vector<Span> {
  return dependency_spans;
}

auto Package::Language::Monograph::get_sources() const -> View::Vector<Source> {
  return sources;
}

auto Package::Language::Monograph::get_members() const
    -> View::Vector<Reference<const Alias>> {
  return members;
}

auto Package::Language::Monograph::get_resources() -> Package::Resources& {
  return resources;
}

auto Package::Language::Monograph::get_resources() const
    -> const Package::Resources& {
  return resources;
}
