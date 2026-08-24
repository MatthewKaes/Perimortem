// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/graphics/hosting.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
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

static auto select_type(
    const Package::Language::Monograph& package,
    View::Bytes name) -> Option<const Library::Language::Types::Composite&> {
  auto member = package.resolve_context(name)
                    .resolve()
                    .select<Library::Language::Monograph>();
  BAIL_IF(!member);
  return member->resolve_context(name)
      .resolve()
      .select<Library::Language::Types::Composite>();
}

static auto has_callable(
    const Library::Language::Types::Composite& type,
    View::Bytes name) -> Bool {
  for (const auto& callable :
       type.get_callables(Tetrodotoxin::Language::Visibility::Public)) {
    if (callable.get().get_name() == name) {
      return True;
    }
  }
  return False;
}

PERIMORTEM_UNIT_TEST(StandardGraphicsPackage, restores_public_api) {
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
  auto pixel = select_type(package, "Pixel"_view);
  auto point = select_type(package, "Point2D"_view);
  auto size = select_type(package, "Size2D"_view);
  auto tone = select_type(package, "Tone"_view);
  auto transform = select_type(package, "Transform2D"_view);
  auto host = select_type(package, "Host"_view);
  auto image = select_type(package, "Image"_view);
  auto sprite = select_type(package, "Sprite"_view);
  ASSERT(
      pixel && point && size && tone && transform && host && image && sprite);

  EXPECT_EQ(pixel->get_layout().get_size(), Count(4));
  EXPECT_EQ(point->get_layout().get_size(), Count(2));
  EXPECT_EQ(size->get_layout().get_size(), Count(2));
  EXPECT_EQ(tone->get_layout().get_size(), Count(4));
  EXPECT_EQ(transform->get_layout().get_size(), Count(4));
  EXPECT(has_callable(*pixel, "from_grey"_view));
  EXPECT(has_callable(*pixel, "from_grey_alpha"_view));
  EXPECT(has_callable(*pixel, "from_rgb"_view));
  EXPECT(has_callable(*pixel, "from_rgba"_view));
  EXPECT(has_callable(*image, "decode"_view));
  EXPECT(has_callable(*image, "get_pixels"_view));
  EXPECT(has_callable(*image, "get_size_pixels"_view));

  Graphics::Hosting hosting;
  EXPECT(hosting.accepts(*host, *sprite));
  EXPECT_TEXT(
      host->get_documentation().get_line(0),
      "Host is a requirement rather than an allocated node Type. Graphics"_view);
  EXPECT_TEXT(
      pixel->get_documentation().get_line(0),
      "Pixel stores one eight bit red, green, blue, and alpha sample. Fully"_view);
}

PERIMORTEM_UNIT_TEST(StandardGraphicsPackage, native_image_product) {
  Process::Request request = {
    .executable = ".bin/bin/validation/graphics_integration"_view,
  };
  Process::Observation observation = Process::run(request);
  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT(observation.standard_output.is_empty());
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());
}
