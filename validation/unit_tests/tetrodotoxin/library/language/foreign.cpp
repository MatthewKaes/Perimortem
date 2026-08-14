// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/foreign/function.hpp"
#include "tetrodotoxin/library/language/foreign/state.hpp"
#include "tetrodotoxin/library/language/foreign/surface.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Validation;

using Tetrodotoxin::Language::Visibility;

static Harness ForeignTests = {
  .name = "Tetrodotoxin::Library::Language::Foreign"_view,
};

static auto interpret(
    Environment::Workspace& workspace,
    Errors& errors,
    View::Bytes source) -> Option<Library::Language::Monograph&> {
  if (!workspace.install_dialect<Library::Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "ForeignTest"_view, "foreign.ttx"_view, source);
  if (!interpreted || !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }
  return static_cast<Library::Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return Bool(
      !monograph && !errors.is_empty() &&
      &workspace.resolve_context("ForeignTest"_view) ==
          &Invalid::get_invalid());
}

static auto rejects_link(View::Bytes source) -> Bool {
  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return Bool(
      monograph && !workspace.link(errors) && !errors.is_empty() &&
      &workspace.resolve_context("ForeignTest"_view) ==
          &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(ForeignTests, source_identity_and_lifecycle) {
  static constexpr View::Bytes source =
      "// Foreign source identity.\n"
      "dialect : Library;\n"
      "foreign \"C\" {}\n"
      "foreign \"C\" {\n"
      "  // Shared State category.\n"
      "  public state shared : Unsigned_64;\n"
      "  expose state observed : Unsigned_64;\n"
      "  public state buffer : Fixed[Unsigned_8, 4];\n"
      "  // Shared Callable category.\n"
      "  public func shared[.value : Unsigned_64] -> Unsigned_64;\n"
      "  public func notify[] -> [];\n"
      "}\n"_view;

  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  auto& surface = monograph->get_source().get_foreign();
  ASSERT(surface.get_abi());
  EXPECT(*surface.get_abi() == "C"_view);
  EXPECT(&surface.resolve() == &surface);
  EXPECT(&surface.resolve_context("shared"_view) == &Invalid::get_invalid());

  ASSERT(workspace.link(errors));
  ASSERT(monograph->link());
  ASSERT(workspace.finalize(errors));
  ASSERT(monograph->finalize());

  EXPECT(
      &monograph->get_source().resolve_context("shared"_view) ==
      &Invalid::get_invalid());
  Count state_count = 0;
  for (const Reference<Abstract>& state : surface.get_states()) {
    EXPECT(state.get().is<Library::Language::Foreign::State>());
    state_count++;
  }
  Count function_count = 0;
  for (const Reference<Abstract>& function : surface.get_functions()) {
    EXPECT(function.get().is<Library::Language::Foreign::Function>());
    function_count++;
  }
  EXPECT_EQ(state_count, Count(3));
  EXPECT_EQ(function_count, Count(2));

  auto shared_state = surface.select_state("shared"_view);
  auto observed = surface.select_state("observed"_view);
  auto buffer = surface.select_state("buffer"_view);
  auto shared_function = surface.select_function("shared"_view);
  auto notify = surface.select_function("notify"_view);
  ASSERT(shared_state);
  ASSERT(observed);
  ASSERT(buffer);
  ASSERT(shared_function);
  ASSERT(notify);

  EXPECT(&shared_state->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(shared_state->get_visibility() == Visibility::Public);
  EXPECT(observed->get_visibility() == Visibility::Exposed);
  EXPECT(buffer->get_type().is<Library::Language::Types::Fixed>());
  EXPECT(buffer->get_type_reference().has_arguments());
  EXPECT(shared_state->get_abi() == "C"_view);
  ASSERT(shared_state->get_authorship());
  EXPECT(shared_state->get_authorship()->is_published());

  EXPECT(shared_function->get_abi() == "C"_view);
  EXPECT(shared_function->get_symbol() == "shared"_view);
  EXPECT_EQ(shared_function->get_parameters().get_size(), Count(1));
  EXPECT_EQ(shared_function->get_results().get_size(), Count(1));
  EXPECT_EQ(notify->get_parameters().get_size(), Count(0));
  EXPECT_EQ(notify->get_results().get_size(), Count(0));
  EXPECT_NOT(shared_function->is_type_bound());
  ASSERT(shared_function->get_authorship());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ForeignTests, access_invocation_and_category_separation) {
  static constexpr View::Bytes source =
      "// Foreign access integration.\n"
      "dialect : Library;\n"
      "foreign \"C\" {\n"
      "  expose state input : Unsigned_64;\n"
      "  public state output : Unsigned_64;\n"
      "  public state shared : Unsigned_64;\n"
      "  public func shared[.value : Unsigned_64] -> Unsigned_64;\n"
      "  public func no_result[] -> [];\n"
      "}\n"
      "public consume : func = [] -> Unsigned_64 {\n"
      "  foreign.output = foreign.input;\n"
      "  foreign -> no_result();\n"
      "  return foreign -> shared(foreign.output);\n"
      "}\n"_view;

  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ForeignTests, authored_rejections_are_atomic) {
  static constexpr Static::Vector<View::Bytes, 8> rejected = {{
    "// ABI.\ndialect : Library;\nforeign \"C++\" {}"_view,
    "// Const.\ndialect : Library;\nforeign \"C\" { public const value : "
    "Unsigned_64; }"_view,
    "// Private State.\ndialect : Library;\nforeign \"C\" { private state "
    "value : Unsigned_64; }"_view,
    "// Private Function.\ndialect : Library;\nforeign \"C\" { private func "
    "call[] -> []; }"_view,
    "// Exposed Function.\ndialect : Library;\nforeign \"C\" { expose func "
    "call[] -> []; }"_view,
    "// Receiver.\ndialect : Library;\nforeign \"C\" { public func call[self] "
    "-> []; }"_view,
    "// Body.\ndialect : Library;\nforeign \"C\" { public func call[] -> "
    "[] {} }"_view,
    "// Duplicate.\ndialect : Library;\nforeign \"C\" { public state value : "
    "Unsigned_64; public state value : Unsigned_64; }"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_interpretation(rejected[i]));
  }
}

PERIMORTEM_UNIT_TEST(ForeignTests, link_rejections_keep_source_unpublished) {
  static constexpr Static::Vector<View::Bytes, 8> rejected = {{
    "// Empty State.\ndialect : Library;\npublic Empty : struct {} foreign "
    "\"C\" { public state value : Empty; }"_view,
    "// Exposed write.\ndialect : Library;\nforeign \"C\" { expose state value "
    ": Unsigned_64; } public write : func = [] -> [] { foreign.value = 1; "
    "return; }"_view,
    "// Missing State.\ndialect : Library;\nforeign \"C\" {} public read : "
    "func "
    "= [] -> Unsigned_64 { return foreign.missing; }"_view,
    "// Ambient.\ndialect : Library;\nprivate foreign : Unsigned_64; foreign "
    "\"C\" "
    "{} public read : func = [] -> Unsigned_64 { return foreign.missing; }"_view,
    "// Missing Function.\ndialect : Library;\nforeign \"C\" {} public call : "
    "func = [] -> [] { foreign -> missing(); return; }"_view,
    "// Arguments.\ndialect : Library;\nforeign \"C\" { public func use[.value "
    ": Unsigned_64] -> []; } public call : func = [] -> [] { foreign -> "
    "use(false); return; }"_view,
    "// Dot mismatch.\ndialect : Library;\nforeign \"C\" { public func "
    "shared[] "
    "-> []; } public read : func = [] -> [] { return foreign.shared; }"_view,
    "// Arrow mismatch.\ndialect : Library;\nforeign \"C\" { public state "
    "shared : "
    "Unsigned_64; } public call : func = [] -> [] { foreign -> shared(); "
    "return; }"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_link(rejected[i]));
  }
}
