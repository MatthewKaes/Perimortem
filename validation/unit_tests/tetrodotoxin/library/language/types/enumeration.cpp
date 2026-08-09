// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/enumeration.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

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
    Language::Types::Enumeration,
    const Language::Types::Enumeration&));
static_assert(!__is_constructible(
    Language::Types::Enumeration,
    Language::Types::Enumeration&&));

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
      errors, "EnumerationTest"_view, "enumeration.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty() &&
         &workspace.resolve_context("EnumerationTest"_view) ==
             &Invalid::get_invalid();
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph) {
    return False;
  }

  Bool linked = workspace.link(errors);
  return !linked && !errors.is_empty() &&
         &workspace.resolve_context("EnumerationTest"_view) ==
             &Invalid::get_invalid();
}

static auto rejects_finalize_without_cases(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph || !workspace.link(errors)) {
    return False;
  }

  auto bindings = monograph->get_authored_bindings();
  if (bindings.is_empty() ||
      !bindings.get_data()[0].get().is<Language::Types::Enumeration>()) {
    return False;
  }

  Bool finalized = workspace.finalize(errors);
  const auto& enumeration = static_cast<const Language::Types::Enumeration&>(
      bindings.get_data()[0].get());
  auto cases = enumeration.get_cases();
  return !finalized && cases.is_empty() && !errors.is_empty() &&
         &workspace.resolve_context("EnumerationTest"_view) ==
             &Invalid::get_invalid();
}

static Harness EnumerationTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Enumeration"_view,
};

