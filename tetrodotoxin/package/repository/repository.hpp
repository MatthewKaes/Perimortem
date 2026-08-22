// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/repository/input.hpp"
#include "tetrodotoxin/package/repository/output.hpp"

namespace Tetrodotoxin::Package::Repository {

// Selects only products supplied by the calling Bazel action. Discovery and
// version preference would make the result depend on ambient repository state,
// so every semantic key, physical input, and publication route is explicit.
class Repository {
 public:
  // Names the stable caller decision for one rejected Repository selection.
  // Repository keeps declaration and selection details in its Info record
  // because those facts explain the failure without changing recovery policy.
  enum class Error : U8 {
    Unknown = U8(-1),
    NotDeclared = 0,
    Unreadable,
    InvalidFormat,
    UnsupportedFormat,
    PackageKeyMismatch,
    ArtifactMismatch,
    AbiMismatch,
    ArtifactNotDeclared,
  };

  // The Arena establishes the lifetime promised by all borrowed declarations.
  // Repository does not copy them automatically because callers may already
  // hold Arena stable or cache stable views. Only normalized routes created by
  // this operation require new storage.
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Input> inputs,
      Perimortem::Core::View::Vector<Output> archive_outputs,
      Perimortem::Core::View::Vector<Output> native_outputs)
      -> Perimortem::Core::Option<Repository>;

  // Selects the exact declared semantic Archive and preserves it in the
  // Repository cache. Every call chooses the Archive or a stable caller error,
  // while Reader retains the format detail that only it can explain.
  auto select_archive(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version)
      -> Perimortem::Utility::Result<const Archive::Archive&, Error>;

  // Semantic selection succeeds without native declarations. This operation
  // adds the complete native inventory check before exposing one borrowed
  // path, keeping native bytes with their eventual format consumer.
  auto select_native(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes artifact_id)
      -> Perimortem::Utility::Result<Perimortem::Core::View::Bytes, Error>;

  // The shared Output value does not erase product kind. Archive lookup stays
  // on its own inventory and cannot fall through to a native declaration.
  auto get_archive_output_path(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes artifact_id) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  // Native lookup uses the same complete key while remaining on its own
  // inventory. A native route therefore cannot substitute for an Archive.
  auto get_native_output_path(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes artifact_id) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

 private:
  Repository(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Input> inputs,
      Perimortem::Core::View::Vector<Output> archive_outputs,
      Perimortem::Core::View::Vector<Output> native_outputs)
      : arena(arena),
        inputs(inputs),
        archive_outputs(archive_outputs),
        native_outputs(native_outputs),
        archive_cache(arena) {}

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Core::View::Vector<Input> inputs;
  Perimortem::Core::View::Vector<Output> archive_outputs;
  Perimortem::Core::View::Vector<Output> native_outputs;
  Perimortem::Memory::Managed::Vector<Archive::Archive> archive_cache;
};

}  // namespace Tetrodotoxin::Package::Repository
