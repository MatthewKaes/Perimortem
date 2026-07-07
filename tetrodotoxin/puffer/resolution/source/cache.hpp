// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/object.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/puffer/resolution/source/record.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Resolver source cache with the dependency indexes needed for invalidation.
//
// Only valid records are published here. Records are addressed by import name:
// file sources use their normalized file path, while packages use their
// declared package name.
//
// The two dependency maps are indexes over the producer / consumer
// relationships between clusters: producers let a changed record find its
// consumers, and consumers let a removed record detach from its producers
// without scanning unrelated sources. Published records always have entries in
// both maps; an empty dependency set is represented by an empty set, not by a
// missing map entry.
//
// That matters because TTX facts are address identities rather than the prior
// GUID model. If a producer record is destroyed or republished, every consumer
// that may point into its arena has to leave the cache or be evaluated again.
class Cache {
 public:
  Cache() = default;
  Cache(const Cache&) = delete;
  auto operator=(const Cache&) -> Cache& = delete;
  ~Cache() { reset(); }

  auto reset() -> void;
  auto find(Perimortem::Core::View::Bytes key) -> Record*;
  auto find(Perimortem::Core::View::Bytes key) const -> const Record*;
  auto publish(Perimortem::Memory::Dynamic::Object<Record>& record) -> Record&;
  auto remove(Perimortem::Core::View::Bytes key) -> void;
  auto remove(Record& record) -> void;
  auto connect(Record& consumer, Record& producer) -> void;

  // Collects every published source that directly or indirectly imports this
  // record.
  auto collect_consumers(
      const Record& record,
      Perimortem::Memory::Dynamic::Vector<Record*>& consumers) const -> void;

  template <typename visitor_type>
  auto visit_producers(const Record& record, visitor_type visit) const -> void {
    const auto* entry = producers_by_consumer.find(&record);
    entry->value.visit([&](Record* producer) -> void {
      visit(*producer);
      visit_producers(*producer, visit);
    });
  }

 private:
  using Records = Perimortem::Memory::Dynamic::Set<Record*>;

  auto collect_removal_plan(
      Record& record,
      Perimortem::Memory::Dynamic::Vector<Record*>& records) const -> void;
  auto detach(Record& record) -> void;

  // Import name to record. This is the public resolver lookup surface and owns
  // published records.
  Perimortem::Memory::Dynamic::Map<
      Perimortem::Core::View::Bytes,
      Perimortem::Memory::Dynamic::Object<Record>>
      records;
  // Producer to consumers. Used when a changed record needs to clear its users.
  Perimortem::Memory::Dynamic::Map<const Record*, Records>
      consumers_by_producer;
  // Consumer to producers. Used when a removed record detaches its imports.
  Perimortem::Memory::Dynamic::Map<const Record*, Records>
      producers_by_consumer;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
