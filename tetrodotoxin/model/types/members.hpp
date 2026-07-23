// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressables/writable.hpp"
#include "ttx/model/callables/self.hpp"
#include "ttx/model/callables/static.hpp"

namespace Tetrodotoxin::Model::Types {

// Members is the identity-free ownership value embedded in a concrete Type.
// Roots retain all declarations in authored order while the named, Static, and
// Self indices preserve the distinct lookup surfaces selected by source
// grammar. Public edges are recorded independently from retained lookup. The
// stored entries are real Abstract edges, never copied field or function
// records.
//
// Construction is append only until seal(). The containing Type owns that
// finalization point and cannot reopen this value after an immutable consumer
// begins.
class Members {
 public:
  Members(Perimortem::Memory::Allocator::Arena& arena)
      : roots(arena),
        exports(arena),
        roots_by_name(arena),
        exports_by_name(arena),
        statics_by_name(arena),
        exported_statics_by_name(arena),
        selves_by_name(arena),
        exported_selves_by_name(arena) {}

  auto add_root(
      const Ttx::Concept::Abstract& member,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  auto add_export(
      const Ttx::Concept::Abstract& member,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  auto add_exposed(
      const Ttx::Model::Addressables::Writable& member,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  auto add_static(
      const Ttx::Model::Callables::Static& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exported_static(
      const Ttx::Model::Callables::Static& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  auto add_self(
      const Ttx::Model::Callables::Self& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exported_self(
      const Ttx::Model::Callables::Self& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;

  constexpr auto get_root_count() const -> Count { return roots.get_size(); }
  auto get_root(Count index) const -> const Ttx::Concept::Abstract&;

  constexpr auto get_export_count() const -> Count {
    return exports.get_size();
  }
  auto get_export(Count index) const -> const Ttx::Concept::Abstract&;

  auto resolve_root(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_static(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_exported_static(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_self(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_exported_self(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto seal() -> Bool;

 private:
  using Index = Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>;

  auto outer_contains(
      const Ttx::Concept::Abstract& outer_context,
      Perimortem::Core::View::Bytes name) const -> Bool;

  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      roots;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      exports;
  Index roots_by_name;
  Index exports_by_name;
  Index statics_by_name;
  Index exported_statics_by_name;
  Index selves_by_name;
  Index exported_selves_by_name;
  Bool sealed = False;
};

}  // namespace Tetrodotoxin::Model::Types
