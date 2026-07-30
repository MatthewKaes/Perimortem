// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/file.hpp"

#include "perimortem/utility/option.hpp"

namespace Tetrodotoxin::Package {

// Opens the physical root for one Package and caches every successful Source
// or resource read by its normalized logical route.
//
// Package::Language::Source owns the semantic name and authored route. Storage
// resolves only that route into a diagnostic path and bytes. It constructs
// Content in the Workspace Arena so staged Source and resource consumers
// retain stable views for the semantic island lifetime, even after Storage
// closes its root.
//
// The cache belongs to this opened Package storage only. Storage never
// interprets content or derives semantic identity from a route.
class Storage {
 public:
  // Binds one canonical diagnostic path to its retained content bytes.
  class Content {
   public:
    constexpr Content(
        Perimortem::Core::View::Bytes diagnostic_path,
        Perimortem::Core::View::Bytes contents)
        : diagnostic_path(diagnostic_path), contents(contents) {}

    constexpr auto get_diagnostic_path() const
        -> Perimortem::Core::View::Bytes {
      return diagnostic_path;
    }

    constexpr auto get_contents() const -> Perimortem::Core::View::Bytes {
      return contents;
    }

   private:
    Perimortem::Core::View::Bytes diagnostic_path;
    Perimortem::Core::View::Bytes contents;
  };

  Storage(const Storage&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;
  Storage(Storage&&) = default;
  auto operator=(Storage&&) -> Storage& = delete;

  static auto open(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes root)
      -> Perimortem::Utility::Option<Storage>;

  auto read(Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Utility::Option<Content&>;

 private:
  Storage(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::System::File::Root&& root)
      : arena(arena),
        root(static_cast<Perimortem::System::File::Root&&>(root)),
        cache(arena) {}

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::System::File::Root root;
  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Content&>
      cache;
};

}  // namespace Tetrodotoxin::Package
