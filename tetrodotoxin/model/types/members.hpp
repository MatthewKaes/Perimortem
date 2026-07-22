// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Model::Types {

// Members is the identity-free ownership value embedded in a concrete Type.
// Roots retain all declarations while exports alone participate in public
// lookup. The stored entries are real Abstract edges, never copied field or
// function records.
class Members {
 public:
  Members(Perimortem::Memory::Allocator::Arena& arena)
      : roots(arena),
        exports(arena),
        roots_by_name(arena),
        exports_by_name(arena) {}

  auto add(const Ttx::Concept::Abstract& member, Bool publish) -> Bool;

  constexpr auto get_root_count() const -> Count { return roots.get_size(); }
  auto get_root(Count index) const -> const Ttx::Concept::Abstract&;

  constexpr auto get_export_count() const -> Count {
    return exports.get_size();
  }
  auto get_export(Count index) const -> const Ttx::Concept::Abstract&;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

 private:
  using Index = Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>;

  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      roots;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      exports;
  Index roots_by_name;
  Index exports_by_name;
};

}  // namespace Tetrodotoxin::Model::Types
