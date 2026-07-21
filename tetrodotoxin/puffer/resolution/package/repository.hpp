// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/object.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/model/environment.hpp"
#include "tetrodotoxin/model/package.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Package {

// Repository owns registered immutable package buffers and the complete
// restored lifetimes they produce. Manifest identity is repository data;
// restored Packages remain anonymous semantic objects.
class Repository {
 public:
  // Loads one immutable archive from disk and registers its Manifest identity.
  // Disk discovery remains caller policy; Repository owns the bytes after this
  // transaction so restored Packages never borrow a temporary file buffer.
  auto register_file(Perimortem::Core::View::Bytes path) -> Bool;

  auto register_buffer(
      Perimortem::Core::View::Bytes content,
      Perimortem::Core::View::Bytes diagnostic_path = {}) -> Bool;

  auto find_manifest(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Version version) const
      -> const Tetrodotoxin::Archiver::Manifest*;

  auto resolve(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Version version) -> const Ttx::Concept::Abstract&;

  // Resolves one exact repository key and publishes its local binding into a
  // Environment. The Repository remains the lifetime owner of the restored
  // Package; the Environment owns only the durable resolution edge consumed
  // by Sources in this build transaction.
  auto resolve(
      Model::Environment& environment,
      const Model::Dialect& root_dialect,
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version) -> Bool;

 private:
  // Key is a repository lookup value, not semantic Package identity. Its byte
  // view points into the immutable Manifest retained by the indexed Record.
  class Key {
   public:
    constexpr Key(
        Perimortem::Core::View::Bytes name,
        Perimortem::System::Version version)
        : name(name), version(version) {}

    constexpr auto operator==(const Key& rhs) const -> Bool {
      return name == rhs.name && version == rhs.version;
    }

    constexpr auto hash() const -> Unsigned_64 {
      Unsigned_64 encoded =
          (Unsigned_64(version.get_major()) << 16) | version.get_minor();
      return Perimortem::Core::Hash(name).Rehash(encoded);
    }

   private:
    Perimortem::Core::View::Bytes name;
    Perimortem::System::Version version;
  };

  class Record {
   public:
    Record(
        Perimortem::Core::View::Bytes content,
        Perimortem::Core::View::Bytes diagnostic_path);

    auto is_valid() const -> Bool;
    auto get_content() const -> Perimortem::Core::View::Bytes;
    auto get_manifest() const -> const Tetrodotoxin::Archiver::Manifest&;
    auto get_result() const -> const Ttx::Concept::Abstract&;
    auto publish(const Model::Package& package) -> void;
    auto get_arena() -> Perimortem::Memory::Allocator::Arena&;

   private:
    Perimortem::Memory::Allocator::Arena arena;
    Perimortem::Memory::Managed::Bytes content;
    Perimortem::Core::View::Bytes diagnostic_path;
    const Tetrodotoxin::Archiver::Manifest* manifest;
    const Ttx::Concept::Abstract* result;
  };

  auto find(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Version version) -> Record*;
  auto find(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Version version) const -> const Record*;
  auto resolve(
      Record& record,
      Perimortem::Memory::Dynamic::Set<Record*>& active)
      -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Object<Record>>
      records;
  Perimortem::Memory::Dynamic::Map<Key, Record*> records_by_key;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Package
