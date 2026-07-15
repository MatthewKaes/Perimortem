// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Puffer;
using namespace Validation;

static Harness TtxAppScene = {
  .name = "TTX::AppScene"_view,
};

static auto first_error(const Resolution::Resolver::Context& source_context)
    -> View::Bytes {
  return source_context.get_errors()[0].get_message();
}

static auto type_function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Function* {
  return type.find_type_function(name);
}

static auto addressable_function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Function* {
  return type.find_addressable_function(name);
}

static auto register_package(
    Resolution::Resolver& resolver,
    Resolution::Resolver::Context& context,
    View::Bytes path) -> Bool {
  auto package_buffer = File::read(path);
  return !package_buffer.is_empty() &&
         resolver.register_package_buffer(context, path, package_buffer);
}

static auto register_standard_packages(
    Resolution::Resolver& resolver,
    Resolution::Resolver::Context& context) -> Bool {
  return register_package(
             resolver, context,
             ".bin/bin/tetrodotoxin/standard/Perimortem.Math/"
             "binary_archive.puffer"_view) &&
         register_package(
             resolver, context,
             ".bin/bin/tetrodotoxin/standard/Perimortem.Runtime/"
             "binary_archive.puffer"_view) &&
         register_package(
             resolver, context,
             ".bin/bin/tetrodotoxin/standard/Perimortem.Graphics/"
             "binary_archive.puffer"_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, app_main) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context app_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      app_source_context, "unit/app.ttx"_view,
      "dialect : App;\n"
      "public func main[] -> [] {\n"
      "  return;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(app_source_context.has_errors());
  EXPECT_TEXT(record->get_type().get_name(), "App"_view);
  EXPECT_TEXT(record->get_dialect().get_name(), "App"_view);
  const Ttx::Function* main = type_function(record->get_type(), "main"_view);
  ASSERT(main != nullptr);
  EXPECT(record->get_implementation().has(*main));
}

