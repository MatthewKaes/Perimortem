// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/aligned_buffer.h"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/hash_index.h"

#include "perimortem/system/file.hpp"

namespace Tetrodotoxin::Package {

// Snapshots owns reusable nonsemantic filesystem bytes across complete
// Workspace graph replacements. Package Storage remains responsible for route
// normalization and confinement, while this owner compares metadata from the
// exact opened member before reusing an immutable byte value.
class Snapshots {
 public:
  Snapshots() = default;
  ~Snapshots();
  Snapshots(const Snapshots&) = delete;
  Snapshots(Snapshots&&) = delete;
  auto operator=(const Snapshots&) -> Snapshots& = delete;
  auto operator=(Snapshots&&) -> Snapshots& = delete;

  auto read(
      const Perimortem::System::File::Root& root,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  // An editor overlay is authoritative until removed. Empty text remains a
  // real overlay and never falls through to the filesystem snapshot.
  auto overlay(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route,
      Perimortem::Core::View::Bytes contents) -> Bool;

  auto remove_overlay(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) -> Bool;

 private:
  struct Entry {
    constexpr Entry() = default;

    Perimortem::Memory::Dynamic::Bytes contents;
    Perimortem::Memory::Dynamic::Bytes overlay_contents;
    Perimortem::System::File::Fingerprint fingerprint;
    Bool loaded = False;
    Bool overlaid = False;
  };

  struct StoredEntry {
    Perimortem::Core::View::Bytes key;
    Entry value;
  };

  auto find(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Core::Option<Entry&>;

  auto create(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Core::Option<Entry&>;

  auto ensure_capacity(Count required) -> Bool;

  // The Arena retains stable lookup bytes. The Hash Index stores only their
  // hashes and entry positions, while Snapshots owns comparison, relocation,
  // and destruction for the real typed entries.
  Perimortem::Memory::Allocator::Arena arena;
  perimortem_aligned_buffer entry_storage = {};
  perimortem_hash_index entry_index = {};
  Count entry_count = 0;
  Count entry_capacity = 0;
};

}  // namespace Tetrodotoxin::Package
