// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/source/cache.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer::Resolution;

auto Source::Cache::reset() -> void {
  while (records.get_size() != 0) {
    // Every published import name points at a valid record. Removing the first
    // map entry drains the cache because import names are the only cache keys.
    remove(*records.get_entry(0)->value);
  }

  consumers_by_producer.clear();
  producers_by_consumer.clear();
}

auto Source::Cache::find(View::Bytes key) -> Source::Record* {
  auto* entry = records.find(key);
  return entry == nullptr ? nullptr : &*entry->value;
}

auto Source::Cache::find(View::Bytes key) const -> const Source::Record* {
  const auto* entry = records.find(key);
  return entry == nullptr ? nullptr : &*entry->value;
}

auto Source::Cache::publish(Dynamic::Object<Record>& record) -> Record& {
  Record& published = *record;

  Record* current = find(published.get_import_name());
  if (current != nullptr && current != &published) {
    remove(*current);
  }

  records.insert(published.get_import_name(), record);
  consumers_by_producer.at(&published);
  producers_by_consumer.at(&published);

  return published;
}

auto Source::Cache::remove(View::Bytes key) -> void {
  auto* entry = records.find(key);
  if (entry == nullptr) {
    return;
  }

  remove(*entry->value);
}

auto Source::Cache::connect(Record& consumer, Record& producer) -> void {
  consumers_by_producer.find(&producer)->value.insert(&consumer);
  producers_by_consumer.find(&consumer)->value.insert(&producer);
}

auto Source::Cache::collect_consumers(
    const Record& record,
    Dynamic::Vector<Record*>& consumers) const -> void {
  const auto* entry = consumers_by_producer.find(&record);
  entry->value.visit([&](Record* consumer) -> void {
    if (consumers.contains(consumer)) {
      return;
    }

    consumers.insert(consumer);
    collect_consumers(*consumer, consumers);
  });
}

auto Source::Cache::collect_removal_plan(
    Record& record,
    Dynamic::Vector<Record*>& records) const -> void {
  if (records.contains(&record)) {
    return;
  }
  records.insert(&record);

  const auto* entry = consumers_by_producer.find(&record);
  entry->value.visit([&](Record* consumer) -> void {
    collect_removal_plan(*consumer, records);
  });
}

auto Source::Cache::remove(Record& record) -> void {
  Dynamic::Vector<Record*> removal_plan;
  collect_removal_plan(record, removal_plan);

  // Cache records are address identities. A replaced producer invalidates every
  // transitive consumer before any owning handle is released, because consumer
  // facts can point into producer arenas. The plan keeps those pointers stable
  // while dependency indexes and public lookup keys are erased.
  for (Count i = 0; i < removal_plan.get_size(); i++) {
    detach(*removal_plan[i]);
  }

  for (Count i = 0; i < removal_plan.get_size(); i++) {
    Record& removed = *removal_plan[i];
    consumers_by_producer.remove(&removed);
    producers_by_consumer.remove(&removed);
    records.remove(removed.get_import_name());
  }
}

auto Source::Cache::detach(Record& record) -> void {
  // Dependency edges are inserted into both maps by `connect`. The reciprocal
  // entry is part of the cache invariant, so detach only removes edges from the
  // opposite sets. Record entries are removed once the whole invalidation plan
  // is detached.
  producers_by_consumer.find(&record)->value.visit([&](Record* producer) {
    consumers_by_producer.find(producer)->value.remove(&record);
  });

  consumers_by_producer.find(&record)->value.visit([&](Record* consumer) {
    producers_by_consumer.find(consumer)->value.remove(&record);
  });
}
