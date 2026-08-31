// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/snapshots.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.h"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;

static constexpr Count maximum_snapshot_key = Path::max_size * 2 + 1;

static auto make_key(
    Static::Bytes<maximum_snapshot_key>& storage,
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<View::Bytes> {
  Path root(package_root);
  Path route(logical_route);
  View::Bytes normalized_root = root.get_view();
  View::Bytes normalized_route = route.get_view();
  if (normalized_root.is_empty() || normalized_route.is_empty() ||
      route.is_rooted()) {
    return {};
  }

  Count required = normalized_root.get_size() + normalized_route.get_size() + 1;
  if (required > storage.get_size()) {
    return {};
  }

  Data::copy(
      storage.get_data(), normalized_root.get_data(),
      normalized_root.get_size());
  storage[normalized_root.get_size()] = '\0';
  Data::copy(
      storage.get_data() + normalized_root.get_size() + 1,
      normalized_route.get_data(), normalized_route.get_size());
  return View::Bytes(storage.get_data(), required);
}

Package::Snapshots::~Snapshots() {
  StoredEntry* entries = Data::cast<StoredEntry>(entry_storage.data);
  for (Count index = 0; index < entry_count; ++index) {
    entries[index].~StoredEntry();
  }

  perimortem_hash_index_release(&entry_index);
  perimortem_aligned_buffer_release(&entry_storage);
}

auto Package::Snapshots::ensure_capacity(Count required) -> Bool {
  if (required <= entry_capacity) {
    return True;
  }

  Count requested = entry_capacity == 0 ? 8 : entry_capacity * 2;
  if (requested < required || requested > Count(-1) / sizeof(StoredEntry)) {
    requested = required;
  }
  BAIL_IF(requested > Count(-1) / sizeof(StoredEntry));

  perimortem_aligned_buffer replacement = {};
  BAIL_IF(!perimortem_aligned_buffer_create(
      requested * sizeof(StoredEntry), alignof(StoredEntry), &replacement));

  StoredEntry* previous = Data::cast<StoredEntry>(entry_storage.data);
  StoredEntry* next = Data::cast<StoredEntry>(replacement.data);
  for (Count index = 0; index < entry_count; ++index) {
    new (next + index, Placement::Construct) StoredEntry{
      previous[index].key,
      static_cast<Entry&&>(previous[index].value),
    };
    previous[index].~StoredEntry();
  }

  perimortem_aligned_buffer_release(&entry_storage);
  entry_storage = replacement;
  entry_capacity = replacement.capacity / sizeof(StoredEntry);
  return True;
}

auto Package::Snapshots::find(
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<Entry&> {
  Static::Bytes<maximum_snapshot_key> storage;
  auto key = make_key(storage, package_root, logical_route);
  BAIL_IF(!key);

  const U64 hash = perimortem_hash_bytes({key->get_data(), key->get_size()});
  perimortem_hash_index_match match = {};
  StoredEntry* entries = Data::cast<StoredEntry>(entry_storage.data);
  Bool found = perimortem_hash_index_find_first(&entry_index, hash, &match);
  while (found) {
    if (match.entry < entry_count && entries[match.entry].key == *key) {
      return entries[match.entry].value;
    }

    found = perimortem_hash_index_find_next(&entry_index, hash, &match);
  }

  return {};
}

auto Package::Snapshots::create(
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<Entry&> {
  Static::Bytes<maximum_snapshot_key> storage;
  auto key = make_key(storage, package_root, logical_route);
  BAIL_IF(!key);

  auto existing = find(package_root, logical_route);
  if (existing) {
    return existing;
  }

  BAIL_IF(!ensure_capacity(entry_count + 1));

  View::Bytes retained_key = arena.proxy(*key);
  StoredEntry* entries = Data::cast<StoredEntry>(entry_storage.data);
  new (entries + entry_count, Placement::Construct)
      StoredEntry{retained_key, Entry()};
  const U64 hash =
      perimortem_hash_bytes({retained_key.get_data(), retained_key.get_size()});
  if (!perimortem_hash_index_insert(&entry_index, hash, entry_count)) {
    entries[entry_count].~StoredEntry();
    return {};
  }

  return entries[entry_count++].value;
}

auto Package::Snapshots::read(
    const File::Root& root,
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<View::Bytes> {
  auto selected = find(package_root, logical_route);
  if (selected && selected->overlaid) {
    return selected->overlay_contents.get_view();
  }

  if (selected && selected->loaded) {
    auto fingerprint = root.fingerprint(logical_route);
    if (fingerprint && *fingerprint == selected->fingerprint) {
      return selected->contents.get_view();
    }
  }

  auto snapshot = root.read_snapshot(logical_route);
  if (!snapshot) {
    return {};
  }

  if (!selected) {
    selected = create(package_root, logical_route);
  }
  BAIL_IF(!selected);

  selected->contents = snapshot->take_contents();
  selected->fingerprint = snapshot->get_fingerprint();
  selected->loaded = True;
  return selected->contents.get_view();
}

auto Package::Snapshots::overlay(
    View::Bytes package_root,
    View::Bytes logical_route,
    View::Bytes contents) -> Bool {
  auto selected = find(package_root, logical_route);
  if (!selected) {
    selected = create(package_root, logical_route);
  }
  BAIL_IF(!selected);

  selected->overlay_contents = contents;
  selected->overlaid = True;
  return True;
}

auto Package::Snapshots::remove_overlay(
    View::Bytes package_root,
    View::Bytes logical_route) -> Bool {
  auto selected = find(package_root, logical_route);
  BAIL_IF(!selected || !selected->overlaid);

  selected->overlay_contents.clear();
  selected->overlaid = False;
  return True;
}
