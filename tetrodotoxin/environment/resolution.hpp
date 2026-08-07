// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/environment/dialects.hpp"
#include "tetrodotoxin/environment/retention.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"

namespace Tetrodotoxin::Environment {

// Resolves exact Package dependencies into one Workspace semantic island. It
// owns traversal and restored Package cache state while borrowing the Arena,
// installed Dialects, and retained Monographs that supply graph lifetime.
class Resolution {
 public:
  Resolution(
      Perimortem::Memory::Allocator::Arena& arena,
      Dialects& dialects,
      Retention& retention);

  // Resolves one fully staged Package import. Every return selects either the
  // completed root or a failure category, so the public transaction has no
  // untyped empty state.
  auto resolve(
      Ttx::Lexical::Errors& errors,
      Count first_monograph,
      Perimortem::Core::View::Bytes root_package_identity,
      Perimortem::System::Version root_package_version,
      Package::Language::Monograph& root_package,
      Package::Repository::Repository& repository) -> Perimortem::Utility::
      Result<Language::Monograph&, Package::Repository::SelectionError>;

 private:
  struct RestoredPackage {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
    Package::Language::Monograph& value;
    Bool ready;
  };

  Perimortem::Memory::Allocator::Arena& arena;
  Dialects& dialects;
  Retention& retention;
  Perimortem::Memory::Managed::Vector<RestoredPackage> restored_packages;
};

}  // namespace Tetrodotoxin::Environment
