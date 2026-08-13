// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/assignment.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/block.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness AssignmentTests = {
  .name = "Tetrodotoxin::Library::Language::Assignment"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "AssignmentTest"_view, "assignment.ttx"_view, source);
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
         &workspace.resolve_context("AssignmentTest"_view) ==
             &Invalid::get_invalid();
}

PERIMORTEM_UNIT_TEST(AssignmentTests, retained_targets_and_order) {
  static constexpr View::Bytes source =
      "// Assignment graph.\n"
      "dialect : Library;\n"
      "public Data : struct {\n"
      "  public value : Unsigned_64 = 0;\n"
      "  expose state guarded : Unsigned_64 = 0;\n"
      "  public mutate : func = [self] -> [] {\n"
      "    self.guarded += 1;\n"
      "    return;\n"
      "  }\n"
      "}\n"
      "private data : Data;\n"
      "private state global : Unsigned_64 = 0;\n"
      "public run : func = [] -> [] {\n"
      "  state local : Unsigned_64 = 1;\n"
      "  state access : Access[Unsigned_64];\n"
      "  data.value = local;\n"
      "  global = data.value;\n"
      "  local += 3;\n"
      "  local -= 1;\n"
      "  access[0] = local;\n"
      "  return;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto run = find_function(monograph->get_source(), "run"_view);
  ASSERT(run && run->get_body());
  auto statements = run->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(8));

  static constexpr Static::Vector<Code::Type, 5> operations = {{
    Code::Type::Assign,
    Code::Type::Assign,
    Code::Type::AddAssign,
    Code::Type::SubAssign,
    Code::Type::Assign,
  }};
  for (Count i = 0; i < operations.get_size(); i++) {
    const Abstract& statement = statements.get_data()[i + 2].get();
    ASSERT(statement.is<Language::Assignment>());
    const auto& assignment =
        static_cast<const Language::Assignment&>(statement);
    EXPECT(assignment.get_operator() == operations[i]);
    EXPECT(assignment.get_target().resolve().is<Language::Expression>());
    EXPECT(assignment.get_source().resolve().is<Language::Model::Pack>());
  }

  const Abstract& first = statements.get_data()[2].get();
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  EXPECT(&run->get_body()->get_statements().get_data()[2].get() == &first);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AssignmentTests, complete_pack_fits_target) {
  static constexpr View::Bytes source =
      "// Assignment Pack.\n"
      "dialect : Library;\n"
      "public Pair : struct {\n"
      "  public number : Unsigned_64;\n"
      "  public flag : Bool;\n"
      "}\n"
      "public run : func = [] -> Pair {\n"
      "  state pair : Pair;\n"
      "  pair = (1, true);\n"
      "  return pair;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AssignmentTests, immutable_targets_are_rejected) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Const Local.\ndialect : Library; private invalid : func = [] -> [] { const value : Unsigned_64 = 1; value = 2; return; }"_view,
    "// Const Field.\ndialect : Library; private const value : Unsigned_64 = 1; private invalid : func = [] -> [] { value = 2; return; }"_view,
    "// Parameter target.\ndialect : Library; private invalid : func = [.value : Unsigned_64] -> [] { value = 2; return; }"_view,
    "// External state.\ndialect : Library; public Data : struct { expose state value : Unsigned_64 = 0; } private data : Data; private invalid : func = [] -> [] { data.value = 2; return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(AssignmentTests, incompatible_values_are_rejected) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Plain mismatch.\ndialect : Library; private invalid : func = [] -> [] { state value : Bool = false; value = 1; return; }"_view,
    "// Compound mismatch.\ndialect : Library; private invalid : func = [] -> [] { state value : Unsigned_64 = 0; value += -1; return; }"_view,
    "// Compound nonnumeric.\ndialect : Library; private invalid : func = [] -> [] { state value : Bool = false; value += true; return; }"_view,
    "// Pack mismatch.\ndialect : Library; private invalid : func = [] -> [] { state value : Unsigned_64 = 0; value = (1, 2); return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(AssignmentTests, malformed_targets_are_rejected) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Type target.\ndialect : Library; private invalid : func = [] -> [] { Bool = true; return; }"_view,
    "// Computed target.\ndialect : Library; private invalid : func = [] -> [] { 1 + 2 = 3; return; }"_view,
    "// Missing source.\ndialect : Library; private invalid : func = [] -> [] { state value : Unsigned_64 = 0; value = ; return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}
