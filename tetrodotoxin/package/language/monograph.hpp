// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/span.hpp"

namespace Tetrodotoxin::Package::Language {

// Contains the dependency and source manifest of a `package.ttx`.
// Used by Environment and other systems to bootstrap a valid TTX island.
class Monograph : public Tetrodotoxin::Language::Dialect::Monograph {
 private:
  struct Construction {};

 public:
  using ClassCatagory = Monograph;
  static constexpr Perimortem::System::Uuid contract_id{
    0x5f23a554d3745ebd,
    0xe899f24ef18c3a52,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Tetrodotoxin::Language::Dialect::Monograph::implements(requested);
  }

  // Authored construction rejects a partial provenance inventory before any
  // graph identity enters the Arena.
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Dialect& host,
      Perimortem::Core::View::Vector<Dependency> dependencies,
      Perimortem::Core::View::Vector<Ttx::Lexical::Span> dependency_spans,
      Perimortem::Core::View::Vector<Source> sources)
      -> Perimortem::Utility::Option<Monograph&> {
    if (dependencies.get_size() != dependency_spans.get_size()) {
      return {};
    }

    return domain.construct<Monograph>(
        Construction(), domain, documentation, host, dependencies,
        dependency_spans, sources);
  }

  // Archive restoration has no authored Tokens or Source paths. Selecting
  // this operation records that absence directly instead of asking callers to
  // infer it from a vector length.
  static auto create_source_free(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Dialect& host,
      Perimortem::Core::View::Vector<Dependency> dependencies) -> Monograph& {
    return domain.construct<Monograph>(
        Construction(), domain, documentation, host, dependencies,
        Perimortem::Core::View::Vector<Ttx::Lexical::Span>(),
        Perimortem::Core::View::Vector<Source>());
  }

  Monograph(
      Construction,
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Dialect& host,
      Perimortem::Core::View::Vector<Dependency> dependencies,
      Perimortem::Core::View::Vector<Ttx::Lexical::Span> dependency_spans,
      Perimortem::Core::View::Vector<Source> sources)
      : Tetrodotoxin::Language::Dialect::Monograph(domain, documentation, host),
        dependencies(dependencies),
        dependency_spans(dependency_spans),
        sources(sources) {}

  // The package monograph requires the caller to know the contract directly.
  // It doesn't resolve any routes to any subtree context.
  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Package"_view;
  }

  constexpr auto get_dependencies() const
      -> Perimortem::Core::View::Vector<Dependency> {
    return dependencies;
  }

  constexpr auto get_dependency_spans() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Span> {
    return dependency_spans;
  }

  constexpr auto get_sources() const -> Perimortem::Core::View::Vector<Source> {
    return sources;
  }

 private:
  // The Package Monograph is fully formed by interpretation and only exposes
  // ordered views over its Arena backed durable values.
  Perimortem::Core::View::Vector<Dependency> dependencies;
  Perimortem::Core::View::Vector<Ttx::Lexical::Span> dependency_spans;
  Perimortem::Core::View::Vector<Source> sources;
};
}  // namespace Tetrodotoxin::Package::Language
