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
    if (&dependencies[i] == &candidate) {
      return True;
    }
  }

  return False;
}

static auto has_dependency_name(
    View::Vector<Package::Language::Dependency> dependencies,
    View::Bytes local_name) -> Bool {
  for (Count i = 0; i < dependencies.get_size(); i++) {
    if (dependencies[i].get_local_name() == local_name) {
      return True;
    }
  }

  return False;
}

static auto has_source_name(
    View::Vector<Package::Language::Source> sources,
    View::Bytes local_name) -> Bool {
  for (Count i = 0; i < sources.get_size(); i++) {
    if (sources[i].get_local_name() == local_name) {
      return True;
    }
  }

  return False;
}

static auto bind_alias(
    Allocator::Arena& domain,
    Managed::Map<View::Bytes, Alias&>& bindings,
    View::Bytes local_name,
    const Abstract& target) -> Bool {
  if (local_name.is_empty() || bindings.contains(local_name)) {
    return False;
  }

  Alias& alias = domain.construct<Alias>(local_name, target);
  bindings.launder(local_name, alias);
  return True;
}

auto Package::Language::Monograph::create_authored(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Tetrodotoxin::Language::Dialect& host,
    View::Vector<Dependency> dependencies,
    View::Vector<Span> dependency_spans,
    View::Vector<Source> sources) -> Option<Monograph&> {
  if (dependencies.get_size() != dependency_spans.get_size() ||
      sources.is_empty()) {
    return {};
  }

  return domain.construct<Monograph>(
      Construction(), domain, documentation, host, dependencies,
      dependency_spans, sources);
}

auto Package::Language::Monograph::create_source_free(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Tetrodotoxin::Language::Dialect& host,
    View::Vector<Dependency> dependencies) -> Monograph& {
  return domain.construct<Monograph>(
      Construction(), domain, documentation, host, dependencies,
      View::Vector<Span>(), View::Vector<Source>());
}

Package::Language::Monograph::Monograph(
    Construction,
    Allocator::Arena& domain,
    const Documentation& documentation,
    Tetrodotoxin::Language::Dialect& host,
    View::Vector<Dependency> dependencies,
    View::Vector<Span> dependency_spans,
    View::Vector<Source> sources)
    : Tetrodotoxin::Language::Dialect::Monograph(domain, documentation, host),
      dependencies(dependencies),
      dependency_spans(dependency_spans),
      sources(sources),
      bindings(domain) {}

auto Package::Language::Monograph::bind_member(
    View::Bytes local_name,
    const Tetrodotoxin::Language::Dialect::Monograph& member) -> Bool {
  // Authored Source names and every Dependency alias reserve one scope before
  // staging begins. Source free members arrive without Source values, so their
  // validated Archive names enter through this same exact key operation.
  if (has_dependency_name(dependencies, local_name) ||
      (!sources.is_empty() && !has_source_name(sources, local_name)) ||
      &member == this) {
    return False;
  }

  return bind_alias(domain, bindings, local_name, member);
}

auto Package::Language::Monograph::bind_dependency(
    const Dependency& dependency,
    const Monograph& package) -> Bool {
  const View::Bytes local_name = dependency.get_local_name();

  // A caller cannot manufacture another alias spelling for a retained request.
  // Source inventory checks happen before construction so staging order never
  // decides which cross kind meaning survives.
  if (!has_dependency(dependencies, dependency) ||
      has_source_name(sources, local_name) || &package == this) {
    return False;
  }

  return bind_alias(domain, bindings, local_name, package);
}

auto Package::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
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
