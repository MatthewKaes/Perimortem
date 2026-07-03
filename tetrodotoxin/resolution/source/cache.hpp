// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/resolution/source/record.hpp"

namespace Tetrodotoxin::Resolution::Source {

// Resolver's source cache with its dependency graph.
//
// Only valid records are published to the cache and come in two major flavors:
// concrete source path for reloads, and package name for package imports. The
// cache keeps a graph keyed on View::Bytes to work with both types. The graph
// is used to track which consumers need to be detatched on source change for
// quick graph invalidation. Since TTX's Type and Data model is address based
// consumers also keep track of their producers so they can clean up the list
// when they are invalidated.
class Cache {
 public:
  Cache() = default;
  Cache(const Cache&) = delete;
  auto operator=(const Cache&) -> Cache& = delete;
  ~Cache() { reset(); }

  auto reset() -> void;
  auto find(Perimortem::Core::View::Bytes key) -> Record*;
  auto find(Perimortem::Core::View::Bytes key) const -> const Record*;
  auto publish(Record& record) -> Record&;
  auto remove(Perimortem::Core::View::Bytes key) -> void;
  auto connect(Record& consumer, Record& producer) -> void;

  // Collects every published source that directly or indirectly imports the
  // selected record.
  auto collect_transitive_consumers(
      const Record& record,
      Perimortem::Memory::Dynamic::Vector<Record*>& consumers) const -> void;

 private:
  using Records = Perimortem::Memory::Dynamic::Vector<Record*>;

  auto remove(Record& record) -> void;
  auto detach(Record& record) -> void;

  Perimortem::Memory::Dynamic::Map<Perimortem::Core::View::Bytes, Record*>
      records;
  // producer -> consumers map for invalidation and recursive removal.
  Perimortem::Memory::Dynamic::Map<const Record*, Records>
      consumers_by_producer;
  // consumer -> producers map to detach without scanning unrelated records.
  Perimortem::Memory::Dynamic::Map<const Record*, Records>
      producers_by_consumer;
};

}  // namespace Tetrodotoxin::Resolution::Source
