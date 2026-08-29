// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/repository/repository.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/package/archive/reader.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static auto valid_coordinate(
    Core::View::Bytes identity,
    System::Version version) -> Bool {
  if (identity.is_empty() || version.is_null() || identity == "."_view ||
      identity == ".."_view) {
    return False;
  }
  for (Count index = 0; index < identity.get_size(); index++) {
    BAIL_IF(identity[index] == '/');
  }
  return True;
}

static auto coordinate_root(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes root,
    Core::View::Bytes identity,
    System::Version version) -> Core::View::Bytes {
  Memory::Managed::Bytes path(arena, root);
  if (path.get_size() != 0 && path[path.get_size() - 1] != '/') {
    path.append('/');
  }
  path.concat(identity);
  path.append('/');
  Serialization::Stream::Textual<Memory::Managed::Bytes> output(path);
  output << version.get_major() << "."_view << version.get_minor();
  auto normalized = System::Path::normalize(arena, path.get_view());
  return normalized ? *normalized : Core::View::Bytes();
}

auto Package::Repository::Repository::create(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes terminal_root,
    Core::Option<Core::View::Bytes> package_root) -> Core::Option<Repository> {
  auto normalized = System::Path::normalize(arena, terminal_root);
  BAIL_IF(
      !normalized || normalized->is_empty() ||
      !System::Path(*normalized).is_rooted());
  Core::Option<Core::View::Bytes> normalized_package;
  if (package_root) {
    auto selected = System::Path::normalize(arena, *package_root);
    BAIL_IF(
        !selected || selected->is_empty() ||
        !System::Path(*selected).is_rooted());
    normalized_package = *selected;
  }
  return Repository(arena, *normalized, normalized_package);
}

static auto select_coordinate_root(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes terminal_root,
    Core::Option<Core::View::Bytes> package_root,
    Core::View::Bytes identity,
    System::Version version,
    Core::View::Bytes file) -> Core::View::Bytes {
  Core::View::Bytes roots[2] = {
    terminal_root,
    package_root ? *package_root : Core::View::Bytes(),
  };
  for (Core::View::Bytes root : roots) {
    if (root.is_empty()) {
      continue;
    }
    Core::View::Bytes selected =
        coordinate_root(arena, root, identity, version);
    if (selected.is_empty()) {
      continue;
    }
    Memory::Managed::Bytes path(arena, selected);
    path.append('/');
    path.concat(file);
    if (System::File::exists(path.get_view())) {
      return selected;
    }
  }
  return {};
}

auto Package::Repository::Repository::select_source(
    Core::View::Bytes identity,
    System::Version version) -> Utility::Result<Core::View::Bytes, Error> {
  if (!valid_coordinate(identity, version)) {
    return Error::NotDeclared;
  }
  for (const Source& source : sources.get_view()) {
    if (source.identity == identity && source.version == version) {
      return source.root;
    }
  }

  Core::View::Bytes selected = select_coordinate_root(
      arena, terminal_root, package_root, identity, version,
      "package.ttx"_view);
  if (selected.is_empty()) {
    return Error::NotDeclared;
  }
  Core::View::Bytes retained = arena.proxy(selected);
  sources.insert(Source(arena.proxy(identity), version, retained));
  return retained;
}

auto Package::Repository::Repository::select_archive(
    Core::View::Bytes identity,
    System::Version version)
    -> Utility::Result<const Package::Archive::Archive&, Error> {
  if (!valid_coordinate(identity, version)) {
    return Error::NotDeclared;
  }
  for (Count index = 0; index < archives.get_size(); index++) {
    const Package::Archive::Archive& archive = archives[index];
    if (archive.get_identity() == identity &&
        archive.get_version() == version) {
      return archive;
    }
  }

  Core::View::Bytes selected = select_coordinate_root(
      arena, terminal_root, package_root, identity, version,
      "package.ttxp"_view);
  if (selected.is_empty()) {
    return Error::NotDeclared;
  }
  Memory::Managed::Bytes product(arena, selected);
  product.concat("/package.ttxp"_view);
  auto bytes = System::File::read(arena, product.get_view());
  if (!bytes) {
    return Error::Unreadable;
  }

  return Package::Archive::Reader::read(arena, *bytes)
      .visit(
          [&](Package::Archive::Archive& archive)
              -> Utility::Result<const Package::Archive::Archive&, Error> {
            if (archive.get_identity() != identity ||
                archive.get_version() != version) {
              return Error::PackageKeyMismatch;
            }
            return archives.emplace(Package::Archive::Archive(archive));
          },
          [](Package::Archive::Reader::Error error)
              -> Utility::Result<const Package::Archive::Archive&, Error> {
            return error == Package::Archive::Reader::Error::UnsupportedFormat
                       ? Error::UnsupportedFormat
                       : Error::InvalidFormat;
          });
}
