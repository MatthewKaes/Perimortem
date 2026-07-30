// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {

// Workspace owns one semantic island, including installed Dialect state,
// imported source bytes, and the Monographs published by successful imports.
class Workspace : public Ttx::Concept::Abstract {
 public:
  Workspace();
  ~Workspace() override;

  // Installs a Dialect under a name for interpreting source.
  //
  // The same Dialect can be installed under multiple names, however each
  // instance is distinct and retains its own graph context.
  template <typename TargetDialect>
  constexpr auto install_dialect(Perimortem::Core::View::Bytes name) -> Bool {
    // Reject the exact authored duplicate before constructing owner state.
    if (dialects.contains(name)) {
      return false;
    }

    Perimortem::Core::View::Bytes installed_name = arena.proxy(name);
    auto& new_dialect = arena.construct<TargetDialect>(*this);

    installed_names.insert(installed_name);
    installed_dialects.insert(&new_dialect);

    // Managed::Map::insert requires assignable values even when the key is new.
    // Dialect references are nonassignable owner state, so launder constructs
    // the new binding. The duplicate guard guarantees it never replaces one.
    dialects.launder(installed_name, new_dialect);
    return true;
  }

  // Imports one direct source under an exact semantic name. The diagnostic
  // path identifies parser errors and never becomes semantic identity.
  auto import_source(
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents,
      Ttx::Lexical::Errors& errors) -> Bool;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
      installed_names;
  Perimortem::Memory::Managed::Vector<Language::Dialect*> installed_dialects;
  Perimortem::Memory::Managed::Vector<Language::Dialect::Monograph*>
      retained_monographs;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Dialect&>
          dialects;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Dialect::Monograph&>
          source_monographs;
};

}  // namespace Tetrodotoxin::Environment
