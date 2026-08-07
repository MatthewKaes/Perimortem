// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static_assert(!__is_constructible(
    Language::Types::Structure,
    const Language::Types::Structure&));
static_assert(!__is_constructible(
    Language::Types::Structure,
    Language::Types::Structure&&));

class EmptyContext : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "EmptyContext"_view;
  }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "StructureTest"_view, "structure.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty();
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph) {
    return False;
  }

  Bool linked = workspace.link(errors);
  return !linked && !errors.is_empty();
}

static auto rejects_finalize(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph || !workspace.link(errors)) {
    return False;
  }

  Bool finalized = workspace.finalize(errors);
  return !finalized && !errors.is_empty();
}

static Harness StructureTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Structure"_view,
};

PERIMORTEM_UNIT_TEST(StructureTests, stable_authored_graph) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Later : struct { public value : Unsigned_64; }\n"
      "private Hidden : struct { public flag : Bool; }\n"
      "public Packet : struct {\n"
      "  // Later field.\n"
      "  public later : Later;\n"
      "  private hidden : Hidden;\n"
      "  public func inspect[] -> Packet {}\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto structures = monograph->get_structures();
  auto public_structures = monograph->get_public_structures();
  ASSERT_EQ(structures.get_size(), Count(3));
  ASSERT_EQ(public_structures.get_size(), Count(2));
  auto& packet = structures.get_data()[2].get();
  const Abstract& reserved = monograph->resolve_context("Packet"_view);
  EXPECT(&reserved == &packet);
  EXPECT(&packet.resolve() == &Invalid::get_invalid());
  EXPECT_TEXT(
      packet.get_anchor().get_span().caculate_text(source),
      "public Packet : struct {\n"
      "  // Later field.\n"
      "  public later : Later;\n"
      "  private hidden : Hidden;\n"
      "  public func inspect[] -> Packet {}\n"
      "}"_view);
  EXPECT_TEXT(
      packet.get_anchor().get_token().caculate_text(source), "struct"_view);
  EXPECT_TEXT(
      packet.get_name_anchor().get_span().caculate_text(source), "Packet"_view);
  auto authored_fields = packet.get_authored_fields();
  ASSERT_EQ(authored_fields.get_size(), Count(2));
  const Language::Field& authored_later = authored_fields.get_data()[0];
  EXPECT(authored_later.get_visibility() == Language::Visibility::Public);
  EXPECT_TEXT(authored_later.get_name(), "later"_view);
  EXPECT_TEXT(authored_later.get_type_route(), "Later"_view);
  EXPECT_NOT(authored_later.get_type());
  EXPECT_NOT(authored_later.get_addressable());
  EXPECT_NOT(authored_later.get_documentation().is_empty());
  EXPECT_TEXT(
      authored_later.get_anchor().get_span().caculate_text(source),
      "public later : Later;"_view);
  EXPECT_TEXT(
      authored_later.get_type_anchor().get_span().caculate_text(source),
      "Later"_view);

  ASSERT(workspace.link(errors));
  EXPECT(&packet.resolve() == &packet);
  ASSERT(workspace.finalize(errors));
  EXPECT(packet.is_finalized());

  auto fields = packet.get_fields();
  auto public_fields = packet.get_public_fields();
  auto callables = packet.get_callables();
  auto public_callables = packet.get_public_callables();
  ASSERT_EQ(fields.get_size(), Count(2));
  ASSERT_EQ(public_fields.get_size(), Count(1));
  ASSERT_EQ(callables.get_size(), Count(1));
  ASSERT_EQ(public_callables.get_size(), Count(1));
  EXPECT_TEXT(fields.get_data()[0].get().get_name(), "later"_view);
  EXPECT_TEXT(fields.get_data()[1].get().get_name(), "hidden"_view);
  EXPECT(
      &fields.get_data()[0].get().get_type() ==
      &structures.get_data()[0].get());
  EXPECT(
      &fields.get_data()[1].get().get_type() ==
      &structures.get_data()[1].get());
  EXPECT(&public_fields.get_data()[0].get() == &fields.get_data()[0].get());
  ASSERT(authored_later.get_addressable());
  EXPECT(&*authored_later.get_addressable() == &fields.get_data()[0].get());
  EXPECT(&packet.resolve_context("later"_view) == &fields.get_data()[0].get());
  EXPECT(
      &packet.resolve_context("inspect"_view) ==
      &callables.get_data()[0].get());
  EXPECT(&packet.resolve_context("Later"_view) == &Invalid::get_invalid());

  const Layout& layout = packet.get_layout();
  ASSERT_EQ(layout.get_size(), Count(2));
  ASSERT(layout.get_abstract(0));
  ASSERT(layout.get_abstract(1));
  EXPECT(&*layout.get_abstract(0) == &fields.get_data()[0].get());
  EXPECT(&*layout.get_abstract(1) == &fields.get_data()[1].get());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, declaration_reorder) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public First : struct { public next : Second; }\n"
    "public Second : struct { public value : Unsigned_64; }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Second : struct { public value : Unsigned_64; }\n"
    "public First : struct { public next : Second; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[i]);
    ASSERT(monograph);
    ASSERT(workspace.link(errors));
    ASSERT(workspace.finalize(errors));

    const Abstract& first = monograph->resolve_context("First"_view);
    const Abstract& second = monograph->resolve_context("Second"_view);
    ASSERT(first.is<Language::Types::Structure>());
    ASSERT(second.is<Language::Types::Structure>());
    const Abstract& field = first.resolve_context("next"_view);
    ASSERT(field.is<Addressable>());
    EXPECT(&static_cast<const Addressable&>(field).get_type() == &second);
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, exact_collision_domain) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; private value : Bool; }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public func value[] -> [] {} private value : Bool; }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; private func value[] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public func value[] -> [] {} private func value[] -> [] {} }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 9> sources = {{
    "// Structure test.\ndialect : Library; public Packet struct {}"_view,
    "// Structure test.\ndialect : Library; public Packet : wrong {}"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public Value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Bool }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Bool = true; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Core ::Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Core:: Bool; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, missing_type_rejected) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public missing : Missing; }"_view;
  EXPECT(rejects_link(source));
}

