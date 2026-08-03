// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "perimortem/utility/option.hpp"
#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/package/content.hpp"

namespace Tetrodotoxin::Package {

// Opens the physical root for one Package and caches every successful Source
// or resource read by its normalized logical route.
//
// Package::Language::Source owns the semantic name and authored route. Storage
// resolves only that route into a diagnostic path and bytes. It constructs
// Content in the Workspace Arena so staged Source and resource consumers
// retain stable views for the semantic island lifetime, even after Storage
// closes its root.
//
// The cache belongs to this opened Package storage only. Storage never
// interprets content or derives semantic identity from a route.
class Storage {
 public:
  // A failed read keeps only the normalized Path that Storage could establish
  // and the caller decision supported by File Root. Error stays nested because
  // it has no identity or use outside this one result.
  class Failure {
   public:
    enum class Error : Unsigned_8 {
      Unknown = Unsigned_8(-1),
      InvalidRoute = 0,
      Unreadable,
    };

    constexpr Failure(Error error) : error(error) {}

    constexpr Failure(Perimortem::System::Path path, Error error)
        : path(path), error(error) {}

    constexpr auto get_path() const
        -> Perimortem::Utility::Option<const Perimortem::System::Path&> {
      if (path.get_view().is_empty()) {
        return {};
      }

      return path;
    }

    constexpr auto get_error() const -> Error { return error; }

   private:
    Perimortem::System::Path path;
    Error error;
  };

  Storage(const Storage&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;
  Storage(Storage&&) = default;
  auto operator=(Storage&&) -> Storage& = delete;

  static auto open(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes root)
      -> Perimortem::Utility::Option<Storage>;

  auto read(Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Utility::Result<Content&, Failure>;

 private:
  Storage(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::System::File::Root&& root)
      : arena(arena),
        root(static_cast<Perimortem::System::File::Root&&>(root)),
        cache(arena) {}

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::System::File::Root root;
  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Content&>
      cache;
};

}  // namespace Tetrodotoxin::Package
