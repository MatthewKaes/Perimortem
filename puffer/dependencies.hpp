// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"

namespace Puffer {

// Dependencies selects exact Contract products in dependency order. A caller
// may retain products declared by its build request and then acquire any
// missing coordinate from the same Repository used by editor source selection.
class Dependencies {
 public:
  constexpr Dependencies(
      Perimortem::Memory::Allocator::Arena& arena,
      Tetrodotoxin::Package::Repository::Repository& repository)
      : repository(repository), archives(arena) {}

  auto retain(const Tetrodotoxin::Package::Archive::Archive& archive) -> Bool;

  auto acquire(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version) -> Bool;

  constexpr auto get_archives() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Package::Archive::Archive> {
    return archives.get_view();
  }

 private:
  struct Coordinate {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
  };

  auto contains(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version) const -> Bool;

  Tetrodotoxin::Package::Repository::Repository& repository;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Package::Archive::Archive>
      archives;
  Perimortem::Memory::Dynamic::Vector<Coordinate> active;
};

}  // namespace Puffer
