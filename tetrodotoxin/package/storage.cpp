// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/storage.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

static auto normalize_logical_route(View::Bytes logical_route) -> Option<Path> {
  if (logical_route.is_empty()) {
    return {};
  }

  for (Count i = 0; i < logical_route.get_size(); i++) {
    if (logical_route[i] == '\0') {
      return {};
    }
  }

  Path normalized(logical_route);
  if (normalized.get_view().is_empty() || normalized.is_rooted()) {
    return {};
  }

  return normalized;
}

auto Package::Storage::open(Allocator::Arena& arena, View::Bytes location)
    -> Option<Storage> {
  return File::Root::open(location).visit(
      []() { return Option<Storage>(); },
      [&arena](File::Root& root) {
        return Option<Storage>(Storage(arena, Data::take(root)));
      });
}

auto Package::Storage::read(View::Bytes logical_route) -> Option<Content&> {
  auto normalized = normalize_logical_route(logical_route);
  if (!normalized) {
    return {};
  }

  View::Bytes diagnostic_path = (*normalized).get_view();
  auto cached = cache.find(diagnostic_path);
  if (cached) {
    return cached->value;
  }

  auto contents = root.read(arena, diagnostic_path);
  if (!contents) {
    return {};
  }

  // Construct the Content with stabalized lifetime in the Arena. We can then
  // launder the address into the cache to enforce the cache only containing
  // valid items. Since the Map and content share the same Arena we this will
  // always be true, but it requires us to also proxy the `diagnostic_path` to
  // ensure all data members share the same lifetime.
  View::Bytes retained_path = arena.proxy(diagnostic_path);
  Content& retained = arena.construct<Content>(retained_path, *contents);
  cache.launder(retained_path, retained);
  return retained;
}
