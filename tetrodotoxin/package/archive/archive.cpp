// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/archive/archive.hpp"

using namespace Tetrodotoxin;

auto Package::Archive::Archive::matches(const Linker::Manifest& manifest) const
    -> Bool {
  if (manifest.get_identity() != get_identity() ||
      manifest.get_version() != get_version()) {
    return False;
  }

  auto artifacts = get_artifacts();
  for (Count artifact_index = 0; artifact_index < artifacts.get_size();
       artifact_index++) {
    const Artifact& artifact = artifacts.get_data()[artifact_index];
    if (artifact.get_id() != manifest.get_artifact()) {
      continue;
    }
    if (artifact.get_target() != manifest.get_target() ||
        !(artifact.get_fingerprint() == manifest.get_fingerprint())) {
      return False;
    }

    auto expected = artifact.get_imports();
    auto actual = manifest.get_imports();
    if (expected.get_size() != actual.get_size()) {
      return False;
    }
    for (Count import_index = 0; import_index < expected.get_size();
         import_index++) {
      if (!(expected.get_data()[import_index] ==
            actual.get_data()[import_index])) {
        return False;
      }
    }
    return True;
  }
  return False;
}
