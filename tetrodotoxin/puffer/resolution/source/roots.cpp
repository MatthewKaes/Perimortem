// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/source/roots.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Puffer;

auto Resolution::Source::Roots::include(View::Bytes source_path) -> Bool {
  if (Algorithm::search(source_path, '\\') != Count(-1)) {
    return False;
  }

  Path path(source_path);
  View::Bytes root = path.get_directory();
  if (root.is_empty()) {
    reset();
    root_count = 1;
    return True;
  }

  for (Count i = 0; i < root_count; i++) {
    View::Bytes existing = roots[i].get_view();
    if (existing.is_empty()) {
      return True;
    }

    View::Bytes shared = common(existing, root);
    if (shared == existing) {
      return True;
    }

    if (!shared.is_empty()) {
      roots[i] = shared;
      return True;
    }
  }

  if (root_count >= roots.get_size()) {
    return False;
  }

  roots[root_count] = root;
  root_count++;
  return True;
}

auto Resolution::Source::Roots::contains(View::Bytes source_path) const
    -> Bool {
  for (Count i = 0; i < root_count; i++) {
    View::Bytes root = roots[i].get_view();
    if (root.is_empty() || source_path == root ||
        (source_path.get_size() > root.get_size() &&
         source_path.slice(0, root.get_size()) == root &&
         source_path[root.get_size()] == '/')) {
      return True;
    }
  }

  return False;
}

auto Resolution::Source::Roots::reset() -> void {
  for (Count i = 0; i < root_count; i++) {
    roots[i].clear();
  }

  root_count = 0;
}

auto Resolution::Source::Roots::common(View::Bytes left, View::Bytes right)
    -> View::Bytes {
  Count common_size = 0;
  Count limit =
      left.get_size() < right.get_size() ? left.get_size() : right.get_size();
  for (Count i = 0; i < limit && left[i] == right[i]; i++) {
    if (left[i] == '/') {
      common_size = i;
    }

    Bool left_root = left.get_size() == limit &&
                     (right.get_size() == limit || right[limit] == '/');
    Bool right_root = right.get_size() == limit &&
                      (left.get_size() == limit || left[limit] == '/');
    if (i + 1 == limit && (left_root || right_root)) {
      common_size = limit;
    }
  }

  return left.slice(0, common_size);
}
