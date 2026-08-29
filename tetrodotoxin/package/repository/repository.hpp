// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"

namespace Tetrodotoxin::Package::Repository {

// Repository resolves one exact Package coordinate from the writable Terminal
// root first and an optional read only Package root second. It owns no declared
// inventory, target ABI, native cache, provider mapping, or build tool artifact
// policy.
class Repository {
 public:
  enum class Error : U8 {
    Unknown = U8(-1),
    NotDeclared = 0,
    Unreadable,
    InvalidFormat,
    UnsupportedFormat,
    PackageKeyMismatch,
  };

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes terminal_root,
      Perimortem::Core::Option<Perimortem::Core::View::Bytes> package_root = {})
      -> Perimortem::Core::Option<Repository>;

  auto select_source(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version)
      -> Perimortem::Utility::Result<Perimortem::Core::View::Bytes, Error>;

  auto select_archive(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version)
      -> Perimortem::Utility::Result<const Archive::Archive&, Error>;

 private:
  class Source {
   public:
    constexpr Source(
        Perimortem::Core::View::Bytes identity,
        Perimortem::System::Version version,
        Perimortem::Core::View::Bytes root)
        : identity(identity), version(version), root(root) {}

    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
    Perimortem::Core::View::Bytes root;
  };

  Repository(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes terminal_root,
      Perimortem::Core::Option<Perimortem::Core::View::Bytes> package_root)
      : arena(arena),
        terminal_root(terminal_root),
        package_root(package_root),
        sources(arena),
        archives(arena) {}

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Core::View::Bytes terminal_root;
  Perimortem::Core::Option<Perimortem::Core::View::Bytes> package_root;
  Perimortem::Memory::Managed::Vector<Source> sources;
  Perimortem::Memory::Managed::Vector<Archive::Archive> archives;
};

}  // namespace Tetrodotoxin::Package::Repository
