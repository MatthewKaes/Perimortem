// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/interfaces/structure.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/shader/dialect.hpp"

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
  auto math_product =
      File::read(".bin/bin/packages/ttx/Perimortem.Math/1.0/contract.txa"_view);
  auto product = File::read(
      ".bin/bin/packages/ttx/Perimortem.Graphics/1.0/contract.txa"_view);
  ASSERT(math_product && product);
  Allocator::Arena math_archive_arena;
  auto decoded_math =
      Package::Archive::Reader::read(math_archive_arena, *math_product);
  Option<Package::Archive::Archive> math_archive;
  decoded_math.visit(
      [&](const Package::Archive::Archive& selected) {
        math_archive = selected;
      },
      [](const Package::Archive::Reader::Error&) {});
  ASSERT(math_archive);
  Allocator::Arena archive_arena;
  auto decoded = Package::Archive::Reader::read(archive_arena, *product);
  Option<Package::Archive::Archive> archive;
  decoded.visit(
      [&](const Package::Archive::Archive& selected) { archive = selected; },
      [](const Package::Archive::Reader::Error&) {});
  ASSERT(archive);

  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Package::Dialect>("Package"_view));
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto render = toolchain.install<Render::Dialect>("Render"_view);
  ASSERT(library && render);
  ASSERT(toolchain.install<Shader::Dialect>("Shader"_view, *library, *render));
  Environment::Workspace workspace(toolchain);
  ASSERT(workspace.restore_package(*math_archive, "Math"_view));
  auto restored = workspace.restore_package(*archive, "Graphics"_view);
  ASSERT(restored && restored->is<Package::Language::Monograph>());
  const auto& package =
      static_cast<const Package::Language::Monograph&>(*restored);
  auto pixel = select_type(package, "Pixel"_view);
  auto point = select_type(package, "Point2D"_view);
  auto size = select_type(package, "Size2D"_view);
  auto tone = select_type(package, "Tone"_view);
  auto transform = select_type(package, "Transform2D"_view);
  auto placement = select_type(package, "Placement2D"_view);
  auto image = select_type(package, "Image"_view);
  auto texture = select_type(package, "Texture2D"_view);
  auto sprite = select_type(package, "Sprite"_view);
  const auto& format = package.resolve_context("Format"_view).resolve();
  auto png_member = format.resolve_context("PNG"_view)
                        .resolve()
                        .select<Library::Language::Monograph>();
  auto png = png_member ? png_member->resolve_context("PNG"_view)
                              .resolve()
                              .select<Library::Language::Types::Composite>()
                        : Option<const Library::Language::Types::Composite&>();
  ASSERT(
      pixel && point && size && tone && transform && placement && image &&
      texture && sprite && png);

  EXPECT_EQ(pixel->get_layout().get_size(), Count(4));
  EXPECT_EQ(point->get_layout().get_size(), Count(2));
  EXPECT_EQ(size->get_layout().get_size(), Count(2));
  EXPECT_EQ(tone->get_layout().get_size(), Count(4));
  EXPECT_EQ(transform->get_layout().get_size(), Count(4));
  EXPECT(has_callable(*pixel, "from_grey"_view));
  EXPECT(has_callable(*pixel, "from_grey_alpha"_view));
  EXPECT(has_callable(*pixel, "from_rgb"_view));
  EXPECT(has_callable(*pixel, "from_rgba"_view));
  EXPECT(has_callable(*png, "decode"_view));
  EXPECT(has_callable(*image, "get_pixels"_view));
  EXPECT(has_callable(*image, "get_addressing"_view));
  EXPECT(has_callable(*image, "get_size_pixels"_view));
  EXPECT(has_callable(*image, "sample"_view));
  EXPECT(has_callable(*image, "with_addressing"_view));
  EXPECT(has_callable(*texture, "from_image"_view));
  EXPECT(has_callable(*texture, "get_image"_view));

  Library::Language::Interfaces::Structure hosting;
  EXPECT(hosting.accepts(*placement, *sprite));
  EXPECT_TEXT(
      placement->get_documentation().get_line(0),
      "Placement2D lets Graphics read the transform and frame ordering of a real"_view);
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
