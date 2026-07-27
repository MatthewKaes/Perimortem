// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/source.hpp"
#include "ttx/concept/invalid.hpp"

namespace Tetrodotoxin::Package::Language {

// Contains the dependency and source manifest of a `package.ttx`.
// Used by Environment and other systems to bootstrap a valid TTX island.
class Monograph : public Tetrodotoxin::Language::Dialect::Monograph {
 public:
  using ClassCatagory = Monograph;
  static constexpr Perimortem::System::Uuid contract_id{
    0x5f23a554d3745ebd,
    0xe899f24ef18c3a52,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id;
  }

  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Dialect& host,
      Perimortem::Core::View::Vector<Dependency> dependencies,
      Perimortem::Core::View::Vector<Source> sources)
      : Tetrodotoxin::Language::Dialect::Monograph(domain, documentation, host),
        dependencies(dependencies),
        sources(sources) {}

  // The package monograph requires the caller to know the contract directly.
  // It doesn't resolve any routes to any subtree context.
  constexpr auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  };

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Package"_view;
  };

  constexpr auto get_dependencies() const
      -> Perimortem::Core::View::Vector<Dependency> {
    return dependencies;
  }

  constexpr auto get_sources() const -> Perimortem::Core::View::Vector<Source> {
    return sources;
  }

 private:
  // The package monograph is fully formed on parse so we only have to expose
  // views to the client which can manage mutable import state if any.
  Perimortem::Core::View::Vector<Dependency> dependencies;
  Perimortem::Core::View::Vector<Source> sources;
};
}  // namespace Tetrodotoxin::Package::Language
