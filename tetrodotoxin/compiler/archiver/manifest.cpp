// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/manifest.hpp"

#include "perimortem/memory/managed/map.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Archiver;

auto Manifest::is_valid_name(View::Bytes name) -> Bool {
  if (name.is_empty()) {
    return False;
  }

  Bool segment_start = True;
  for (Count i = 0; i < name.get_size(); i++) {
    Unsigned_8 byte = name[i];
    if (byte == '.') {
      // Guard against `..` as well as catching any trailing dots.
      if (segment_start || i + 1 == name.get_size()) {
        return False;
      }

      segment_start = True;
      continue;
    }

    // Since segments must be Lexical Type names they should always start with a
    // capital ascii letter.
    if (segment_start) {
      if (byte < 'A' || byte > 'Z') {
        return False;
      }

      segment_start = False;
      continue;
    }

    // The rest of the characters fall into a simple Type valid character check.
    Bool letter = (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z');
    Bool digit = byte >= '0' && byte <= '9';
    if (!letter && !digit && byte != '_') {
      return False;
    }
  }

  return True;
}

auto Manifest::is_valid() const -> Bool {
  Perimortem::Memory::Allocator::Arena arena;
  return is_valid(arena);
}

auto Manifest::is_valid(Perimortem::Memory::Allocator::Arena& arena) const
    -> Bool {
  // Package identity must distinguish an authored version from the null value.
  if (!is_valid_name(name) || version.is_null()) {
    return False;
  }

  Perimortem::Memory::Managed::Map<Dependency, Bool> identities(arena);
  identities.ensure_capacity(dependencies.get_size());
  for (Count i = 0; i < dependencies.get_size(); i++) {
    const Dependency& dependency = dependencies[i];
    if (!is_valid_name(dependency.get_name()) ||
        dependency.get_version().is_null() ||
        identities.find(dependency) != nullptr) {
      return False;
    }

    identities.insert(dependency, True);
  }

  return True;
}