PERIMORTEM_UNIT_TEST(StructureTests, public_field_exposure_rejected) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "private Hidden : struct {}\n"
      "public Packet : struct { public hidden : Hidden; }"_view;
  EXPECT(rejects_finalize(source));
}

PERIMORTEM_UNIT_TEST(StructureTests, public_callable_exposure_rejected) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct {}\n"
    "public Packet : struct { public func reveal[Hidden] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct {}\n"
    "public Packet : struct { public func reveal[] -> Hidden {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public func reveal[Hidden] -> [] {}\n"
    "private Hidden : struct {}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_finalize(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, private_exposure_retained_locally) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "private Hidden : struct {}\n"
      "public Packet : struct {\n"
      "  private hidden : Hidden;\n"
      "  private func reveal[Hidden] -> Hidden {}\n"
      "}\n"
      "private func root[Hidden] -> Hidden {}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(monograph->resolve_context("Hidden"_view)
             .is<Language::Types::Structure>());
  EXPECT(monograph->resolve_context("root"_view).is<Language::Function>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, repeat_lifecycle) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public value : Unsigned_64; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(monograph->link());
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  ASSERT(monograph->finalize());
  EXPECT(monograph->get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, lifecycle_order_rejected) {
  static constexpr View::Bytes source =
      "public Packet : struct { public value : Bool; }"_view;
  EmptyContext context;
  Allocator::Arena arena;
  Dialect dialect(context);
  Language::Materializations materializations(arena);
  auto& monograph = Language::Monograph::create_authored(
      arena, Documentation::get_empty(), dialect, context, materializations);
  Errors errors;
  Tokenizer tokenizer(arena, source, "structure-stage.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto structure = Language::Types::Structure::interpret(
      arena, cursor, Documentation::get_empty(), monograph, materializations);
  ASSERT(structure);

  EXPECT_NOT(structure->link_callable_signatures());
  EXPECT_NOT(structure->link_callable_bodies());
  EXPECT_NOT(structure->finalize());
  EXPECT_EQ(monograph.get_diagnostics().get_size(), Count(3));
  EXPECT_NOT(structure->is_linked());
  EXPECT_NOT(structure->is_finalized());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, field_cursor_atomicity) {
  static constexpr View::Bytes malformed = "public value : Bool"_view;
  static constexpr View::Bytes complete = "private value : Core::Bool;"_view;
  Allocator::Arena arena;
  Errors malformed_errors;
  Tokenizer malformed_tokenizer(arena, malformed, "malformed-field.ttx"_view);
  Cursor malformed_cursor(malformed_tokenizer, malformed_errors);
  Token opening = malformed_cursor.current();
  auto rejected =
      Language::Field::interpret(malformed_cursor, Documentation::get_empty());
  EXPECT_NOT(rejected);
  EXPECT_EQ(malformed_cursor.current().get_offset(), opening.get_offset());
  EXPECT_NOT(malformed_errors.is_empty());

  Errors complete_errors;
  Tokenizer complete_tokenizer(arena, complete, "complete-field.ttx"_view);
  Cursor complete_cursor(complete_tokenizer, complete_errors);
  auto field =
      Language::Field::interpret(complete_cursor, Documentation::get_empty());
  ASSERT(field);
  EXPECT_TEXT(field->get_name(), "value"_view);
  EXPECT_TEXT(field->get_type_route(), "Core::Bool"_view);
  EXPECT_TEXT(field->get_anchor().get_span().caculate_text(complete), complete);
  EXPECT_TEXT(
      field->get_type_anchor().get_span().caculate_text(complete),
      "Core::Bool"_view);
  EXPECT(field->get_visibility() == Language::Visibility::Private);
  EXPECT(complete_cursor.matches(Code::Type::Terminal));
  EXPECT(complete_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, cursor_atomicity) {
  static constexpr View::Bytes malformed =
      "public Packet : struct { public value : Bool }"_view;
  static constexpr View::Bytes complete =
      "public Packet : struct { public value : Bool; }"_view;
  EmptyContext context;
  Allocator::Arena arena;
  Dialect dialect(context);
  Language::Materializations materializations(arena);
  auto& monograph = Language::Monograph::create_authored(
      arena, Documentation::get_empty(), dialect, context, materializations);

  Errors malformed_errors;
  Tokenizer malformed_tokenizer(
      arena, malformed, "malformed-structure.ttx"_view);
  Cursor malformed_cursor(malformed_tokenizer, malformed_errors);
  Token opening = malformed_cursor.current();
  auto rejected = Language::Types::Structure::interpret(
      arena, malformed_cursor, Documentation::get_empty(), monograph,
      materializations);
  EXPECT_NOT(rejected);
  EXPECT_EQ(malformed_cursor.current().get_offset(), opening.get_offset());
  EXPECT(malformed_cursor.current().get_code() == opening.get_code());
  EXPECT_NOT(malformed_errors.is_empty());

  Errors complete_errors;
  Tokenizer complete_tokenizer(arena, complete, "complete-structure.ttx"_view);
  Cursor complete_cursor(complete_tokenizer, complete_errors);
  auto parsed = Language::Types::Structure::interpret(
      arena, complete_cursor, Documentation::get_empty(), monograph,
      materializations);
  ASSERT(parsed);
  EXPECT(complete_cursor.matches(Code::Type::Terminal));
  EXPECT(complete_errors.is_empty());
}
