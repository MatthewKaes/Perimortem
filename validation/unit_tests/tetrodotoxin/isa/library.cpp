// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/isa/library/block.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Puffer::Resolution;
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

static auto function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Type::Function* {
  return type.find_function(name);
}

static auto error_message(
    const Resolver::Context& context,
    Count index = 0) -> View::Bytes {
  return context.get_errors()[index].get_message();
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
  const Ttx::Type* image = member_type(*sprite, "image"_view);
  ASSERT(image != nullptr);
  EXPECT_TEXT(image->get_name(), "View[Bytes]"_view);
  const Ttx::Attribute* abi = image->find_attribute("abi"_view);
  ASSERT(abi != nullptr);
  EXPECT_TEXT(abi->get_value(), "view_bytes"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, enum_type) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/enums.ttx"_view,
      "dialect : Library;\n"
      "private Color : enum[Bits_8](.red = 1, .green = 2);\n"
      "private Filter : enum[Bits_8] {\n"
      "  none = 0;\n"
      "  sub = 1;\n"
      "  public func default_filter[] -> Filter {\n"
      "    return Filter.none;\n"
      "  }\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* color = nested_type(record, "Color"_view);
  ASSERT(color != nullptr);
  EXPECT_TEXT(member_type(*color, "red"_view)->get_name(), "Bits_8"_view);
  EXPECT_TEXT(member_type(*color, "green"_view)->get_name(), "Bits_8"_view);

  const Ttx::Type* filter = nested_type(record, "Filter"_view);
  ASSERT(filter != nullptr);
  EXPECT_TEXT(member_type(*filter, "none"_view)->get_name(), "Bits_8"_view);
  const Ttx::Type::Function* default_filter =
      function(*filter, "default_filter"_view);
  ASSERT(default_filter != nullptr);
  EXPECT(default_filter->has_body());
  ASSERT_EQ(default_filter->get_result().get_size(), Count(1));
  EXPECT(default_filter->get_result()[0].get_type() == filter);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, foreign_scope) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Sampler2D : foreign {\n"
      "  expose func sample[.texture_uv : Vec2D] -> [Color];\n"
      "}\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* sampler = nested_type(record, "Sampler2D"_view);
  ASSERT(sampler != nullptr);
  EXPECT_TEXT(sampler->get_name(), "Sampler2D"_view);
  const Ttx::Type::Function* sample = function(*sampler, "sample"_view);
  ASSERT(sample != nullptr);
  EXPECT_NOT(sample->has_body());
  EXPECT_EQ(sample->get_parameters().get_size(), Count(1));
  EXPECT_EQ(sample->get_result().get_size(), Count(1));
  EXPECT_TEXT(sample->get_parameters()[0].get_name(), "texture_uv"_view);
  ASSERT(sample->get_result()[0].get_type() != nullptr);
  EXPECT_TEXT(sample->get_result()[0].get_type()->get_name(), "Color"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, root_function) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/main.ttx"_view,
      "dialect : Library;\n"
      "public SceneConfig : struct {\n"
      "  public title : View[Bytes] = \"Test\";\n"
      "}\n"
      "public func main[@builtin(.slot = 0) .scene : SceneConfig] -> [] {\n"
      "  return;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* library = source_type(record);
  ASSERT(library != nullptr);
  const Ttx::Type::Function* main = function(*library, "main"_view);
  ASSERT(main != nullptr);
  EXPECT(main->has_body());
  EXPECT_EQ(main->get_blocks().get_size(), Count(1));
  ASSERT(Tetrodotoxin::Isa::Library::Block::from(main->get_blocks()[0]));
  EXPECT_EQ(
      Tetrodotoxin::Isa::Library::Block::from(main->get_blocks()[0])
          ->get_statements()
          .get_size(),
      Count(1));
  EXPECT_EQ(main->get_parameters().get_size(), Count(1));
  EXPECT_EQ(main->get_result().get_size(), Count(0));
  EXPECT_TEXT(main->get_parameters()[0].get_name(), "scene"_view);
  ASSERT(main->get_parameters()[0].get_type() != nullptr);
  EXPECT_TEXT(
      main->get_parameters()[0].get_type()->get_name(), "SceneConfig"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, body_statements) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/body.ttx"_view,
      "dialect : Library;\n"
      "public Console : foreign {\n"
      "  expose func print[.data : View[Bytes]] -> [];\n"
      "}\n"
      "public func hello[.data : View[Bytes]] -> [] {\n"
      "  Console->print(\"Hi\\n\");\n"
      "  Console->print(data);\n"
      "  return;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* library = source_type(record);
  ASSERT(library != nullptr);
  const Ttx::Type::Function* hello = function(*library, "hello"_view);
  ASSERT(hello != nullptr);
  ASSERT_EQ(hello->get_blocks().get_size(), Count(1));

  const auto* block =
      Tetrodotoxin::Isa::Library::Block::from(hello->get_blocks()[0]);
  ASSERT(block != nullptr);
  ASSERT(hello->get_blocks()[0].get_block() != nullptr);
  EXPECT(
      hello->get_blocks()[0].get_block()->get_representation() ==
      &Tetrodotoxin::Isa::Library::Block::get_representation_type());

  View::Vector<Tetrodotoxin::Isa::Library::Statement> statements =
      block->get_statements();
  ASSERT_EQ(statements.get_size(), Count(3));
  EXPECT(
      statements[0].get_kind() ==
      Tetrodotoxin::Isa::Library::Statement::Kind::Call);
  EXPECT(
      statements[1].get_kind() ==
      Tetrodotoxin::Isa::Library::Statement::Kind::Call);
  EXPECT(
      statements[2].get_kind() ==
      Tetrodotoxin::Isa::Library::Statement::Kind::Return);

  const Tetrodotoxin::Isa::Library::Call& literal_call =
      statements[0].get_call();
  ASSERT(literal_call.get_owner() != nullptr);
  ASSERT(literal_call.get_function() != nullptr);
  EXPECT_TEXT(literal_call.get_owner()->get_name(), "Console"_view);
  EXPECT_TEXT(literal_call.get_name(), "print"_view);
  ASSERT_EQ(literal_call.get_pack().get_values().get_size(), Count(1));
  EXPECT(
      literal_call.get_pack().get_values()[0].get_kind() ==
      Tetrodotoxin::Isa::Expression::Value::Kind::String);
  EXPECT_TEXT(literal_call.get_pack().get_values()[0].get_value(), "Hi\n"_view);

  const Tetrodotoxin::Isa::Library::Call& parameter_call =
      statements[1].get_call();
  ASSERT_EQ(parameter_call.get_pack().get_values().get_size(), Count(1));
  EXPECT(
      parameter_call.get_pack().get_values()[0].get_kind() ==
      Tetrodotoxin::Isa::Expression::Value::Kind::Reference);
  EXPECT_TEXT(
      parameter_call.get_pack().get_values()[0].get_value(), "data"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, struct_method) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  const Source::Record* record = resolver.load_source(
      context, "unit/sprite.ttx"_view,
      "dialect : Library;\n"
      "public Sprite : struct {\n"
      "  public size : Vec2D;\n"
      "  public func area[] -> Real_32 {\n"
      "    return 0.0;\n"
      "  }\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* sprite = nested_type(record, "Sprite"_view);
  ASSERT(sprite != nullptr);
  const Ttx::Type::Function* area = function(*sprite, "area"_view);
  ASSERT(area != nullptr);
  EXPECT(area->has_body());
  EXPECT_EQ(area->get_parameters().get_size(), Count(0));
  EXPECT_EQ(area->get_result().get_size(), Count(1));
  ASSERT(area->get_result()[0].get_type() != nullptr);
  EXPECT_TEXT(area->get_result()[0].get_type()->get_name(), "Real_32"_view);
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
      error_message(context),
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
      error_message(context),
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
      error_message(context),
      "Definitions can only be created here for the following types "
      "{type, addressable identifier}"_view);
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_kind.ttx"_view,
      "dialect : Library;\n"
      "public Color : nope;\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Definition name provided is not one of the known types "
      "{alias, enum, struct, object, foreign}"_view);
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/capital_alias.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "}\n"
      "public Tint : Alias = Color;\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Definition name provided is not one of the known types "
      "{alias, enum, struct, object, foreign}"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_enum) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_enum.ttx"_view,
      "dialect : Library;\n"
      "private Color : enum[Missing](.red = 1);\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Library enum storage type could not be resolved."_view);
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/duplicate_enum.ttx"_view,
      "dialect : Library;\n"
      "private Color : enum[Bits_8](.red = 1, .red = 2);\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Library enum case name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_function) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_function.ttx"_view,
      "dialect : Library;\n"
      "public func main[] [] {\n"
      "}\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Expected `->` before library function result."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_foreign) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_foreign.ttx"_view,
      "dialect : Library;\n"
      "public Sampler2D : foreign {\n"
      "  public func sample[] -> [Bits_8];\n"
      "}\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Expected a foreign function declaration to start with expose."_view);
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/foreign_body.ttx"_view,
      "dialect : Library;\n"
      "public Sampler2D : foreign {\n"
      "  expose func sample[] -> [Bits_8] {\n"
      "  }\n"
      "}\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Expected `;` after library function declaration."_view);
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
      error_message(context),
      "Library type name is already defined."_view);
  EXPECT_NOT(resolver.resolve("unit/duplicate.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, duplicate_member) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  EXPECT_NOT(resolver.load_source(
      context, "unit/duplicate_member.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public r : Real_32;\n"
      "}\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Library member name is already defined."_view);
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/duplicate_root.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "}\n"
      "public color : Color;\n"
      "public color : Color;\n"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      error_message(context),
      "Library member name is already defined."_view);
}
