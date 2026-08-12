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
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
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

  const Abstract& selected = monograph->resolve_context("Bad"_view);
  if (!selected.is<Language::Types::Enumeration>()) {
    return False;
  }

  const auto& enumeration =
      static_cast<const Language::Types::Enumeration&>(selected);
  Bool finalized = workspace.finalize(errors);
  auto cases = enumeration.get_cases();
  return !finalized && cases.is_empty() && !errors.is_empty() &&
         &workspace.resolve_context("EnumerationTest"_view) ==
             &Invalid::get_invalid();
}

static Harness EnumerationTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Enumeration"_view,
};

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

  const Abstract& selected = monograph->resolve_context("Offset"_view);
  ASSERT(selected.is<Language::Types::Enumeration>());
  const auto& offset =
      static_cast<const Language::Types::Enumeration&>(selected);
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

  const Abstract& unsigned_identity =
      monograph->resolve_context("UnsignedEdge"_view);
  const Abstract& signed_identity =
      monograph->resolve_context("SignedEdge"_view);
  ASSERT(unsigned_identity.is<Language::Types::Enumeration>());
  ASSERT(signed_identity.is<Language::Types::Enumeration>());
  const auto& unsigned_edge =
      static_cast<const Language::Types::Enumeration&>(unsigned_identity);
  const auto& signed_edge =
      static_cast<const Language::Types::Enumeration&>(signed_identity);
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

  const auto& source_type = monograph->get_source();
  const Abstract& first = monograph->resolve_context("First"_view);
  const Abstract& second = monograph->resolve_context("Second"_view);
  ASSERT(first.is<Language::Types::Enumeration>());
  ASSERT(second.is<Language::Types::Enumeration>());
  auto types = source_type.get_types();
  ASSERT(types != types.end());
  EXPECT_TEXT((*types).get().get_name(), "Hidden"_view);
  ++types;
  ASSERT(types != types.end());
  EXPECT(&(*types).get() == &first);
  ++types;
  ASSERT(types != types.end());
  EXPECT(&(*types).get() == &second);
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
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
