// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/repository/repository.hpp"

#include "validation/unit_test.hpp"

#include <limits.h>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/version.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness PackageRepository = {
  .name = "Package repository"_view,
};

static auto repository_root(Memory::Allocator::Arena& arena)
    -> Core::View::Bytes {
  char current[PATH_MAX];
  BAIL_IF(getcwd(current, sizeof(current)) == nullptr);
  Memory::Managed::Bytes root(arena, Core::NullTerminated::to_view(current));
  root.concat("/.bin/bin/packages/ttx/perimortem_memory.products"_view);
  return root.get_view();
}

PERIMORTEM_UNIT_TEST(PackageRepository, exact_complete_product) {
  Memory::Allocator::Arena arena;
  auto repository =
      Package::Repository::Repository::create(arena, repository_root(arena));
  ASSERT(repository);

  const Package::Archive::Archive* first = nullptr;
  repository->select_archive("Perimortem.Memory"_view, System::Version(1, 0))
      .visit(
          [&](const Package::Archive::Archive& archive) { first = &archive; },
          [](Package::Repository::Repository::Error) {});
  ASSERT(first);
  EXPECT_TEXT(first->get_identity(), "Perimortem.Memory"_view);
  EXPECT(first->get_version() == System::Version(1, 0));
  const Package::Archive::Archive* repeated = nullptr;
  repository->select_archive("Perimortem.Memory"_view, System::Version(1, 0))
      .visit(
          [&](const Package::Archive::Archive& archive) {
            repeated = &archive;
          },
          [](Package::Repository::Repository::Error) {});
  ASSERT(repeated);
  EXPECT_TEXT(repeated->get_identity(), first->get_identity());
  EXPECT(repeated->get_version() == first->get_version());
}

PERIMORTEM_UNIT_TEST(PackageRepository, missing_coordinate) {
  Memory::Allocator::Arena arena;
  auto repository =
      Package::Repository::Repository::create(arena, repository_root(arena));
  ASSERT(repository);

  Bool missing =
      repository
          ->select_archive("Perimortem.Missing"_view, System::Version(1, 0))
          .visit(
              [](const Package::Archive::Archive&) -> Bool { return False; },
              [](Package::Repository::Repository::Error error) -> Bool {
                return error == Package::Repository::Repository::Error::
                                    NotDeclared
                           ? True
                           : False;
              });
  EXPECT(missing);
}