PERIMORTEM_UNIT_TEST(TtxAppScene, scene_shape) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context scene_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      scene_source_context, "unit/scene.ttx"_view,
      "dialect : Scene;\n"
      "state icon : Bits_32 = 0;\n"
      "const fade : Real_64 = 1.0;\n"
      "private func create_icon[] -> Bits_32 {\n"
      "  return 0;\n"
      "}\n"
      "on_start[self] {\n"
      "  return;\n"
      "}\n"
      "on_update[self, .frame : Bits_32] {\n"
      "  return;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(scene_source_context.has_errors());
  const Ttx::Type& scene = record->get_type();
  EXPECT_TEXT(scene.get_name(), "Scene"_view);
  const Ttx::Member* icon_member = scene.find_member("icon"_view);
  const Ttx::Member* fade_member = scene.find_member("fade"_view);
  ASSERT(icon_member != nullptr);
  ASSERT(fade_member != nullptr);
  EXPECT(scene.find_type("state"_view) == nullptr);
  EXPECT(scene.find_type("const"_view) == nullptr);
  const Tetrodotoxin::Isa::Base::Definition* icon =
      record->get_implementation().find(*icon_member);
  const Tetrodotoxin::Isa::Base::Definition* fade =
      record->get_implementation().find(*fade_member);
  ASSERT(icon != nullptr);
  ASSERT(fade != nullptr);
  EXPECT(icon->get_modifier() == Ttx::Lexical::Class::Type::State);
  EXPECT(fade->get_modifier() == Ttx::Lexical::Class::Type::Const);
  EXPECT(icon->has_initializer());
  EXPECT(fade->has_initializer());
  EXPECT(type_function(scene, "create_icon"_view) != nullptr);
  EXPECT(addressable_function(scene, "on_start"_view) != nullptr);
  EXPECT(addressable_function(scene, "on_update"_view) != nullptr);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, bad_app_root) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_app_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_app_source_context, "unit/bad_app.ttx"_view,
      "dialect : App;\n"
      "public func start[] -> [] {\n"
      "  return;\n"
      "}\n"_view));

  ASSERT(bad_app_source_context.has_errors());
  EXPECT_TEXT(
      first_error(bad_app_source_context),
      "App root can only define `main`."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, bad_scene_root) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_scene_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_scene_source_context, "unit/bad_scene.ttx"_view,
      "dialect : Scene;\n"
      "draw {\n"
      "  return;\n"
      "}\n"_view));

  ASSERT(bad_scene_source_context.has_errors());
  EXPECT_TEXT(
      first_error(bad_scene_source_context),
      "Scene root addressable must be on_start, on_update, or on_exit."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, scene_dupe) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context duplicate_scene_source_context;

  EXPECT_NOT(resolver.load_source(
      duplicate_scene_source_context, "unit/dupe_scene.ttx"_view,
      "dialect : Scene;\n"
      "state value : Bits_32;\n"
      "const value : Bits_32;\n"_view));

  ASSERT(duplicate_scene_source_context.has_errors());
  EXPECT_TEXT(
      first_error(duplicate_scene_source_context),
      "Scene member name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, app_sources) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(register_standard_packages(resolver, dependency_context));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context app_source_context;

  const Resolution::Source::Record* app =
      resolver.load_source(app_source_context, "apps/ttx/demo/main.ttx"_view);
  ASSERT(app != nullptr);
  EXPECT_NOT(app_source_context.has_errors());
  EXPECT_TEXT(app->get_module(), "main"_view);
  EXPECT_TEXT(app->get_type().get_name(), "App"_view);
  ASSERT(type_function(app->get_type(), "main"_view) != nullptr);

  const Resolution::Source::Record* scene =
      resolver.resolve("apps/ttx/demo/splash_screen.ttx"_view);
  ASSERT(scene != nullptr);
  EXPECT_TEXT(scene->get_module(), "splash_screen"_view);
  EXPECT_TEXT(scene->get_type().get_name(), "Scene"_view);
  EXPECT(addressable_function(scene->get_type(), "on_update"_view) != nullptr);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, app_needs_main) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context app_source_context;

  EXPECT_NOT(resolver.load_source(
      app_source_context, "unit/empty_app.ttx"_view, "dialect : App;\n"_view));

  ASSERT(app_source_context.has_errors());
  EXPECT_TEXT(
      first_error(app_source_context),
      "App source must define public `main[] -> []`."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, app_bad_shape) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context app_source_context;

  EXPECT_NOT(resolver.load_source(
      app_source_context, "unit/bad_app.ttx"_view,
      "dialect : App;\n"
      "public func main[.argc : Bits_32] -> [] {\n"
      "  return;\n"
      "}\n"_view));

  ASSERT(app_source_context.has_errors());
  EXPECT_TEXT(
      first_error(app_source_context),
      "App `main` must use `main[] -> []`."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, app_dupe_main) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context app_source_context;

  EXPECT_NOT(resolver.load_source(
      app_source_context, "unit/dupe_app.ttx"_view,
      "dialect : App;\n"
      "public func main[] -> [] { return; }\n"
      "public func main[] -> [] { return; }\n"_view));

  ASSERT(app_source_context.has_errors());
  EXPECT_TEXT(
      first_error(app_source_context),
      "App function name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, scene_lifecycle) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context scene_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      scene_source_context, "unit/scene.ttx"_view,
      "dialect : Scene;\n"
      "on_start[self] { return; }\n"
      "on_exit[self] { return; }\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(scene_source_context.has_errors());
  EXPECT(addressable_function(record->get_type(), "on_start"_view) != nullptr);
  EXPECT(addressable_function(record->get_type(), "on_exit"_view) != nullptr);
}

PERIMORTEM_UNIT_TEST(TtxAppScene, scene_bad_type) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context scene_source_context;

  EXPECT_NOT(resolver.load_source(
      scene_source_context, "unit/bad_scene.ttx"_view,
      "dialect : Scene;\n"
      "state value : MissingType;\n"_view));

  ASSERT(scene_source_context.has_errors());
  EXPECT_TEXT(
      first_error(scene_source_context),
      "Scene member type could not be resolved."_view);
}
