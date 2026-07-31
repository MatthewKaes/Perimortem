// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/map.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/source.hpp"
#include "ttx/lexical/span.hpp"
#include "ttx/model/alias.hpp"

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

  // Authored construction rejects an empty Source inventory or partial
  // provenance before any graph identity enters the Arena.
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Dialect& host,
      Perimortem::Core::View::Vector<Dependency> dependencies,
      Perimortem::Core::View::Vector<Ttx::Lexical::Span> dependency_spans,
      Perimortem::Core::View::Vector<Source> sources)
      -> Perimortem::Utility::Option<Monograph&>;

  // Archive restoration has no authored Tokens or Source paths. Selecting
  // this operation records that absence directly instead of asking callers to
  // infer it from a vector length.
  static auto create_source_free(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Dialect& host,
      Perimortem::Core::View::Vector<Dependency> dependencies) -> Monograph&;

  Monograph(
      Construction,
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Dialect& host,
      Perimortem::Core::View::Vector<Dependency> dependencies,
      Perimortem::Core::View::Vector<Ttx::Lexical::Span> dependency_spans,
      Perimortem::Core::View::Vector<Source> sources);

  // Member names and targets must already belong to the Monograph Arena.
  // Authored Packages accept their declared Source names while source free
  // Packages accept the member inventory validated by Archive Reader.
  auto bind_member(
      Perimortem::Core::View::Bytes local_name,
      const Tetrodotoxin::Language::Dialect::Monograph& member) -> Bool;

  // The request must belong to this Monograph and the completed Package root
  // must already share its Arena lifetime.
  auto bind_dependency(const Dependency& dependency, const Monograph& package)
      -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto get_dependencies() const -> Perimortem::Core::View::Vector<Dependency>;

  auto get_dependency_spans() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Span>;

  auto get_sources() const -> Perimortem::Core::View::Vector<Source>;

 private:
  // The Package Monograph is fully formed by interpretation and only exposes
  // ordered views over its Arena backed durable values.
  Perimortem::Core::View::Vector<Dependency> dependencies;
  Perimortem::Core::View::Vector<Ttx::Lexical::Span> dependency_spans;
  Perimortem::Core::View::Vector<Source> sources;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Ttx::Model::Alias&>
          bindings;
};
}  // namespace Tetrodotoxin::Package::Language
