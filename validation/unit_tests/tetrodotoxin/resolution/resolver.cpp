// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/resolution/resolver.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxResolution = {
  .name = "Tetrodotoxin::Resolution"_view,
};

PERIMORTEM_UNIT_TEST(TtxResolution, package_no_imports) {
  Resolution::Resolver resolver;

  EXPECT(
      resolver.resolve("validation/data/ttx/resolution/no_imports.ttx"_view));
  EXPECT_EQ(resolver.get_error_count(), 0);
  ASSERT_EQ(resolver.get_packages().get_size(), 1);

  Resolution::Record* record =
      resolver.find("validation/data/ttx/resolution/no_imports.ttx"_view);
  ASSERT(record);
  EXPECT(record->is_resolved());
  EXPECT_EQ(record->get_parse_count(), 1);
  EXPECT_EQ(record->get_package().get_imports().get_size(), 0);
}

PERIMORTEM_UNIT_TEST(TtxResolution, multiple_imports) {
  Resolution::Resolver resolver;

  EXPECT(resolver.resolve("validation/data/ttx/resolution/root.ttx"_view));
  EXPECT_EQ(resolver.get_error_count(), 0);
  ASSERT_EQ(resolver.get_packages().get_size(), 3);

  Resolution::Record* root =
      resolver.find("validation/data/ttx/resolution/root.ttx"_view);
  Resolution::Record* math =
      resolver.find("validation/data/ttx/resolution/math.ttx"_view);
  Resolution::Record* image =
      resolver.find("validation/data/ttx/resolution/image.ttx"_view);

  ASSERT(root);
  ASSERT(math);
  ASSERT(image);
  EXPECT(root->is_resolved());
  EXPECT(math->is_resolved());
  EXPECT(image->is_resolved());
  ASSERT_EQ(root->get_package().get_imports().get_size(), 2);
}

PERIMORTEM_UNIT_TEST(TtxResolution, cached_package_asts) {
  Resolution::Resolver resolver;

  EXPECT(resolver.resolve("validation/data/ttx/resolution/root.ttx"_view));

  Resolution::Record* root =
      resolver.find("validation/data/ttx/resolution/root.ttx"_view);
  Resolution::Record* math =
      resolver.find("validation/data/ttx/resolution/math.ttx"_view);
  Resolution::Record* image =
      resolver.find("validation/data/ttx/resolution/image.ttx"_view);

  ASSERT(root);
  ASSERT(math);
  ASSERT(image);
  EXPECT_EQ(root->get_parse_count(), 1);
  EXPECT_EQ(math->get_parse_count(), 1);
  EXPECT_EQ(image->get_parse_count(), 1);

  EXPECT(resolver.resolve("validation/data/ttx/resolution/root.ttx"_view));
  EXPECT_EQ(root->get_parse_count(), 1);
  EXPECT_EQ(math->get_parse_count(), 1);
  EXPECT_EQ(image->get_parse_count(), 1);
}

PERIMORTEM_UNIT_TEST(TtxResolution, import_cycles) {
  Resolution::Resolver resolver;

  EXPECT_NOT(
      resolver.resolve("validation/data/ttx/resolution/cycle_a.ttx"_view));
  EXPECT(resolver.get_error_count() != 0);
  ASSERT_EQ(resolver.get_packages().get_size(), 2);

  Resolution::Record* cycle_a =
      resolver.find("validation/data/ttx/resolution/cycle_a.ttx"_view);
  Resolution::Record* cycle_b =
      resolver.find("validation/data/ttx/resolution/cycle_b.ttx"_view);

  ASSERT(cycle_a);
  ASSERT(cycle_b);
  EXPECT(
      cycle_a->get_state() == Resolution::Resolver::State::Failed ||
      cycle_b->get_state() == Resolution::Resolver::State::Failed);
}

PERIMORTEM_UNIT_TEST(TtxResolution, import_kind_mismatch) {
  Resolution::Resolver resolver;

  EXPECT_NOT(
      resolver.resolve("validation/data/ttx/resolution/wrong_kind.ttx"_view));
  EXPECT(resolver.get_error_count() != 0);
  ASSERT_EQ(resolver.get_packages().get_size(), 2);

  Resolution::Record* root =
      resolver.find("validation/data/ttx/resolution/wrong_kind.ttx"_view);
  Resolution::Record* image =
      resolver.find("validation/data/ttx/resolution/image.ttx"_view);

  ASSERT(root);
  ASSERT(image);
  EXPECT(root->get_state() == Resolution::Resolver::State::Failed);
  EXPECT(image->get_package().get_kind() == Syntax::PackageKind::Render);
}

PERIMORTEM_UNIT_TEST(TtxResolution, compiler_ttx_imports) {
  Resolution::Resolver resolver;

  EXPECT(resolver.resolve("validation/data/ttx/syntax_sample.ttx"_view));
  EXPECT_EQ(resolver.get_error_count(), 0);

  Resolution::Record* root =
      resolver.find("validation/data/ttx/syntax_sample.ttx"_view);
  Resolution::Record* graphics =
      resolver.find("validation/data/ttx/graphics/image.ttx"_view);

  ASSERT(root);
  ASSERT(graphics);
  EXPECT(root->is_resolved());
  EXPECT(graphics->is_resolved());
  EXPECT(resolver.find_standard_package(*root, "Core"_view).exists());
}

PERIMORTEM_UNIT_TEST(TtxResolution, unhandled_registry) {
  Resolution::Resolver resolver;

  EXPECT_NOT(resolver.resolve(
      "validation/data/ttx/resolution/registry_import.ttx"_view));
  EXPECT(resolver.get_error_count() != 0);

  Resolution::Record* root =
      resolver.find("validation/data/ttx/resolution/registry_import.ttx"_view);
  ASSERT(root);
  EXPECT(root->get_state() == Resolution::Resolver::State::Failed);
}

PERIMORTEM_UNIT_TEST(TtxResolution, unknown_ttx_imports) {
  Resolution::Resolver resolver;

  EXPECT_NOT(resolver.resolve(
      "validation/data/ttx/resolution/unknown_ttx_import.ttx"_view));
  EXPECT(resolver.get_error_count() != 0);

  Resolution::Record* root = resolver.find(
      "validation/data/ttx/resolution/unknown_ttx_import.ttx"_view);
  ASSERT(root);
  EXPECT(root->get_state() == Resolution::Resolver::State::Failed);
}

PERIMORTEM_UNIT_TEST(TtxResolution, resolves_png_fixture) {
  Resolution::Resolver resolver;

  EXPECT(resolver.resolve("validation/data/ttx/png.ttx"_view));
  EXPECT_EQ(resolver.get_error_count(), 0);

  Resolution::Record* png = resolver.find("validation/data/ttx/png.ttx"_view);
  Resolution::Record* graphics =
      resolver.find("validation/data/ttx/graphics/image.ttx"_view);

  ASSERT(png);
  ASSERT(graphics);
  EXPECT(png->is_resolved());
  EXPECT(graphics->is_resolved());
}

PERIMORTEM_UNIT_TEST(TtxResolution, default_2d_shader_fixture) {
  Resolution::Resolver resolver;

  EXPECT(resolver.resolve("apps/shaders/default_2d.ttx"_view));
  EXPECT_EQ(resolver.get_error_count(), 0);

  Resolution::Record* default_2d =
      resolver.find("apps/shaders/default_2d.ttx"_view);
  ASSERT(default_2d);
  EXPECT(default_2d->is_resolved());
}
