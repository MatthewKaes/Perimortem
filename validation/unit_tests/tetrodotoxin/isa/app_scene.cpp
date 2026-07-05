// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Puffer::Resolution;
using namespace Validation;

static Harness TtxAppScene = {
  .name = "TTX::AppScene"_view,
};

static auto first_error(const Resolver::Context& context) -> View::Bytes {
  return context.get_errors()[0].get_message();
}

static auto function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Type::Function* {
  return type.find_function(name);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, app_main) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/app.ttx"_view,
      "dialect : App;\n"
      "public func main[] -> [] {\n"
      "  return;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());
  ASSERT(record->get_type() != nullptr);
  EXPECT_TEXT(record->get_type()->get_name(), "App"_view);
  const Ttx::Attribute* isa = record->get_type()->find_attribute("isa"_view);
  ASSERT(isa != nullptr);
  EXPECT_TEXT(isa->get_value(), "App"_view);
  ASSERT(function(*record->get_type(), "main"_view) != nullptr);
  EXPECT(function(*record->get_type(), "main"_view)->has_body());
}

PERIMORTEM_UNIT_TEST(TtxAppScene, scene_shape) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/scene.ttx"_view,
      "dialect : Scene;\n"
      "state icon : Bits_32 = 0;\n"
      "const fade : Real_64 = 1.0;\n"
      "private func create_icon[] -> Bits_32 {\n"
      "  return 0;\n"
      "}\n"
      "on_start {\n"
      "  return;\n"
      "}\n"
      "on_update[.frame : Bits_32] {\n"
      "  return;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());
  const Ttx::Type* scene = record->get_type();
  ASSERT(scene != nullptr);
  EXPECT_TEXT(scene->get_name(), "Scene"_view);
  EXPECT(scene->find_member("icon"_view) != nullptr);
  EXPECT(scene->find_member("fade"_view) != nullptr);
  ASSERT(scene->find_type("state"_view) != nullptr);
  ASSERT(scene->find_type("const"_view) != nullptr);
  EXPECT(scene->find_type("state"_view)->find_member("icon"_view) != nullptr);
  EXPECT(scene->find_type("const"_view)->find_member("fade"_view) != nullptr);
  EXPECT(function(*scene, "create_icon"_view) != nullptr);
  EXPECT(function(*scene, "on_start"_view) != nullptr);
  EXPECT(function(*scene, "on_update"_view) != nullptr);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, bad_app_root) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_app.ttx"_view,
      "dialect : App;\n"
      "public func start[] -> [] {\n"
      "  return;\n"
      "}\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(first_error(context), "App root can only define `main`."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, bad_scene_root) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_scene.ttx"_view,
      "dialect : Scene;\n"
      "draw {\n"
      "  return;\n"
      "}\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(
      first_error(context),
      "Scene root addressable must be on_start, on_update, or on_exit."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, scene_dupe) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/dupe_scene.ttx"_view,
      "dialect : Scene;\n"
      "state value : Bits_32;\n"
      "const value : Bits_32;\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(first_error(context), "Scene member name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, app_sources) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* app = resolver.load_source(context, "apps/main.ttx"_view);
  ASSERT(app != nullptr);
  EXPECT_NOT(context.has_errors());
  ASSERT(app->get_type() != nullptr);
  EXPECT_TEXT(app->get_type()->get_name(), "App"_view);
  ASSERT(function(*app->get_type(), "main"_view) != nullptr);

  const Source::Record* scene = resolver.resolve("apps/splash_screen.ttx"_view);
  ASSERT(scene != nullptr);
  ASSERT(scene->get_type() != nullptr);
  EXPECT_TEXT(scene->get_type()->get_name(), "Scene"_view);
  EXPECT(function(*scene->get_type(), "on_update"_view) != nullptr);
}
