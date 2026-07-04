// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/resolution/resolver.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Resolution;
using namespace Validation;

static Harness TetrodotoxinLibrary = {
  .name = "Tetrodotoxin::Library"_view,
};

static auto source_type(const Source::Record* record) -> const Ttx::Type* {
  if (record == nullptr) {
    return nullptr;
  }

  return record->get_type();
}

static auto nested_type(const Source::Record* record, View::Bytes name)
    -> const Ttx::Type* {
  const Ttx::Type* type = source_type(record);
  if (type == nullptr) {
    return nullptr;
  }

  return type->find_type(name);
}

static auto member_type(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Type* {
  const Ttx::Type::Member* member = type.find_member(name);
  if (member == nullptr) {
    return nullptr;
  }

  return member->get_type();
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, struct_types) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public g : Real_32;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  ASSERT(source_type(record) != nullptr);
  EXPECT_TEXT(source_type(record)->get_name(), "Library"_view);

  const Ttx::Type* color = nested_type(record, "Color"_view);
  ASSERT(color != nullptr);
  EXPECT_TEXT(color->get_name(), "Color"_view);
  EXPECT_EQ(color->get_members().get_size(), Count(2));
  ASSERT(member_type(*color, "r"_view) != nullptr);
  ASSERT(member_type(*color, "g"_view) != nullptr);
  EXPECT_TEXT(member_type(*color, "r"_view)->get_name(), "Real_32"_view);
  EXPECT_TEXT(member_type(*color, "g"_view)->get_name(), "Real_32"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, root_members) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/screen.ttx"_view,
      "dialect : Library;\n"
      "public Screen : struct {\n"
      "}\n"
      "public screen : Screen;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* library = source_type(record);
  const Ttx::Type* screen = nested_type(record, "Screen"_view);
  ASSERT(library != nullptr);
  ASSERT(screen != nullptr);
  EXPECT_TEXT(library->get_name(), "Library"_view);
  EXPECT_EQ(library->get_members().get_size(), Count(1));
  EXPECT(member_type(*library, "screen"_view) == screen);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, aliases) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"
      "public Tint : alias = Color;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* color = nested_type(record, "Color"_view);
  const Ttx::Type* tint = nested_type(record, "Tint"_view);
  ASSERT(color != nullptr);
  ASSERT(tint != nullptr);
  EXPECT(tint->is_alias());
  EXPECT(tint->canonical() == color);
  ASSERT(member_type(*tint, "r"_view) != nullptr);
  EXPECT_TEXT(member_type(*tint, "r"_view)->get_name(), "Real_32"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, private_alias) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"
      "private LocalColor : alias = Color;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* color = nested_type(record, "Color"_view);
  const Ttx::Type* local_color = nested_type(record, "LocalColor"_view);
  ASSERT(color != nullptr);
  ASSERT(local_color != nullptr);
  EXPECT(local_color->is_alias());
  EXPECT(local_color->canonical() == color);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, type_arguments) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/sprite.ttx"_view,
      "dialect : Library;\n"
      "public Sprite : struct {\n"
      "  public image : View[Bytes];\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* sprite = nested_type(record, "Sprite"_view);
  ASSERT(sprite != nullptr);
  ASSERT(member_type(*sprite, "image"_view) != nullptr);
  EXPECT_TEXT(member_type(*sprite, "image"_view)->get_name(), "View"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, foreign_scope) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Sampler2D : foreign {\n"
      "  external public func sample[] -> [Color];\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* sampler = nested_type(record, "Sampler2D"_view);
  ASSERT(sampler != nullptr);
  EXPECT_TEXT(sampler->get_name(), "Sampler2D"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, package_exports) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* types = resolver.load_source(
      context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"_view);
  ASSERT(types != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  const Source::Record* package = resolver.load_source(
      context, "unit/package.ttx"_view,
      "dialect : Package;\n"
      "import Types : Library = \"types.ttx\";\n"
      "@package_name = Test::Graphics;\n"
      "expose Color : alias = Types::Color;\n"_view);

  ASSERT(package != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* library_color = nested_type(types, "Color"_view);
  const Ttx::Type* package_color = nested_type(package, "Color"_view);
  const Ttx::Type* package_type = source_type(package);
  ASSERT(package_type != nullptr);
  EXPECT_TEXT(package_type->get_name(), "Package"_view);
  const Ttx::Type::Member* package_name =
      package_type->find_member("package_name"_view);
  ASSERT(package_name != nullptr);
  ASSERT(package_name->get_type() != nullptr);
  EXPECT_TEXT(package_name->get_type()->get_name(), "Test::Graphics"_view);
  EXPECT(resolver.resolve("Test::Graphics"_view) == package);

  ASSERT(library_color != nullptr);
  ASSERT(package_color != nullptr);
  EXPECT(package_color->is_alias());
  EXPECT(package_color->canonical() == library_color);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_alias) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad.ttx"_view,
      "dialect : Library;\n"
      "public Broken : alias = Missing;\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(
      context.get_errors()[0].get_message(),
      "Library alias target could not be resolved."_view);
  EXPECT_NOT(resolver.resolve("unit/bad.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_definition) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_modifier.ttx"_view,
      "dialect : Library;\n"
      "state Color : struct {\n"
      "}\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      context.get_errors()[0].get_message(),
      "Expected a definition to start with one of the following modifiers "
      "{public, private, expose}"_view);
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_name.ttx"_view,
      "dialect : Library;\n"
      "public 7 : struct {\n"
      "}\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      context.get_errors()[0].get_message(),
      "Definitions can only be created here for the following types "
      "{type, addressable identifier}"_view);
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_kind.ttx"_view,
      "dialect : Library;\n"
      "public Color : nope;\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      context.get_errors()[0].get_message(),
      "Definition name provided is not one of the known types "
      "{alias, struct, foreign}"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, duplicate_type) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/duplicate.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"
      "public Color : alias = Bits_8;\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(
      context.get_errors()[0].get_message(),
      "Library type name is already defined."_view);
  EXPECT_NOT(resolver.resolve("unit/duplicate.ttx"_view));
}
