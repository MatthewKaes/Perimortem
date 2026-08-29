// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"

namespace Ttx::Model {

// Context owns one bounded set of graph-query snapshots. It provides no
// lookup, registry, callback payload, or arbitrary storage API; Abstract owners
// can only ask it to retain one Pack from an existing Layout.
class Context : public Concept::Context {
 public:
  constexpr explicit Context(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena) {}

  auto pack(const Concept::Layout& layout) -> const Concept::Pack& override;

 private:
  class SnapshotLayout : public Concept::Layout {
   public:
    SnapshotLayout(
        Perimortem::Memory::Allocator::Arena& arena,
        const Concept::Layout& source);

    constexpr auto get_size() const -> Count override {
      return abstracts.get_size();
    }
    constexpr auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Concept::Abstract&> override;
    constexpr auto get_name(Count index) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;
    auto fits_entry(
        const Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override;
    auto fits_at(const Concept::Layout& target, Count target_offset) const
        -> Bool override;
    auto get_fitted_at(
        const Concept::Layout& target,
        Count target_offset,
        Count target_index) const -> Perimortem::Utility::
        Result<const Concept::Abstract&, Errors> override;

   private:
    Perimortem::Memory::Managed::Vector<
        Concept::Reference<const Concept::Abstract>>
        abstracts;
    Perimortem::Memory::Managed::Vector<
        Perimortem::Core::Option<Perimortem::Core::View::Bytes>>
        names;
  };

  class SnapshotPack : public Concept::Pack {
   public:
    constexpr explicit SnapshotPack(const Concept::Layout& layout)
        : layout(layout) {}

    constexpr auto get_layout() const -> const Concept::Layout& override {
      return layout;
    }

   private:
    const Concept::Layout& layout;
  };

  Perimortem::Memory::Allocator::Arena& arena;
};

}  // namespace Ttx::Model
