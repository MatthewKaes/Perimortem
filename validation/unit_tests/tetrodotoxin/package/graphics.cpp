// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/graphics/hosting.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness StandardGraphicsPackage = {
  .name = "Tetrodotoxin::Package::Graphics"_view,
};

PERIMORTEM_UNIT_TEST(StandardGraphicsPackage, restores_host_contract) {
  auto product = File::read(
      ".bin/bin/packages/ttx/Perimortem.Graphics/1.0/contract.txa"_view);
  ASSERT(product);
  Allocator::Arena archive_arena;
  auto decoded = Package::Archive::Reader::read(archive_arena, *product);
  Option<Package::Archive::Archive> archive;
  decoded.visit(
      [&](const Package::Archive::Archive& selected) { archive = selected; },
      [](const Package::Archive::Reader::Error&) {});
  ASSERT(archive);

  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Package::Dialect>("Package"_view));
  ASSERT(toolchain.install<Library::Dialect>("Library"_view));
  Environment::Workspace workspace(toolchain);
  auto restored = workspace.restore_package(*archive, "Graphics"_view);
  ASSERT(restored && restored->is<Package::Language::Monograph>());
  const auto& package =
      static_cast<const Package::Language::Monograph&>(*restored);
  auto member = package.resolve_context("Hosting"_view)
                    .resolve()
                    .select<Library::Language::Monograph>();
  ASSERT(member);

  const Ttx::Concept::Abstract& host =
      member->resolve_context("Host"_view).resolve();
  const Ttx::Concept::Abstract& group =
      member->resolve_context("Group"_view).resolve();
  Graphics::Hosting hosting;
  EXPECT(hosting.accepts(host, group));
  EXPECT_TEXT(
      host.get_documentation().get_line(0),
      "Host is a semantic requirement rather than an allocated node Type."_view);
  EXPECT_TEXT(
      group.get_documentation().get_line(0),
      "Group is the minimal hosted Object."_view);
}
