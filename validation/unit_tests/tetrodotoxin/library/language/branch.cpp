// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/branch.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/local.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/return.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness BranchTests = {
  .name = "Tetrodotoxin::Library::Language::Branch"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "BranchTest"_view, "branch.ttx"_view, source);
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
         &workspace.resolve_context("BranchTest"_view) ==
             &Invalid::get_invalid();
}

PERIMORTEM_UNIT_TEST(BranchTests, retained_blocks_and_condition_pack) {
  static constexpr View::Bytes source =
      "// Branch graph.\n"
      "dialect : Library;\n"
      "public run : func = [] -> Unsigned_64 {\n"
      "  state outer : Unsigned_64 = 1;\n"
      "  if (true, 9) {\n"
      "    state outer : Unsigned_64 = 3;\n"
      "    outer += 1;\n"
      "  } else {\n"
      "    outer = 2;\n"
      "  }\n"
      "  while false {\n"
      "    outer += 1;\n"
      "  }\n"
      "  return outer;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto function = find_function(monograph->get_source(), "run"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(4));
  ASSERT(statements.get_data()[0].get().is<Language::Local>());
  ASSERT(statements.get_data()[1].get().is<Language::Branch>());
  ASSERT(statements.get_data()[2].get().is<Language::Branch>());
  ASSERT(statements.get_data()[3].get().is<Language::Return>());

  const auto& conditional =
      static_cast<const Language::Branch&>(statements.get_data()[1].get());
  const auto& loop =
      static_cast<const Language::Branch&>(statements.get_data()[2].get());
  EXPECT(conditional.get_kind() == Language::Branch::Kind::If);
  EXPECT(loop.get_kind() == Language::Branch::Kind::While);
  EXPECT_EQ(conditional.get_condition().get_layout().get_size(), Count(2));
  ASSERT(conditional.get_alternate());
  EXPECT_TEXT(
      conditional.get_anchor().get_span().caculate_text(source),
      "if (true, 9) {\n"
      "    state outer : Unsigned_64 = 3;\n"
      "    outer += 1;\n"
      "  } else {\n"
      "    outer = 2;\n"
      "  }"_view);

  const Abstract& outer = statements.get_data()[0].get();
  const Abstract& shadowed =
      conditional.get_body().resolve_context("outer"_view);
  EXPECT(&shadowed != &outer);
  EXPECT(shadowed.is<Language::Local>());
  EXPECT(&conditional.get_alternate()->resolve_context("outer"_view) == &outer);

  const Abstract& retained = statements.get_data()[1].get();
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  EXPECT(
      &function->get_body()->get_statements().get_data()[1].get() == &retained);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BranchTests, terminal_if_covers_function_result) {
  static constexpr View::Bytes source =
      "// Terminal branch.\n"
      "dialect : Library;\n"
      "public select : func = [.flag : Bool] -> Unsigned_64 {\n"
      "  if flag {\n"
      "    return 1;\n"
      "  } else {\n"
      "    return 2;\n"
      "  }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto function = find_function(monograph->get_source(), "select"_view);
  ASSERT(function && function->get_body());
  EXPECT_NOT(function->get_body()->reaches_next_statement());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BranchTests, incomplete_result_paths_are_rejected) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Missing alternate.\ndialect : Library; private invalid : func = [.flag : Bool] -> Unsigned_64 { if flag { return 1; } }"_view,
    "// Falling alternate.\ndialect : Library; private invalid : func = [.flag : Bool] -> Unsigned_64 { if flag { return 1; } else {} }"_view,
    "// While may not execute.\ndialect : Library; private invalid : func = [.flag : Bool] -> Unsigned_64 { while flag { return 1; } }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(BranchTests, first_condition_value_must_be_flag) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Empty condition.\ndialect : Library; private invalid : func = [] -> [] { if () {} return; }"_view,
    "// Numeric condition.\ndialect : Library; private invalid : func = [] -> [] { if 1 {} return; }"_view,
    "// Later Flag.\ndialect : Library; private invalid : func = [] -> [] { while (1, true) {} return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(BranchTests, malformed_branch_is_rejected) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Missing body.\ndialect : Library; private invalid : func = [] -> [] { if true return; }"_view,
    "// Missing alternate body.\ndialect : Library; private invalid : func = [] -> [] { if true {} else return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}
