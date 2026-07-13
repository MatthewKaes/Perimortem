// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/toolchain.hpp"
#include "tetrodotoxin/standard/types.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Puffer;
using namespace Validation;

static Harness TetrodotoxinLibrary = {
  .name = "Tetrodotoxin::Library"_view,
};

static auto source_type(const Resolution::Source::Record* record)
    -> const Ttx::Type* {
  if (record == nullptr) {
    return nullptr;
  }

  return &record->get_type();
}

static auto nested_type(
    const Resolution::Source::Record* record,
    View::Bytes name) -> const Ttx::Type* {
  const Ttx::Type* type = source_type(record);
  if (type == nullptr) {
    return nullptr;
  }

  return type->find_type(name);
}

static auto member_type(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Type* {
  const Ttx::Member* member = type.find_member(name);
  if (member == nullptr) {
    return nullptr;
  }

  return &member->get_type();
}

static auto function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Function* {
  return type.find_function(name);
}

static auto error_message(
    const Resolution::Resolver::Context& source_context,
    Count index = 0) -> View::Bytes {
  return source_context.get_errors()[index].get_message();
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, struct_types) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context types_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      types_source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public g : Real_32;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(types_source_context.has_errors());

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
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context screen_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      screen_source_context, "unit/screen.ttx"_view,
      "dialect : Library;\n"
      "public Screen : struct {\n"
      "}\n"
      "public screen : Screen;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(screen_source_context.has_errors());

  const Ttx::Type* library = source_type(record);
  const Ttx::Type* screen = nested_type(record, "Screen"_view);
  ASSERT(library != nullptr);
  ASSERT(screen != nullptr);
  EXPECT_TEXT(library->get_name(), "Library"_view);
  EXPECT_EQ(library->get_members().get_size(), Count(1));
  EXPECT(member_type(*library, "screen"_view) == screen);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, aliases) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context types_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      types_source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"
      "public Tint : alias = Color;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(types_source_context.has_errors());

  const Ttx::Type* color = nested_type(record, "Color"_view);
  const Ttx::Type* tint = nested_type(record, "Tint"_view);
  ASSERT(color != nullptr);
  ASSERT(tint != nullptr);
  EXPECT(tint->is_alias());
  EXPECT(&tint->canonical() == color);
  ASSERT(member_type(*tint, "r"_view) != nullptr);
  EXPECT_TEXT(member_type(*tint, "r"_view)->get_name(), "Real_32"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, private_alias) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context types_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      types_source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"
      "private LocalColor : alias = Color;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(types_source_context.has_errors());

  const Ttx::Type* color = nested_type(record, "Color"_view);
  const Ttx::Type* local_color = nested_type(record, "LocalColor"_view);
  ASSERT(color != nullptr);
  ASSERT(local_color != nullptr);
  EXPECT(local_color->is_alias());
  EXPECT(&local_color->canonical() == color);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, type_arguments) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context sprite_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      sprite_source_context, "unit/sprite.ttx"_view,
      "dialect : Library;\n"
      "public Sprite : struct {\n"
      "  public image : View[Bytes];\n"
      "  public values : Vec[Bits_8, 4];\n"
      "  public copy : Vec[Bits_8, 4];\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(sprite_source_context.has_errors());

  const Ttx::Type* sprite = nested_type(record, "Sprite"_view);
  ASSERT(sprite != nullptr);
  const Ttx::Type* image = member_type(*sprite, "image"_view);
  ASSERT(image != nullptr);
  EXPECT_TEXT(image->get_name(), "View[Bytes]"_view);
  const Ttx::Attribute* abi = image->find_attribute("abi"_view);
  ASSERT(abi != nullptr);
  EXPECT_TEXT(abi->get_value(), "view_bytes"_view);

  const Ttx::Type* values = member_type(*sprite, "values"_view);
  const Ttx::Type* copy = member_type(*sprite, "copy"_view);
  ASSERT(values != nullptr);
  ASSERT(copy != nullptr);
  EXPECT_TEXT(values->get_name(), "Vec[Bits_8,4]"_view);
  EXPECT(values == copy);
  EXPECT(values != Tetrodotoxin::Standard::Types::find_type("Vec"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_type_args) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  EXPECT_NOT(resolver.load_source(
      source_context, "unit/bad_type_arguments.ttx"_view,
      "dialect : Library;\n"
      "public Value : struct {\n"
      "  public data : Bits_32[Bytes];\n"
      "}\n"_view));
  ASSERT(source_context.has_errors());
  EXPECT_TEXT(
      error_message(source_context),
      "Type arguments are not supported for this type."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, enum_type) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context enum_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      enum_source_context, "unit/enums.ttx"_view,
      "dialect : Library;\n"
      "private Color : enum[Bits_8] {\n"
      "  red = 1;\n"
      "  green = 2;\n"
      "}\n"
      "private Filter : enum[Bits_8] {\n"
      "  none = 0;\n"
      "  sub = 1;\n"
      "  public func default_filter[] -> Filter {\n"
      "    return Filter.none;\n"
      "  }\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(enum_source_context.has_errors());

  const Ttx::Type* color = nested_type(record, "Color"_view);
  ASSERT(color != nullptr);
  const Ttx::Member* red = color->find_member("red"_view);
  ASSERT(red != nullptr);
  EXPECT_TEXT(red->get_type().get_name(), "Bits_8"_view);
  EXPECT_TEXT(member_type(*color, "green"_view)->get_name(), "Bits_8"_view);
  const auto* red_facts = record->get_implementation().find(*red);
  ASSERT(red_facts != nullptr);
  EXPECT(red_facts->get_modifier() == Ttx::Lexical::Class::Type::Expose);
  ASSERT(red_facts->get_initializer() != nullptr);
  EXPECT_TEXT(red_facts->get_initializer()->get_value(), "1"_view);

  const Ttx::Type* filter = nested_type(record, "Filter"_view);
  ASSERT(filter != nullptr);
  EXPECT_TEXT(member_type(*filter, "none"_view)->get_name(), "Bits_8"_view);
  const Ttx::Function* default_filter =
      function(*filter, "default_filter"_view);
  ASSERT(default_filter != nullptr);
  EXPECT(record->get_implementation().has(*default_filter));
  ASSERT_EQ(default_filter->get_result().get_member_count(), Count(1));
  EXPECT(&default_filter->get_result().member_at(0).get_type() == filter);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, foreign_scope) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context foreign_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      foreign_source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Sampler2D : foreign {\n"
      "  expose func sample[.texture_uv : Vec2D] -> [Color];\n"
      "}\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(foreign_source_context.has_errors());

  const Ttx::Type* sampler = nested_type(record, "Sampler2D"_view);
  ASSERT(sampler != nullptr);
  EXPECT_TEXT(sampler->get_name(), "Sampler2D"_view);
  EXPECT(sampler->find_attribute("isa"_view) == nullptr);
  const Ttx::Function* sample = function(*sampler, "sample"_view);
  ASSERT(sample != nullptr);
  const auto* linkage = record->get_implementation().find_linkage(*sample);
  ASSERT(linkage != nullptr);
  EXPECT_TEXT(linkage->get_symbol(), "sample"_view);
  EXPECT_NOT(record->get_implementation().has(*sample));
  EXPECT_EQ(sample->get_parameters().get_member_count(), Count(1));
  EXPECT_EQ(sample->get_result().get_member_count(), Count(1));
  EXPECT_TEXT(
      sample->get_parameters().member_at(0).get_name(), "texture_uv"_view);
  EXPECT_TEXT(
      sample->get_result().member_at(0).get_type().get_name(), "Color"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, foreign_import) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context producer_context;

  const Resolution::Source::Record* producer = resolver.load_source(
      producer_context, "unit/foreign.ttx"_view,
      "dialect : Library;\n"
      "public Console : foreign {\n"
      "  expose func print[.data : View[Bytes]] -> [];\n"
      "}\n"_view);
  ASSERT(producer != nullptr);
  EXPECT_NOT(producer_context.has_errors());

  Resolution::Resolver::Context consumer_context;
  const Resolution::Source::Record* consumer = resolver.load_source(
      consumer_context, "unit/main.ttx"_view,
      "dialect : Library;\n"
      "import Api : Library = \"foreign.ttx\";\n"
      "public func main[] -> [] {\n"
      "  Api::Console->print(\"Hello\");\n"
      "}\n"_view);
  ASSERT(consumer != nullptr);
  EXPECT_NOT(consumer_context.has_errors());

  const Ttx::Function* main = function(consumer->get_type(), "main"_view);
  ASSERT(main != nullptr);
  const auto* body = consumer->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*main);
  ASSERT(body != nullptr);
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Call>(), Count(1));
  EXPECT_TEXT(
      body->find<Tetrodotoxin::Compiler::Execution::Call>()->get_symbol(),
      "print"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, ttx_call) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context producer_context;

  const Resolution::Source::Record* producer = resolver.load_source(
      producer_context, "unit/api.ttx"_view,
      "dialect : Library;\n"
      "public func echo[.data : View[Bytes]] -> [] {\n"
      "  return;\n"
      "}\n"_view);
  ASSERT(producer != nullptr);
  EXPECT_NOT(producer_context.has_errors());

  Resolution::Resolver::Context consumer_context;
  const Resolution::Source::Record* consumer = resolver.load_source(
      consumer_context, "unit/main.ttx"_view,
      "dialect : Library;\n"
      "import Api : Library = \"api.ttx\";\n"
      "public func main[] -> [] {\n"
      "  Api->echo(\"Hello\");\n"
      "}\n"_view);
  ASSERT(consumer != nullptr);
  EXPECT_NOT(consumer_context.has_errors());

  const Ttx::Function* main = function(consumer->get_type(), "main"_view);
  ASSERT(main != nullptr);
  const auto* body = consumer->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*main);
  ASSERT(body != nullptr);
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Call>(), Count(1));
  EXPECT_TEXT(
      body->find<Tetrodotoxin::Compiler::Execution::Call>()->get_symbol(),
      "TTX_api_echo"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, root_function) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context main_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      main_source_context, "unit/main.ttx"_view,
      "dialect : Library;\n"
      "public SceneConfig : struct {\n"
      "  public title : View[Bytes] = \"Test\";\n"
      "}\n"
      "public func main[@builtin(.slot = 0) .scene : SceneConfig] -> [] {\n"
      "  return;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(main_source_context.has_errors());

  const Ttx::Type* library = source_type(record);
  ASSERT(library != nullptr);
  const Ttx::Function* main = function(*library, "main"_view);
  ASSERT(main != nullptr);
  const auto* main_body =
      record->get_implementation()
          .find<Tetrodotoxin::Compiler::Execution::Body>(*main);
  ASSERT(main_body != nullptr);
  EXPECT_EQ(main_body->get_operations().get_size(), Count(1));
  EXPECT_EQ(main->get_parameters().get_member_count(), Count(1));
  EXPECT_EQ(main->get_result().get_member_count(), Count(0));
  EXPECT_TEXT(main->get_parameters().member_at(0).get_name(), "scene"_view);
  EXPECT_TEXT(
      main->get_parameters().member_at(0).get_type().get_name(),
      "SceneConfig"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, body_statements) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context body_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      body_source_context, "unit/body.ttx"_view,
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
  EXPECT_NOT(body_source_context.has_errors());

  const Ttx::Type* library = source_type(record);
  ASSERT(library != nullptr);
  const Ttx::Function* hello = function(*library, "hello"_view);
  ASSERT(hello != nullptr);
  const auto* body = record->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*hello);
  ASSERT(body != nullptr);

  ASSERT_EQ(body->get_operations().get_size(), Count(3));
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Call>(), Count(2));
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Return>(), Count(1));

  const Tetrodotoxin::Compiler::Execution::Call& literal_call =
      *body->find<Tetrodotoxin::Compiler::Execution::Call>();
  EXPECT_TEXT(literal_call.get_symbol(), "print"_view);
  auto literal_arguments = literal_call.get_arguments();
  ASSERT_EQ(literal_arguments.size, Count(1));
  const auto* literal =
      body->get_operands()[literal_arguments.start]
          .find<Tetrodotoxin::Compiler::Execution::Constant>();
  ASSERT(literal != nullptr);
  const View::Bytes* literal_bytes = literal->find<View::Bytes>();
  ASSERT(literal_bytes != nullptr);
  EXPECT_TEXT(*literal_bytes, "Hi\n"_view);

  const Tetrodotoxin::Compiler::Execution::Call& parameter_call =
      *body->find<Tetrodotoxin::Compiler::Execution::Call>(1);
  auto parameter_arguments = parameter_call.get_arguments();
  ASSERT_EQ(parameter_arguments.size, Count(1));
  const auto* parameter =
      body->get_operands()[parameter_arguments.start]
          .find<Tetrodotoxin::Compiler::Execution::Addressable>();
  ASSERT(parameter != nullptr);
  EXPECT_EQ(parameter->get_id(), Count(0));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, struct_method) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context sprite_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      sprite_source_context, "unit/sprite.ttx"_view,
      "dialect : Library;\n"
      "public Sprite : struct {\n"
      "  public size : Vec2D;\n"
      "  public func area[] -> Real_32 {\n"
      "    return 0.0;\n"
      "  }\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(sprite_source_context.has_errors());

  const Ttx::Type* sprite = nested_type(record, "Sprite"_view);
  ASSERT(sprite != nullptr);
  const Ttx::Function* area = function(*sprite, "area"_view);
  ASSERT(area != nullptr);
  EXPECT(record->get_implementation().has(*area));
  EXPECT_EQ(area->get_parameters().get_member_count(), Count(0));
  EXPECT_EQ(area->get_result().get_member_count(), Count(1));
  EXPECT_TEXT(
      area->get_result().member_at(0).get_type().get_name(), "Real_32"_view);
  const auto* body = record->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*area);
  ASSERT(body != nullptr);
  const auto* value = body->get_operands()[0]
                          .find<Tetrodotoxin::Compiler::Execution::Constant>();
  ASSERT(value != nullptr);
  ASSERT(value->find<Real_64>() != nullptr);
  EXPECT_EQ(*value->find<Real_64>(), Real_64(0));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, scalar_values) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      source_context, "unit/bool.ttx"_view,
      "dialect : Library;\n"
      "public func truth[] -> Bool {\n"
      "  return true;\n"
      "}\n"
      "public func number[] -> Count {\n"
      "  return 42;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(source_context.has_errors());
  const Ttx::Function* truth = function(record->get_type(), "truth"_view);
  ASSERT(truth != nullptr);
  const auto* body = record->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*truth);
  ASSERT(body != nullptr);
  const auto* value = body->get_operands()[0]
                          .find<Tetrodotoxin::Compiler::Execution::Constant>();
  ASSERT(value != nullptr);
  ASSERT(value->find<Bool>() != nullptr);
  EXPECT(*value->find<Bool>());

  const Ttx::Function* number = function(record->get_type(), "number"_view);
  ASSERT(number != nullptr);
  body = record->get_implementation()
             .find<Tetrodotoxin::Compiler::Execution::Body>(*number);
  ASSERT(body != nullptr);
  value = body->get_operands()[0]
              .find<Tetrodotoxin::Compiler::Execution::Constant>();
  ASSERT(value != nullptr);
  ASSERT(value->find<Bits_64>() != nullptr);
  EXPECT_EQ(*value->find<Bits_64>(), Bits_64(42));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, constant_folding) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      source_context, "unit/fold.ttx"_view,
      "dialect : Library;\n"
      "public func value[] -> Count {\n"
      "  return 2 + 3;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(source_context.has_errors());
  const Ttx::Function* value = function(record->get_type(), "value"_view);
  ASSERT(value != nullptr);
  const auto* body = record->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*value);
  ASSERT(body != nullptr);
  EXPECT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Binary>(), Count(0));
  ASSERT_EQ(body->get_operands().get_size(), Count(1));
  const auto* constant =
      body->get_operands()[0]
          .find<Tetrodotoxin::Compiler::Execution::Constant>();
  ASSERT(constant != nullptr);
  ASSERT(constant->find<Bits_64>() != nullptr);
  EXPECT_EQ(*constant->find<Bits_64>(), Bits_64(5));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, named_return_mapping) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      source_context, "unit/swap.ttx"_view,
      "dialect : Library;\n"
      "public func swap[.first : Count, .second : Count] -> [\n"
      "  .first : Count, .second : Count\n"
      "] {\n"
      "  return (.second = first, .first = second);\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(source_context.has_errors());
  const Ttx::Function* swap = function(record->get_type(), "swap"_view);
  ASSERT(swap != nullptr);
  const auto* body = record->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*swap);
  ASSERT(body != nullptr);
  const auto* return_op =
      body->find<Tetrodotoxin::Compiler::Execution::Return>();
  ASSERT(return_op != nullptr);
  Perimortem::Utility::Range values = return_op->get_values();
  ASSERT_EQ(values.size, Count(2));
  const auto* first =
      body->get_operands()[values.start]
          .find<Tetrodotoxin::Compiler::Execution::Addressable>();
  const auto* second =
      body->get_operands()[values.start + 1]
          .find<Tetrodotoxin::Compiler::Execution::Addressable>();
  ASSERT(first != nullptr);
  ASSERT(second != nullptr);
  EXPECT_EQ(first->get_id(), Count(1));
  EXPECT_EQ(second->get_id(), Count(0));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, unknown_return_parameter) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  EXPECT_NOT(resolver.load_source(
      source_context, "unit/bad_swap.ttx"_view,
      "dialect : Library;\n"
      "public func swap[.first : Count, .seconx : Count, .second : Count] -> "
      "[\n"
      "  .first : Count, .second : Count\n"
      "] {\n"
      "  return (.first = second3, .second = first);\n"
      "}\n"_view));

  ASSERT(source_context.has_errors());
  EXPECT_TEXT(
      error_message(source_context),
      "Function has no parameter named `second3`."_view);
  EXPECT_TEXT(
      source_context.get_errors()[0].get_hint(), "Did you mean `second`?"_view);
  const Ttx::Lexical::Token* token =
      source_context.get_errors()[0].get_start_token();
  ASSERT(token != nullptr);
  EXPECT_TEXT(token->get_text(), "second3"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, package_exports) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context types_source_context;

  const Resolution::Source::Record* types = resolver.load_source(
      types_source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"_view);
  ASSERT(types != nullptr);
  EXPECT_NOT(types_source_context.has_errors());
  Resolution::Resolver::Context package_source_context;

  resolver.set_package_name("Test.Graphics"_view);
  const Resolution::Source::Record* package = resolver.load_source(
      package_source_context, "unit/package.ttx"_view,
      "dialect : Package;\n"
      "import Types : Library = \"types.ttx\";\n"
      "expose Color : alias = Types::Color;\n"_view);

  ASSERT(package != nullptr);
  EXPECT_NOT(package_source_context.has_errors());

  const Ttx::Type* library_color = nested_type(types, "Color"_view);
  const Ttx::Type* package_color = nested_type(package, "Color"_view);
  const Ttx::Type* package_type = source_type(package);
  ASSERT(package_type != nullptr);
  EXPECT_TEXT(package_type->get_name(), "Package"_view);
  const Ttx::Member* package_name =
      package_type->find_member("package_name"_view);
  ASSERT(package_name != nullptr);
  EXPECT_TEXT(package_name->get_type().get_name(), "Test.Graphics"_view);
  EXPECT(resolver.resolve("unit/package.ttx"_view) == package);
  EXPECT_NOT(resolver.resolve("Test.Graphics"_view));

  ASSERT(library_color != nullptr);
  ASSERT(package_color != nullptr);
  EXPECT(package_color->is_alias());
  EXPECT(&package_color->canonical() == library_color);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_alias) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_alias_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_alias_source_context, "unit/bad.ttx"_view,
      "dialect : Library;\n"
      "public Broken : alias = Missing;\n"_view));

  ASSERT(bad_alias_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_alias_source_context),
      "Library alias target could not be resolved."_view);
  EXPECT_NOT(resolver.resolve("unit/bad.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_definition) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);

  Resolution::Resolver::Context bad_modifier_source_context;
  EXPECT_NOT(resolver.load_source(
      bad_modifier_source_context, "unit/bad_modifier.ttx"_view,
      "dialect : Library;\n"
      "state Color : struct {\n"
      "}\n"_view));
  ASSERT(bad_modifier_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_modifier_source_context),
      "Expected a definition to start with one of the following modifiers "
      "{public, private, expose}"_view);

  Resolution::Resolver::Context bad_name_source_context;
  EXPECT_NOT(resolver.load_source(
      bad_name_source_context, "unit/bad_name.ttx"_view,
      "dialect : Library;\n"
      "public 7 : struct {\n"
      "}\n"_view));
  ASSERT(bad_name_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_name_source_context),
      "Definitions can only be created here for the following types "
      "{type, addressable identifier}"_view);

  Resolution::Resolver::Context bad_kind_source_context;
  EXPECT_NOT(resolver.load_source(
      bad_kind_source_context, "unit/bad_kind.ttx"_view,
      "dialect : Library;\n"
      "public Color : nope;\n"_view));
  ASSERT(bad_kind_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_kind_source_context),
      "Definition name provided is not one of the known types "
      "{alias, enum, struct, object, foreign}"_view);

  Resolution::Resolver::Context capital_alias_source_context;
  EXPECT_NOT(resolver.load_source(
      capital_alias_source_context, "unit/capital_alias.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "}\n"
      "public Tint : Alias = Color;\n"_view));
  ASSERT(capital_alias_source_context.has_errors());
  EXPECT_TEXT(
      error_message(capital_alias_source_context),
      "Definition name provided is not one of the known types "
      "{alias, enum, struct, object, foreign}"_view);

  Resolution::Resolver::Context unmatched_scope_source_context;
  EXPECT_NOT(resolver.load_source(
      unmatched_scope_source_context, "unit/unmatched_scope.ttx"_view,
      "dialect : Library;\n"
      "}\n"_view));
  ASSERT(unmatched_scope_source_context.has_errors());
  EXPECT_TEXT(
      error_message(unmatched_scope_source_context),
      "Expected a definition to start with one of the following modifiers "
      "{public, private, expose}"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_enum) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context missing_storage_source_context;

  EXPECT_NOT(resolver.load_source(
      missing_storage_source_context, "unit/bad_enum.ttx"_view,
      "dialect : Library;\n"
      "private Color : enum[Missing] { red = 1; }\n"_view));
  ASSERT(missing_storage_source_context.has_errors());
  EXPECT_TEXT(
      error_message(missing_storage_source_context),
      "Library enum storage type could not be resolved."_view);
  Resolution::Resolver::Context duplicate_case_source_context;

  EXPECT_NOT(resolver.load_source(
      duplicate_case_source_context, "unit/duplicate_enum.ttx"_view,
      "dialect : Library;\n"
      "private Color : enum[Bits_8] { red = 1; red = 2; }\n"_view));
  ASSERT(duplicate_case_source_context.has_errors());
  EXPECT_TEXT(
      error_message(duplicate_case_source_context),
      "Library enum case name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_function) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_function_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_function_source_context, "unit/bad_function.ttx"_view,
      "dialect : Library;\n"
      "public func main[] [] {\n"
      "}\n"_view));

  ASSERT(bad_function_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_function_source_context),
      "Expected `->` before library function result."_view);

  Resolution::Resolver::Context empty_return_source_context;
  EXPECT_NOT(resolver.load_source(
      empty_return_source_context, "unit/empty_return.ttx"_view,
      "dialect : Library;\n"
      "public func value[] -> [.result : Real_32] {\n"
      "  return;\n"
      "}\n"_view));
  ASSERT(empty_return_source_context.has_errors());
  EXPECT_TEXT(
      error_message(empty_return_source_context),
      "Return values do not fit the function result."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_foreign) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_foreign_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_foreign_source_context, "unit/bad_foreign.ttx"_view,
      "dialect : Library;\n"
      "public Sampler2D : foreign {\n"
      "  public func sample[] -> [Bits_8];\n"
      "}\n"_view));
  ASSERT(bad_foreign_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_foreign_source_context),
      "Expected a foreign function declaration to start with expose."_view);
  Resolution::Resolver::Context foreign_body_source_context;

  EXPECT_NOT(resolver.load_source(
      foreign_body_source_context, "unit/foreign_body.ttx"_view,
      "dialect : Library;\n"
      "public Sampler2D : foreign {\n"
      "  expose func sample[] -> [Bits_8] {\n"
      "  }\n"
      "}\n"_view));
  ASSERT(foreign_body_source_context.has_errors());
  EXPECT_TEXT(
      error_message(foreign_body_source_context),
      "Expected `;` after library function declaration."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, duplicate_type) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context duplicate_type_source_context;

  EXPECT_NOT(resolver.load_source(
      duplicate_type_source_context, "unit/duplicate.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "}\n"
      "public Color : alias = Bits_8;\n"_view));

  ASSERT(duplicate_type_source_context.has_errors());
  EXPECT_TEXT(
      error_message(duplicate_type_source_context),
      "Library type name is already defined."_view);
  EXPECT_NOT(resolver.resolve("unit/duplicate.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, duplicate_member) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context duplicate_member_source_context;

  EXPECT_NOT(resolver.load_source(
      duplicate_member_source_context, "unit/duplicate_member.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public r : Real_32;\n"
      "}\n"_view));
  ASSERT(duplicate_member_source_context.has_errors());
  EXPECT_TEXT(
      error_message(duplicate_member_source_context),
      "Library member name is already defined."_view);
  Resolution::Resolver::Context duplicate_root_member_source_context;

  EXPECT_NOT(resolver.load_source(
      duplicate_root_member_source_context, "unit/duplicate_root.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "}\n"
      "public color : Color;\n"
      "public color : Color;\n"_view));
  ASSERT(duplicate_root_member_source_context.has_errors());
  EXPECT_TEXT(
      error_message(duplicate_root_member_source_context),
      "Library member name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, struct_layout) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context point_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      point_source_context, "unit/point.ttx"_view,
      "dialect : Library;\n"
      "public Point2D : struct {\n"
      "  public x : Real_32;\n"
      "  public y : Real_32;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(point_source_context.has_errors());
  const Ttx::Type* point = nested_type(record, "Point2D"_view);
  ASSERT(point != nullptr);
  Ttx::Layout layout(*point);
  ASSERT_EQ(layout.get_member_count(), Count(2));
  EXPECT_TEXT(layout.member_at(0).get_name(), "x"_view);
  EXPECT_TEXT(layout.member_at(1).get_name(), "y"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, type_attribute) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context color_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      color_source_context, "unit/color.ttx"_view,
      "dialect : Library;\n"
      "@shader_type(Vec4D)\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public g : Real_32;\n"
      "  public b : Real_32;\n"
      "  public a : Real_32;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(color_source_context.has_errors());
  const Ttx::Type* color = nested_type(record, "Color"_view);
  ASSERT(color != nullptr);
  const Ttx::Attribute* attribute = color->find_attribute("shader_type"_view);
  ASSERT(attribute != nullptr);
  EXPECT_TEXT(attribute->get_value(), "Vec4D"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, func_layout) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context function_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      function_source_context, "unit/function.ttx"_view,
      "dialect : Library;\n"
      "public func point[.x : Real_32, .y : Real_32] -> [.size : Real_32] {\n"
      "  return 0.0;\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(function_source_context.has_errors());
  ASSERT(source_type(record) != nullptr);
  const Ttx::Function* point = function(*source_type(record), "point"_view);
  ASSERT(point != nullptr);
  ASSERT_EQ(point->get_parameters().get_member_count(), Count(2));
  ASSERT_EQ(point->get_result().get_member_count(), Count(1));
  EXPECT_TEXT(point->get_parameters().member_at(0).get_name(), "x"_view);
  EXPECT_TEXT(point->get_parameters().member_at(1).get_name(), "y"_view);
  EXPECT_TEXT(point->get_result().member_at(0).get_name(), "size"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, dupe_function) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context duplicate_function_source_context;

  EXPECT_NOT(resolver.load_source(
      duplicate_function_source_context, "unit/duplicate_function.ttx"_view,
      "dialect : Library;\n"
      "public func main[] -> [] { return; }\n"
      "public func main[] -> [] { return; }\n"_view));

  ASSERT(duplicate_function_source_context.has_errors());
  EXPECT_TEXT(
      error_message(duplicate_function_source_context),
      "Library function name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_member_type) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_member_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_member_source_context, "unit/bad_member.ttx"_view,
      "dialect : Library;\n"
      "public value : MissingType;\n"_view));

  ASSERT(bad_member_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_member_source_context),
      "Library member type could not be resolved."_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, missing_body_end) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_body_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_body_source_context, "unit/bad_body.ttx"_view,
      "dialect : Library;\n"
      "public func main[] -> [] {\n"
      "  return;\n"_view));

  ASSERT(bad_body_source_context.has_errors());
  EXPECT_TEXT(
      error_message(bad_body_source_context),
      "Expected `}` after library function body."_view);
}