PERIMORTEM_UNIT_TEST(EnumerationTests, stable_authored_graph) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "// Mode docs.\n"
      "public Mode : enum[Unsigned_8] {\n"
      "  // Idle docs.\n"
      "  idle = 0;\n"
      "  active = 1;\n"
      "  paused = 0x2;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Enumeration>());
  const auto& mode = static_cast<const Language::Types::Enumeration&>(
      bindings.get_data()[0].get());
  const auto& source_structure =
      static_cast<const Language::Types::Structure&>(monograph->get_source());
  auto exposed = source_structure.get_external_static_bindings();
  ASSERT_EQ(exposed.get_size(), Count(1));
  EXPECT(&exposed.get_data()[0].get() == &mode);
  EXPECT(&monograph->resolve_context("source"_view) == &source_structure);
  EXPECT(&monograph->resolve_context("Mode"_view) == &mode);
  EXPECT(&mode.resolve() == &Invalid::get_invalid());
  EXPECT_NOT(mode.get_storage_type());
  EXPECT(mode.get_cases().is_empty());
  EXPECT_EQ(mode.get_case_count(), Count(3));
  EXPECT_NOT(mode.get_documentation().is_empty());
  EXPECT_TEXT(
      mode.get_anchor().get_span().caculate_text(source),
      "public Mode : enum[Unsigned_8] {\n"
      "  // Idle docs.\n"
      "  idle = 0;\n"
      "  active = 1;\n"
      "  paused = 0x2;\n"
      "}"_view);
  EXPECT_TEXT(mode.get_anchor().get_token().caculate_text(source), "enum"_view);
  EXPECT_TEXT(
      mode.get_name_anchor().get_span().caculate_text(source), "Mode"_view);
  EXPECT_TEXT(
      mode.get_storage_anchor().get_span().caculate_text(source),
      "Unsigned_8"_view);
  ASSERT(mode.get_case_anchor(0));
  ASSERT(mode.get_case_name_anchor(0));
  ASSERT(mode.get_case_value_anchor(0));
  EXPECT_TEXT(
      mode.get_case_anchor(0)->get_span().caculate_text(source),
      "idle = 0;"_view);
  EXPECT_TEXT(
      mode.get_case_name_anchor(0)->get_span().caculate_text(source),
      "idle"_view);
  EXPECT_TEXT(
      mode.get_case_value_anchor(0)->get_span().caculate_text(source),
      "0"_view);
  EXPECT_NOT(mode.get_case_anchor(3));

  ASSERT(workspace.link(errors));
  ASSERT(mode.get_storage_type());
  EXPECT(&*mode.get_storage_type() == &Dialect::get_unsigned_8());
  EXPECT(&mode.get_layout() == &Dialect::get_unsigned_8().get_layout());
  EXPECT(&mode.resolve() == &mode);
  EXPECT(&mode.resolve_context("idle"_view) == &Invalid::get_invalid());

  ASSERT(workspace.finalize(errors));
  auto cases = mode.get_cases();
  ASSERT_EQ(cases.get_size(), Count(3));
  EXPECT_TEXT(cases.get_data()[0].get().get_name(), "idle"_view);
  EXPECT_TEXT(cases.get_data()[1].get().get_name(), "active"_view);
  EXPECT_TEXT(cases.get_data()[2].get().get_name(), "paused"_view);
  EXPECT_NOT(cases.get_data()[0].get().get_documentation().is_empty());
  EXPECT(&mode.resolve_context("active"_view) == &cases.get_data()[1].get());
  EXPECT(&mode.resolve_context("missing"_view) == &Invalid::get_invalid());

  Static::Vector<Unsigned_64, 3> expected = {{0, 1, 2}};
  for (Count i = 0; i < cases.get_size(); i++) {
    const Alias& alias = cases.get_data()[i].get();
    const Abstract& resolved = alias.resolve();
    ASSERT(resolved.is<Language::Constants::Unsigned>());
    const auto& constant =
        static_cast<const Language::Constants::Unsigned&>(resolved);
    EXPECT(&constant.get_type() == &Dialect::get_unsigned_8());
    EXPECT_EQ(constant.get_value(), expected[i]);
    ASSERT(constant.get_anchor());
    EXPECT(
        constant.get_anchor()->get_span() ==
        mode.get_case_value_anchor(i)->get_span());
  }
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, signed_values_and_equal_aliases) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "public Offset : enum[Signed_8] {\n"
      "  low = -128;\n"
      "  zero = 0;\n"
      "  high = 127;\n"
      "  hexadecimal = 0x7F;\n"
      "  same = 127;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Enumeration>());
  const auto& offset = static_cast<const Language::Types::Enumeration&>(
      bindings.get_data()[0].get());
  auto cases = offset.get_cases();
  ASSERT_EQ(cases.get_size(), Count(5));
  Static::Vector<Signed_64, 5> expected = {{-128, 0, 127, 127, 127}};
  for (Count i = 0; i < cases.get_size(); i++) {
    const Abstract& resolved = cases.get_data()[i].get().resolve();
    ASSERT(resolved.is<Language::Constants::Signed>());
    const auto& constant =
        static_cast<const Language::Constants::Signed&>(resolved);
    EXPECT(&constant.get_type() == &Dialect::get_signed_8());
    EXPECT_EQ(constant.get_value(), expected[i]);
  }
  EXPECT(&cases.get_data()[2].get() != &cases.get_data()[4].get());
  EXPECT(
      &cases.get_data()[2].get().resolve() !=
      &cases.get_data()[4].get().resolve());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, binary_wide_boundaries) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "public UnsignedEdge : enum[Unsigned_64] {\n"
      "  decimal = 18446744073709551615;\n"
      "  hexadecimal = 0xFFFFFFFFFFFFFFFF;\n"
      "}\n"
      "public SignedEdge : enum[Signed_64] {\n"
      "  low = -9223372036854775808;\n"
      "  high = 9223372036854775807;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(2));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Enumeration>());
  ASSERT(bindings.get_data()[1].get().is<Language::Types::Enumeration>());
  const auto& unsigned_edge = static_cast<const Language::Types::Enumeration&>(
      bindings.get_data()[0].get());
  const auto& signed_edge = static_cast<const Language::Types::Enumeration&>(
      bindings.get_data()[1].get());
  auto unsigned_cases = unsigned_edge.get_cases();
  auto signed_cases = signed_edge.get_cases();
  ASSERT_EQ(unsigned_cases.get_size(), Count(2));
  ASSERT_EQ(signed_cases.get_size(), Count(2));
  for (Count i = 0; i < unsigned_cases.get_size(); i++) {
    const auto& constant = static_cast<const Language::Constants::Unsigned&>(
        unsigned_cases.get_data()[i].get().resolve());
    EXPECT_EQ(constant.get_value(), Unsigned_64(-1));
  }
  const auto& low = static_cast<const Language::Constants::Signed&>(
      signed_cases.get_data()[0].get().resolve());
  const auto& high = static_cast<const Language::Constants::Signed&>(
      signed_cases.get_data()[1].get().resolve());
  EXPECT_EQ(low.get_value(), Signed_64(-9223372036854775807LL - 1));
  EXPECT_EQ(high.get_value(), Signed_64(9223372036854775807LL));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, overflow_publishes_no_cases) {
  static constexpr Static::Vector<View::Bytes, 7> sources = {{
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Unsigned_8] { value = 256; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Signed_8] { value = 128; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Signed_8] { value = -129; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Unsigned_8] { value = -1; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Unsigned_64] { value = 18446744073709551616; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Signed_64] { value = 9223372036854775808; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Signed_64] { value = 0x8000000000000000; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_finalize_without_cases(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, invalid_storage_rejected) {
  static constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Bool] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Real_32] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Packet : struct {}\n"
    "public Bad : enum[Packet] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Mode : enum[Unsigned_8] { value = 0; }\n"
    "public Bad : enum[Mode] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Fixed] { value = 0; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, visibility_and_authored_order) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "private Hidden : enum[Signed_16] { hidden = -1; }\n"
      "public First : enum[Unsigned_16] { first = 1; }\n"
      "public Second : enum[Unsigned_32] { second = 2; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(3));
  EXPECT_TEXT(bindings.get_data()[0].get().get_name(), "Hidden"_view);
  EXPECT_TEXT(bindings.get_data()[1].get().get_name(), "First"_view);
  EXPECT_TEXT(bindings.get_data()[2].get().get_name(), "Second"_view);
  const auto& source_structure =
      static_cast<const Language::Types::Structure&>(monograph->get_source());
  auto exposed = source_structure.get_external_static_bindings();
  ASSERT_EQ(exposed.get_size(), Count(2));
  EXPECT(&exposed.get_data()[0].get() == &bindings.get_data()[1].get());
  EXPECT(&exposed.get_data()[1].get() == &bindings.get_data()[2].get());
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, consumer_declaration_reorder) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Enumeration test.\ndialect : Library;\n"
    "public Mode : enum[Unsigned_8] { ready = 1; }\n"
    "public Packet : struct { public mode : Mode; }\n"
    "public func select[Mode] -> Mode {}"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public func select[Mode] -> Mode {}\n"
    "public Packet : struct { public mode : Mode; }\n"
    "public Mode : enum[Unsigned_8] { ready = 1; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[i]);
    ASSERT(monograph);
    ASSERT(workspace.link(errors));
    ASSERT(workspace.finalize(errors));

    const Abstract& mode = monograph->resolve_context("Mode"_view);
    const Abstract& packet = monograph->resolve_context("Packet"_view);
    const auto& source_structure =
        static_cast<const Language::Types::Structure&>(monograph->get_source());
    auto callable_candidates =
        source_structure.get_callable_bindings(*monograph);
    ASSERT_EQ(callable_candidates.get_size(), Count(1));
    const Abstract& function = callable_candidates.get_data()[0].get();
    ASSERT(mode.is<Language::Types::Enumeration>());
    ASSERT(packet.is<Language::Types::Structure>());
    ASSERT(function.is<Language::Function>());
    const Abstract& field = packet.resolve_context("mode"_view);
    ASSERT(field.is<Addressable>());
    EXPECT(&static_cast<const Addressable&>(field).get_type() == &mode);
    const auto& selected = static_cast<const Language::Function&>(function);
    ASSERT(selected.get_signature());
    ASSERT(selected.get_signature()->get_parameter_type(0));
    ASSERT(selected.get_signature()->get_result_type(0));
    EXPECT(&*selected.get_signature()->get_parameter_type(0) == &mode);
    EXPECT(&*selected.get_signature()->get_result_type(0) == &mode);
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, exact_root_collision_domain) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Enumeration test.\ndialect : Library;\n"
    "public Mode : enum[Unsigned_8] {}\n"
    "private Mode : enum[Signed_8] {}"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Packet : enum[Unsigned_8] {}\n"
    "public Packet : struct {}"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Packet : struct {}\n"
    "public Packet : enum[Unsigned_8] {}"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Unsigned_8 : enum[Unsigned_8] {}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, duplicate_name_rejected) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "public Mode : enum[Unsigned_8] { ready = 1; ready = 2; }"_view;
  EXPECT(rejects_interpretation(source));
}

