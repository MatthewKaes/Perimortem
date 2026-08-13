// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/range_loop.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/local.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/return.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness RangeLoopTests = {
  .name = "Tetrodotoxin::Library::Language::RangeLoop"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "RangeLoopTest"_view, "range_loop.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  for (const Reference<Abstract>& binding : composite.get_callables()) {
    if (binding.get().get_name() == name &&
        binding.get().is<Language::Function>()) {
      return static_cast<const Language::Function&>(binding.get());
    }
  }

  return {};
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return monograph && !workspace.link(errors) && !errors.is_empty();
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty() &&
         &workspace.resolve_context("RangeLoopTest"_view) ==
             &Invalid::get_invalid();
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, binding_is_the_lexical_addressable) {
  static constexpr View::Bytes source =
      "// Range loop graph.\n"
      "dialect : Library;\n"
      "public sum : func = [] -> Unsigned_64 {\n"
      "  state entry : Unsigned_64 = 7;\n"
      "  state total : Unsigned_64 = 0;\n"
      "  for [.entry : Unsigned_64] in 0...3 {\n"
      "    state copy : Unsigned_64 = entry;\n"
      "    total += copy;\n"
      "  }\n"
      "  return total;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto function = find_function(monograph->get_source(), "sum"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(4));
  ASSERT(statements.get_data()[0].get().is<Language::Local>());
  ASSERT(statements.get_data()[1].get().is<Language::Local>());
  ASSERT(statements.get_data()[2].get().is<Language::RangeLoop>());
  ASSERT(statements.get_data()[3].get().is<Language::Return>());

  const auto& loop =
      static_cast<const Language::RangeLoop&>(statements.get_data()[2].get());
  EXPECT_TEXT(loop.get_name(), "entry"_view);
  EXPECT(loop.get_type().is<Ttx::Model::Types::Unsigned>());
  const Abstract& range_type = loop.get_range().get_type().resolve();
  ASSERT(range_type.is<Language::Types::Range>());
  EXPECT(
      &static_cast<const Language::Types::Range&>(range_type)
           .get_element_type() == &loop.get_type());
  EXPECT(&loop.resolve_context("entry"_view) == &loop);
  EXPECT(&loop.get_body().resolve_context("entry"_view) == &loop);
  EXPECT(
      &function->get_body()->resolve_context("entry"_view) ==
      &statements.get_data()[0].get());
  EXPECT(
      &loop.get_body().resolve_context("total"_view) ==
      &statements.get_data()[1].get());
  EXPECT_TEXT(
      loop.get_anchor().get_span().caculate_text(source),
      "for [.entry : Unsigned_64] in 0...3 {\n"
      "    state copy : Unsigned_64 = entry;\n"
      "    total += copy;\n"
      "  }"_view);

  const Abstract& retained = statements.get_data()[2].get();
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  EXPECT(
      &function->get_body()->get_statements().get_data()[2].get() == &retained);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, binding_and_range_must_match) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Different integer Type.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : Signed_64] in 0...3 {} return; }"_view,
    "// Not a Range.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : Unsigned_64] in 3 {} return; }"_view,
    "// Multiple Range values.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : Unsigned_64] in (0...3, 4...6) {} return; }"_view,
    "// Empty binding Type.\ndialect : Library; private Empty : struct {} private invalid : func = [] -> [] { for [.entry : Empty] in 0...3 {} return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, binding_is_read_only_and_does_not_leak) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Immutable binding.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : Unsigned_64] in 0...3 { entry = 1; } return; }"_view,
    "// Leaked binding.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : Unsigned_64] in 0...3 {} state copy : Unsigned_64 = entry; return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, loop_does_not_cover_function_result) {
  static constexpr View::Bytes source =
      "// Range may be empty.\n"
      "dialect : Library;\n"
      "private invalid : func = [] -> Unsigned_64 {\n"
      "  for [.entry : Unsigned_64] in 0...0 { return entry; }\n"
      "}"_view;
  EXPECT(rejects_link(source));
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, malformed_binding_is_rejected) {
  static constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Bare binding.\ndialect : Library; private invalid : func = [] -> [] { for .entry : Unsigned_64 in 0...3 {} return; }"_view,
    "// Empty binding.\ndialect : Library; private invalid : func = [] -> [] { for [] in 0...3 {} return; }"_view,
    "// Positional binding.\ndialect : Library; private invalid : func = [] -> [] { for [Unsigned_64] in 0...3 {} return; }"_view,
    "// Multiple bindings.\ndialect : Library; private invalid : func = [] -> [] { for [.left : Unsigned_64, .right : Unsigned_64] in 0...3 {} return; }"_view,
    "// Missing body.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : Unsigned_64] in 0...3 return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}
