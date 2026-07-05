// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/source/cache.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer::Resolution;

static auto insert_once(
    Dynamic::Vector<Source::Record*>& records,
    Source::Record& record) -> void {
  if (!records.contains(&record)) {
    records.insert(&record);
  }
}

static auto remove_record(
    Dynamic::Vector<Source::Record*>& records,
    const Source::Record& record) -> void {
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i] == &record) {
      records.remove(i);
      return;
    }
  }
}

auto Source::Cache::reset() -> void {
  while (records.get_size() != 0) {
    remove(*records.get_entry(0)->value);
  }

  consumers_by_producer.clear();
  producers_by_consumer.clear();
}

auto Source::Cache::find(View::Bytes key) -> Source::Record* {
  return const_cast<Record*>(static_cast<const Cache*>(this)->find(key));
}

auto Source::Cache::find(View::Bytes key) const -> const Source::Record* {
  const auto* entry = records.find(key);
  return entry == nullptr ? nullptr : entry->value;
}

auto Source::Cache::publish(Record& record) -> Record& {
  Record* current = find(record.get_source_path());
  if (current != nullptr && current != &record) {
    remove(*current);
  }

  current = find(record.get_import_name());
  if (current != nullptr && current != &record) {
    remove(*current);
  }

  records.insert(record.get_source_path(), &record);
  if (record.get_import_name() != record.get_source_path()) {
    records.insert(record.get_import_name(), &record);
  }

  return record;
}

auto Source::Cache::remove(View::Bytes key) -> void {
  auto* entry = records.find(key);
  if (entry == nullptr) {
    return;
  }

  remove(*entry->value);
}

auto Source::Cache::connect(Record& consumer, Record& producer) -> void {
  insert_once(consumers_by_producer.at(&producer), consumer);
  insert_once(producers_by_consumer.at(&consumer), producer);
}

auto Source::Cache::collect_transitive_consumers(
    const Record& record,
    Dynamic::Vector<Record*>& consumers) const -> void {
  const auto* entry = consumers_by_producer.find(&record);
  if (entry == nullptr) {
    return;
  }

  for (Count i = 0; i < entry->value.get_size(); i++) {
    Record* consumer = entry->value[i];
    if (consumers.contains(consumer)) {
      continue;
    }

    consumers.insert(consumer);
    collect_transitive_consumers(*consumer, consumers);
  }
}

auto Source::Cache::collect_transitive_producers(
    const Record& record,
    Dynamic::Vector<Record*>& producers) const -> void {
  const auto* entry = producers_by_consumer.find(&record);
  if (entry == nullptr) {
    return;
  }

  for (Count i = 0; i < entry->value.get_size(); i++) {
    Record* producer = entry->value[i];
    if (producers.contains(producer)) {
      continue;
    }

    producers.insert(producer);
    collect_transitive_producers(*producer, producers);
  }
}

auto Source::Cache::remove(Record& record) -> void {
  while (true) {
    auto* entry = consumers_by_producer.find(&record);
    if (entry == nullptr || entry->value.get_size() == 0) {
      break;
    }

    remove(*entry->value[0]);
  }

  detach(record);
  records.remove(record.get_source_path());
  if (record.get_import_name() != record.get_source_path()) {
    records.remove(record.get_import_name());
  }
  Record::destroy(record);
}

auto Source::Cache::detach(Record& record) -> void {
  auto* producers_entry = producers_by_consumer.find(&record);
  if (producers_entry != nullptr) {
    Records producers = producers_entry->value;
    producers_by_consumer.remove(&record);

    for (Count i = 0; i < producers.get_size(); i++) {
      auto* consumers_entry = consumers_by_producer.find(producers[i]);
      if (consumers_entry == nullptr) {
        continue;
      }

      remove_record(consumers_entry->value, record);
      if (consumers_entry->value.get_size() == 0) {
        consumers_by_producer.remove(producers[i]);
      }
    }
  }

  auto* consumers_entry = consumers_by_producer.find(&record);
  if (consumers_entry != nullptr) {
    Records consumers = consumers_entry->value;
    consumers_by_producer.remove(&record);

    for (Count i = 0; i < consumers.get_size(); i++) {
      auto* consumer_producers =
          producers_by_consumer.find(consumers[i]);
      if (consumer_producers == nullptr) {
        continue;
      }

      remove_record(consumer_producers->value, record);
      if (consumer_producers->value.get_size() == 0) {
        producers_by_consumer.remove(consumers[i]);
      }
    }
  }
}
