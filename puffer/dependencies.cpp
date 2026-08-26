// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/dependencies.hpp"

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/language/persistence/profile.hpp"
#include "tetrodotoxin/package/archive/graph_import.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Puffer::Dependencies::contains(
    Core::View::Bytes identity,
    System::Version version) const -> Bool {
  return archives.get_view().contains(
      [&](const Package::Archive::Archive& archive) {
        return archive.get_identity() == identity &&
               archive.get_version() == version;
      });
}

auto Puffer::Dependencies::retain(const Package::Archive::Archive& archive)
    -> Bool {
  BAIL_IF(archive.get_profile() != Language::Persistence::Profile::Contract);

  for (const Package::Archive::Archive& retained : archives.get_view()) {
    BAIL_IF(retained.get_identity() == archive.get_identity());
  }

  archives.insert(archive);
  return True;
}

auto Puffer::Dependencies::acquire(
    Core::View::Bytes identity,
    System::Version version) -> Bool {
  if (contains(identity, version)) {
    return True;
  }

  for (const Package::Archive::Archive& archive : archives.get_view()) {
    BAIL_IF(archive.get_identity() == identity);
  }

  for (const Coordinate& coordinate : active.get_view()) {
    BAIL_IF(coordinate.identity == identity && coordinate.version == version);
  }

  Core::Option<const Package::Archive::Archive&> selected;
  repository.select_archive(identity, version)
      .visit(
          [&](const Package::Archive::Archive& archive) { selected = archive; },
          [](Package::Repository::Repository::Error) {});
  BAIL_IF(
      !selected ||
      selected->get_profile() != Language::Persistence::Profile::Contract);

  active.insert(Coordinate(identity, version));
  Bool complete = True;

  // Archive coordinates carry enough Package meaning to order their complete
  // Contract closure before a fresh Workspace receives any restored graph.
  for (const Package::Archive::GraphImport& import : selected->get_imports()) {
    if (import.get_kind() == Language::Import::Kind::Package) {
      complete &= acquire(import.get_target(), import.get_version());
    }
  }

  for (const Package::Language::Dependency& dependency :
       selected->get_dependencies()) {
    complete &=
        acquire(dependency.get_package_name(), dependency.get_version());
  }

  active.remove(active.get_size() - 1);
  if (complete) {
    archives.insert(*selected);
  }

  return complete;
}
