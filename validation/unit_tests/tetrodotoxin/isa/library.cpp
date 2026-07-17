// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/abi/type.hpp"
#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/isa/library/staged_declaration.hpp"
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
  return type.find_type_function(name);
}

static auto error_message(
    const Resolution::Resolver::Context& source_context,
    Count index = 0) -> View::Bytes {
  return source_context.get_errors()[index].get_message();
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, staged_failure_revokes_type) {
  Perimortem::Memory::Allocator::Arena arena;
  Ttx::Type* reserved = arena.reserve<Ttx::Type>();
  Tetrodotoxin::Isa::Library::StagedDeclaration staged(
      Tetrodotoxin::Isa::Base::Declaration(), {4, 8});

  EXPECT(staged.begin_evaluation());
  EXPECT(staged.find_type() == nullptr);
  EXPECT(staged.stage_type(reserved));
  EXPECT(staged.find_type() == reserved);
  Ttx::Type other("Other"_view);
  EXPECT_NOT(staged.complete(other));

  staged.fail();
  EXPECT(
      staged.get_state() ==
      Tetrodotoxin::Isa::Library::StagedDeclaration::State::Failed);
  EXPECT(staged.find_type() == nullptr);
  EXPECT_NOT(staged.stage_type(reserved));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, reflected_type_value) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      source_context, "unit/reflection.ttx"_view,
      "dialect : Library;\n"
      "public Window : struct {\n"
      "}\n"
      "const reflected_type : Type = Window;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(source_context.has_errors());
  const Ttx::Type* library = source_type(record);
  const Ttx::Type* window = nested_type(record, "Window"_view);
  const Ttx::Type* meta_type =
      Tetrodotoxin::Standard::Types::find_type("Type"_view);
  ASSERT(library != nullptr);
  ASSERT(window != nullptr);
  ASSERT(meta_type != nullptr);

  const Ttx::Member* reflected = library->find_member("reflected_type"_view);
  ASSERT(reflected != nullptr);
  EXPECT(reflected->get_type().equivalent_to(*meta_type));
  const auto* definition = record->get_implementation().find(*reflected);
  ASSERT(definition != nullptr);
  ASSERT(definition->has_initializer());
  EXPECT(
      definition->get_initializer().get_kind() ==
      Tetrodotoxin::Isa::Base::Expression::Value::Kind::Type);
  EXPECT(&definition->get_initializer().get_type() == window);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, type_model) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public g : Real_32;\n"
      "}\n"
      "public Tint : alias = Color;\n"
      "private LocalColor : alias = Color;\n"
      "public Sprite : struct {\n"
      "  public image : View[Bytes];\n"
      "  public values : Vec[Unsigned_8, 4];\n"
      "  public copy : Vec[Unsigned_8, 4];\n"
      "}\n"
      "public Screen : struct {}\n"
      "public screen : Screen;\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(source_context.has_errors());
  const Ttx::Type& library = record->get_type();
  EXPECT_TEXT(library.get_name(), "Library"_view);

  const Ttx::Type* color = library.find_type("Color"_view);
  const Ttx::Type* tint = library.find_type("Tint"_view);
  const Ttx::Type* local_color = library.find_type("LocalColor"_view);
  const Ttx::Type* screen = library.find_type("Screen"_view);
  ASSERT(color != nullptr);
  ASSERT(tint != nullptr);
  ASSERT(local_color != nullptr);
  ASSERT(screen != nullptr);
  EXPECT_EQ(color->get_members().get_size(), Count(2));
  EXPECT_TEXT(member_type(*color, "r"_view)->get_name(), "Real_32"_view);
  EXPECT_TEXT(member_type(*color, "g"_view)->get_name(), "Real_32"_view);
  EXPECT(tint->is_alias());
  EXPECT(local_color->is_alias());
  EXPECT(&tint->canonical() == color);
  EXPECT(&local_color->canonical() == color);
  EXPECT(member_type(library, "screen"_view) == screen);

  const Ttx::Type* sprite = library.find_type("Sprite"_view);
  ASSERT(sprite != nullptr);
  const Ttx::Type* image = member_type(*sprite, "image"_view);
  ASSERT(image != nullptr);
  EXPECT_TEXT(image->get_name(), "View[Bytes]"_view);
  EXPECT(
      Tetrodotoxin::Abi::Lowering(
          image->resolve_attribute("abi"_view).get_unsigned()) ==
      Tetrodotoxin::Abi::Lowering::ViewBytes);

  const Ttx::Type* values = member_type(*sprite, "values"_view);
  const Ttx::Type* copy = member_type(*sprite, "copy"_view);
  ASSERT(values != nullptr);
  ASSERT(copy != nullptr);
  EXPECT_TEXT(values->get_name(), "Vec[Unsigned_8,4]"_view);
  EXPECT(values == copy);
  EXPECT(values != Tetrodotoxin::Standard::Types::find_type("Vec"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, enum_type) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context enum_source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      enum_source_context, "unit/enums.ttx"_view,
      "dialect : Library;\n"
      "private Color : enum[Unsigned_8] {\n"
      "  red = 1;\n"
      "  green = 2;\n"
      "}\n"
      "private Filter : enum[Unsigned_8] {\n"
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
  EXPECT_TEXT(red->get_type().get_name(), "Unsigned_8"_view);
  EXPECT_TEXT(member_type(*color, "green"_view)->get_name(), "Unsigned_8"_view);
  const auto* red_facts = record->get_implementation().find(*red);
  ASSERT(red_facts != nullptr);
  EXPECT(red_facts->get_modifier() == Ttx::Lexical::Class::Type::Expose);
  ASSERT(red_facts->has_initializer());
  EXPECT_TEXT(red_facts->get_initializer().get_value(), "1"_view);

  const Ttx::Type* filter = nested_type(record, "Filter"_view);
  ASSERT(filter != nullptr);
  EXPECT_TEXT(member_type(*filter, "none"_view)->get_name(), "Unsigned_8"_view);
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
  const auto* definition =
      record->get_implementation().find_definition(*sample);
  ASSERT(definition != nullptr);
  EXPECT(definition->get_modifier() == Ttx::Lexical::Class::Type::Expose);
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
      "  Api::Console -> print(\"Hello\");\n"
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
      "  Api -> echo(\"Hello\");\n"
      "}\n"_view);
  ASSERT(consumer != nullptr);
  EXPECT_NOT(consumer_context.has_errors());

  const Ttx::Function* main = function(consumer->get_type(), "main"_view);
  ASSERT(main != nullptr);
  const auto* body = consumer->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*main);
  ASSERT(body != nullptr);
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Call>(), Count(1));
  View::Bytes symbol =
      body->find<Tetrodotoxin::Compiler::Execution::Call>()->get_symbol();
  ASSERT(symbol.get_size() >= Count(6));
  EXPECT_TEXT(symbol.slice(0, 13), "ttx_internal_"_view);
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
      "public func main[@slot(0) .scene : SceneConfig] -> [] {\n"
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
      "  Console -> print(\"Hi\\n\");\n"
      "  Console -> print(data);\n"
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
  ASSERT(value->find<Unsigned_64>() != nullptr);
  EXPECT_EQ(*value->find<Unsigned_64>(), Unsigned_64(42));
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
  ASSERT(constant->find<Unsigned_64>() != nullptr);
  EXPECT_EQ(*constant->find<Unsigned_64>(), Unsigned_64(5));
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
  ASSERT(source_context.get_errors()[0].has_tokens());
  const Ttx::Lexical::Token& token =
      source_context.get_errors()[0].get_start_token();
  EXPECT_TEXT(token.get_text(), "second3"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, package_exports) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "Test.Graphics"_view);
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
  EXPECT(package_type->find_member("package_name"_view) == nullptr);
  EXPECT(resolver.resolve("unit/package.ttx"_view) == package);
  EXPECT_NOT(resolver.resolve("Test.Graphics"_view));

  ASSERT(library_color != nullptr);
  ASSERT(package_color != nullptr);
  EXPECT(package_color->is_alias());
  EXPECT(&package_color->canonical() == library_color);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, diagnostics) {
  struct Failure {
    View::Bytes source;
    View::Bytes message;
  };
  constexpr Failure failures[] = {
    {
      "dialect : Library;\n"
      "public Value : struct { public data : Unsigned_32[Bytes]; }\n"_view,
      "Type arguments are not supported for this type."_view,
    },
    {
      "dialect : Library;\npublic Broken : alias = Missing;\n"_view,
      "Library alias target could not be resolved."_view,
    },
    {
      "dialect : Library;\nstate Color : struct {}\n"_view,
      "Expected a definition to start with one of the following modifiers "
      "{public, private, expose, const}"_view,
    },
    {
      "dialect : Library;\npublic Color : nope;\n"_view,
      "Definition name provided is not one of the known types "
      "{alias, enum, struct, object, foreign}"_view,
    },
    {
      "dialect : Library;\n"
      "private Color : enum[Missing] { red = 1; }\n"_view,
      "Library enum storage type could not be resolved."_view,
    },
    {
      "dialect : Library;\n"
      "private Color : enum[Unsigned_8] { red = 1; red = 2; }\n"_view,
      "Library enum case name is already defined."_view,
    },
    {
      "dialect : Library;\npublic func main[] [] {}\n"_view,
      "Expected `->` before library function result."_view,
    },
    {
      "dialect : Library;\n"
      "public func value[] -> [.result : Real_32] { return; }\n"_view,
      "Return values do not fit the function result."_view,
    },
    {
      "dialect : Library;\n"
      "public Sampler : foreign { public func sample[] -> []; }\n"_view,
      "Expected a foreign function declaration to start with expose."_view,
    },
    {
      "dialect : Library;\n"
      "public Sampler : foreign { expose func sample[] -> [] {} }\n"_view,
      "Expected `;` after library function declaration."_view,
    },
    {
      "dialect : Library;\n"
      "public Color : struct {}\npublic Color : alias = Unsigned_8;\n"_view,
      "Library type name is already defined."_view,
    },
    {
      "dialect : Library;\n"
      "public Color : struct { public r : Real_32; "
      "public r : Real_32; }\n"_view,
      "Library member name is already defined."_view,
    },
    {
      "dialect : Library;\n"
      "public func invalid[self] -> [] { return; }\n"_view,
      "`self` is only valid in an Addressable function layout."_view,
    },
    {
      "dialect : Library;\n"
      "public Counter : struct { public func invalid[.value : Count, self] "
      "-> Count { return value; } }\n"_view,
      "`self` must be the first function layout entry."_view,
    },
    {
      "dialect : Library;\n"
      "public func main[] -> [] { return; }\n"
      "public func main[] -> [] { return; }\n"_view,
      "Library function name is already defined."_view,
    },
    {
      "dialect : Library;\npublic value : MissingType;\n"_view,
      "Library member type could not be resolved."_view,
    },
    {
      "dialect : Library;\npublic func main[] -> [] { return;\n"_view,
      "Expected `}` after library function body."_view,
    },
  };
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  for (Count i = 0; i < Count(sizeof(failures) / sizeof(Failure)); i++) {
    Resolution::Resolver::Context source_context;
    EXPECT_NOT(resolver.load_source(
        source_context, "unit/error.ttx"_view, failures[i].source));
    ASSERT(source_context.has_errors());
    EXPECT_TEXT(error_message(source_context), failures[i].message);
  }
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
  EXPECT_TEXT(attribute->get_bytes(), "Vec4D"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, bad_abi_attribute) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  EXPECT_NOT(resolver.load_source(
      source_context, "unit/bad_abi.ttx"_view,
      "dialect : Library;\n"
      "@abi(unknown)\n"
      "public Value : struct {}\n"_view));
  EXPECT(source_context.has_errors());
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

PERIMORTEM_UNIT_TEST(TetrodotoxinLibrary, function_surfaces) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context source_context;

  const Resolution::Source::Record* record = resolver.load_source(
      source_context, "unit/function_surfaces.ttx"_view,
      "dialect : Library;\n"
      "public Counter : struct {\n"
      "  public func identity[.value : Count] -> Count {\n"
      "    return value;\n"
      "  }\n"
      "  public func identity[self] -> Counter {\n"
      "    return self;\n"
      "  }\n"
      "}\n"_view);

  ASSERT(record != nullptr);
  EXPECT_NOT(source_context.has_errors());
  const Ttx::Type* counter = nested_type(record, "Counter"_view);
  ASSERT(counter != nullptr);
  const Ttx::Function* type_identity =
      counter->find_type_function("identity"_view);
  const Ttx::Function* addressable_identity =
      counter->find_addressable_function("identity"_view);
  ASSERT(type_identity != nullptr);
  ASSERT(addressable_identity != nullptr);
  EXPECT(&counter->get_type_functions()[0] == type_identity);
  EXPECT(&counter->get_addressable_functions()[0] == addressable_identity);
  EXPECT_EQ(type_identity->get_parameters().get_member_count(), Count(1));
  EXPECT_EQ(
      addressable_identity->get_parameters().get_member_count(), Count(1));
}
