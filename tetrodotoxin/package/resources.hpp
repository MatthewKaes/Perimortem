// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/package/storage.hpp"

namespace Tetrodotoxin::Package {

// Owns contextual resource identities for one Package Monograph. Monograph
// construction has its source transaction Arena but not physical Package
// Storage, so Workspace connects this owner to confined Storage while it
// completes the manifest's fixed Source table.
//
// While connected, complete resource routes read through that one
// Storage and cache one Arena stable Resource or Error identity. The Package
// import operation seals Resources before it links or finalizes the table.
// Sealing clears only
// the borrowed Storage pointer, so cached identities remain valid for the
// Monograph lifetime while a new route resolves to Invalid.
//
// A Resources connection can be used only once. Sealing never permits another
// Storage to be selected, and source free construction seals before publishing
// its Monograph.
class Resources {
 private:
  enum class Stage : U8 {
    Pending,
    Connected,
    Sealed,
  };

 public:
  constexpr Resources(Perimortem::Memory::Allocator::Arena& domain)
      : domain(domain),
        storage(nullptr),
        stage(Stage::Pending),
        resource_cache(domain),
        error_cache(domain) {}

  auto connect(Storage& storage) -> Bool;
  auto seal() -> void;

  auto resolve(Perimortem::Core::View::Bytes logical_route)
      -> const Ttx::Concept::Abstract&;

 private:
  Perimortem::Memory::Allocator::Arena& domain;
  Storage* storage;
  Stage stage;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Tetrodotoxin::Language::Resource&>
          resource_cache;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Tetrodotoxin::Language::Error&>
          error_cache;
};

}  // namespace Tetrodotoxin::Package