PERIMORTEM_UNIT_TEST(EnumerationTests, malformed_atomic_grammar) {
  static constexpr Static::Vector<View::Bytes, 12> sources = {{
    "// Enumeration test.\ndialect : Library; public Mode enum[Unsigned_8] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : wrong[Unsigned_8] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum Unsigned_8] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8 {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8] ready = 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8] { public ready = 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8] { Ready = 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8] { ready 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8] { ready = true; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8] { ready = 1 + 2; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[Unsigned_8] { ready = 1;"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, repeat_lifecycle) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "public Mode : enum[Unsigned_8] { ready = 1; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(monograph->link());
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  ASSERT(monograph->finalize());
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Enumeration>());
  const auto& mode = static_cast<const Language::Types::Enumeration&>(
      bindings.get_data()[0].get());
  EXPECT(mode.is_linked());
  EXPECT(mode.is_finalized());
  EXPECT_EQ(mode.get_cases().get_size(), Count(1));
  EXPECT(monograph->get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, cursor_atomicity) {
  static constexpr View::Bytes malformed =
      "public Mode : enum[Unsigned_8] { ready = 1 }"_view;
  static constexpr View::Bytes complete =
      "private Mode : enum[Signed_8] { ready = -1; }"_view;
  EmptyContext context;
  Allocator::Arena arena;
  Dialect dialect;
  Language::Materializations materializations(arena);
  auto& monograph = Language::Monograph::create_authored(
      arena, Documentation::get_empty(), dialect, context, materializations);

  Errors malformed_errors;
  Tokenizer malformed_tokenizer(
      arena, malformed, "malformed-enumeration.ttx"_view);
  Cursor malformed_cursor(malformed_tokenizer, malformed_errors);
  Token opening = malformed_cursor.current();
  auto rejected = Language::Types::Enumeration::interpret(
      arena, malformed_cursor, Documentation::get_empty(), monograph);
  EXPECT_NOT(rejected);
  EXPECT_EQ(malformed_cursor.current().get_offset(), opening.get_offset());
  EXPECT(malformed_cursor.current().get_code() == opening.get_code());
  EXPECT_NOT(malformed_errors.is_empty());

  Errors complete_errors;
  Tokenizer complete_tokenizer(
      arena, complete, "complete-enumeration.ttx"_view);
  Cursor complete_cursor(complete_tokenizer, complete_errors);
  auto parsed = Language::Types::Enumeration::interpret(
      arena, complete_cursor, Documentation::get_empty(), monograph);
  ASSERT(parsed);
  EXPECT(complete_cursor.matches(Code::Type::Terminal));
  EXPECT(complete_errors.is_empty());
}
