// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {

// Workspace is the host for managing collections of sources typically in a
// package context.
//
// Dialects must be added to the work space for it to be able to interpret
// different sources. By default it only parses the Tetrodotoxin source header
// which includes the source document comment and a single `dialect : Type`
// directive.
class Workspace : Ttx::Concept::Abstract {
 public:
  Workspace()
      : arena(),
        installed_routes(arena),
        installed_dialects(arena),
        dialects(arena),
        source_monographs(arena) {}

  // Installed dialects are allowed to own actual localized graph state so they
  // all need to be destructed to avoid thread memory leaks.
  constexpr ~Workspace() override {
    for (Count i = 0; i < installed_dialects.get_size(); i++) {
      installed_dialects[i]->~Dialect();
    }
  }

  // Installs a Dialect under a name for interpreting source.
  //
  // The same Dialect can be installed under multiple names, however each
  // instance is strictly unique and has it's own graph context.
  template <typename target_dialect>
  constexpr auto install_dialect(Perimortem::Core::View::Bytes name) -> Bool {
    // Can't alias an already included dialect mapping.
    if (dialects.contains(name)) {
      return false;
    }

    // Add the dialect for use directly.
    auto new_dialect = arena.construct<target_dialect>(*this);
    installed_dialects.insert(&new_dialect);
    installed_routes.insert(name);
    dialects.insert({name, new_dialect});
  }

  // Imports a source into the workplace. If it's a package then it's
  // proccessed further.
  auto import_source(
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes contents,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Lexical::Errors& errors) -> Bool;

  // TODO: Actually route cross source look ups / name resolution.
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
      installed_routes;
  Perimortem::Memory::Managed::Vector<Language::Dialect*> installed_dialects;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Dialect&>
          dialects;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Dialect::Monograph&>
          source_monographs;
};

}  // namespace Tetrodotoxin::Environment
