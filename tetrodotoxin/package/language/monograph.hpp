// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/source.hpp"
#include "tetrodotoxin/package/resources.hpp"
#include "ttx/lexical/span.hpp"
#include "ttx/model/alias.hpp"

namespace Tetrodotoxin::Package::Language {

// Describes one exact Package local Alias scope after manifest interpretation
// or Archive restoration. Member lookup and ordered enumeration borrow the
// Workspace owned Monographs they map.
class Monograph : public Tetrodotoxin::Language::Monograph {
 private:
  // A qualified Package name is a real chain of graph contexts. Scope owns
  // one segment table. It never flattens `A::B` into a second lookup language.
  class Scope : public Ttx::Concept::Abstract {
   public:
    Scope(
        Perimortem::Memory::Allocator::Arena& arena,
        Perimortem::Core::View::Bytes name)
        : arena(arena), name(name), bindings(arena) {}

    TTX_CONTRACT(Scope, Ttx::Concept::Abstract);
    TTX_NAME(name);
    TTX_EMPTY_DOCUMENTATION();

    auto bind(const Parser::Name& route, const Ttx::Concept::Abstract& target)
        -> Perimortem::Core::Option<Ttx::Model::Alias&>;

    auto resolve_context(Perimortem::Core::View::Bytes name) const
        -> const Ttx::Concept::Abstract& override;

   private:
    Perimortem::Memory::Allocator::Arena& arena;
    Perimortem::Core::View::Bytes name;
    Perimortem::Memory::Managed::
        Map<Perimortem::Core::View::Bytes, Ttx::Concept::Abstract&>
            bindings;
  };

  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context,
      Perimortem::Core::View::Vector<Dependency> dependencies,
      Perimortem::Core::View::Vector<Source> sources);

 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  // Authored construction rejects an empty Source inventory before any graph
  // identity enters the Arena.
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context,
      Perimortem::Core::View::Vector<Dependency> dependencies,
      Perimortem::Core::View::Vector<Source> sources)
      -> Perimortem::Core::Option<Monograph&>;

  // Archive restoration has no authored Tokens or Source paths. Selecting
  // this operation records that absence directly instead of asking callers to
  // infer it from a vector length.
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context,
      Perimortem::Core::View::Vector<Dependency> dependencies) -> Monograph&;

  // Authored Packages accept their declared Source names while source free
  // Packages accept the member inventory validated by Archive Reader. The
  // Workspace that assembles the Package owns every referenced Monograph.
  auto bind_member(
      const Parser::Name& local_name,
      const Tetrodotoxin::Language::Monograph& member) -> Bool;

  // The request must belong to this Monograph. The Package records only the
  // exact borrowed mapping selected by its Workspace.
  auto bind_dependency(const Dependency& dependency, const Monograph& package)
      -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto get_dependencies() const -> Perimortem::Core::View::Vector<Dependency>;

  auto get_sources() const -> Perimortem::Core::View::Vector<Source>;

  auto get_resources() -> Tetrodotoxin::Package::Resources&;
  auto get_resources() const -> const Tetrodotoxin::Package::Resources&;

 private:
  // Scope is the complete borrowed mapping table. It never participates in
  // the lifetime of the Workspace owned identities it selects.
  Perimortem::Memory::Managed::Vector<Dependency> dependencies;
  Perimortem::Memory::Managed::Vector<Source> sources;
  mutable Tetrodotoxin::Package::Resources resources;
  Scope scope;
};

}  // namespace Tetrodotoxin::Package::Language
