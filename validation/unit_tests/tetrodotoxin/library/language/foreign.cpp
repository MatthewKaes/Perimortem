// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/foreign.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
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
      !monograph && !errors.is_empty() &&
      &workspace.resolve_context("ForeignTest"_view) ==
          &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(ForeignTests, source_identity_and_lifecycle) {
  static constexpr View::Bytes source =
      "// Foreign source identity.\n"
      "dialect : Library;\n"
      "// Primary Foreign context.\n"
      "foreign \"C\" {\n"
      "  // Shared State category.\n"
      "  public state shared : Unsigned_64;\n"
      "  expose state observed : Unsigned_64;\n"
      "  public state buffer : Fixed[Unsigned_8, 4];\n"
      "  // Shared Callable category.\n"
      "  public func transform[.value : Unsigned_64] -> Unsigned_64;\n"
      "  public func notify[] -> [];\n"
      "}\n"
      "// Extended Foreign context.\n"
      "foreign \"C\" {\n"
      "  public state shared : Unsigned_64;\n"
      "}\n"_view;

  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const Library::Language::Foreign& foreign =
      monograph->get_source().get_foreign();
  ASSERT(foreign.get_abi());
  EXPECT(*foreign.get_abi() == "C"_view);
  EXPECT_TEXT(
      foreign.get_documentation().get_line(0), "Primary Foreign context."_view);
  EXPECT_TEXT(
      foreign.get_documentation().get_line(1),
      "Extended Foreign context."_view);
  EXPECT(&monograph->resolve_context("foreign"_view) == &foreign);
  EXPECT(&monograph->get_source().resolve_context("foreign"_view) == &foreign);
  EXPECT(monograph->resolve_context("shared"_view).is<Invalid>());

  const Abstract& shared_identity =
      foreign.resolve_access(foreign, "shared"_view);
  const Abstract& observed_identity =
      foreign.resolve_access(foreign, "observed"_view);
  const Abstract& buffer_identity =
      foreign.resolve_access(foreign, "buffer"_view);
  const Abstract& transform_identity =
      foreign.resolve_call(foreign, "transform"_view);
  const Abstract& notify_identity =
      foreign.resolve_call(foreign, "notify"_view);
  EXPECT_EQ(foreign.get_states().get_size(), Count(3));
  EXPECT_EQ(foreign.get_functions().get_size(), Count(2));

  ASSERT(shared_identity.is<Library::Language::Foreign::State>());
  ASSERT(observed_identity.is<Library::Language::Foreign::State>());
  ASSERT(buffer_identity.is<Library::Language::Foreign::State>());
  ASSERT(transform_identity.is<Library::Language::Foreign::Function>());
  ASSERT(notify_identity.is<Library::Language::Foreign::Function>());
  const auto& shared_state =
      static_cast<const Library::Language::Foreign::State&>(shared_identity);
  const auto& observed =
      static_cast<const Library::Language::Foreign::State&>(observed_identity);
  const auto& buffer =
      static_cast<const Library::Language::Foreign::State&>(buffer_identity);
  const auto& transform =
      static_cast<const Library::Language::Foreign::Function&>(
          transform_identity);
  const auto& notify =
      static_cast<const Library::Language::Foreign::Function&>(notify_identity);

  EXPECT(
      &shared_state.get_type() ==
      &monograph->resolve_context("Unsigned_64"_view));
  const auto& state_definition = shared_state.get_definition();
  EXPECT(state_definition.get_visibility() == Visibility::Public);
  EXPECT(observed.get_definition().get_visibility() == Visibility::Exposed);
  EXPECT(state_definition.is_authored());
  EXPECT(state_definition.is_complete());
  EXPECT(&state_definition.get_host() == &foreign);
  EXPECT_TEXT(
      state_definition.get_documentation().get_line(0),
      "Shared State category."_view);
  EXPECT_TEXT(
      state_definition.get_anchor().get_span().caculate_text(source),
      "public state shared : Unsigned_64;"_view);
  EXPECT(buffer.get_type().is<Library::Language::Types::Fixed>());
  EXPECT(buffer.get_type_reference().has_arguments());
  EXPECT(shared_state.get_abi() == "C"_view);

  EXPECT(transform.get_abi() == "C"_view);
  EXPECT(transform.get_symbol() == "transform"_view);
  const auto& function_definition = transform.get_definition();
  EXPECT(function_definition.get_visibility() == Visibility::Public);
  EXPECT(function_definition.is_authored());
  EXPECT(function_definition.is_complete());
  EXPECT(&function_definition.get_host() == &foreign);
  EXPECT_TEXT(
      function_definition.get_documentation().get_line(0),
      "Shared Callable category."_view);
  EXPECT_TEXT(
      function_definition.get_anchor().get_span().caculate_text(source),
      "public func transform[.value : Unsigned_64] -> Unsigned_64;"_view);
  EXPECT_EQ(transform.get_parameters().get_size(), Count(1));
  EXPECT_EQ(transform.get_results().get_size(), Count(1));
  EXPECT_EQ(notify.get_parameters().get_size(), Count(0));
  EXPECT_EQ(notify.get_results().get_size(), Count(0));
  EXPECT_NOT(transform.is_type_bound());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ForeignTests, access_invocation_and_category_separation) {
  static constexpr View::Bytes source =
      "// Foreign access integration.\n"
      "dialect : Library;\n"
      "public shared : Unsigned_64 = 7;\n"
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
  const Library::Language::Foreign& foreign =
      monograph->get_source().get_foreign();
  const Abstract& root_shared = monograph->resolve_context("shared"_view);
  const Abstract& state_shared = foreign.resolve_access(foreign, "shared"_view);
  const Abstract& function_shared =
      foreign.resolve_call(foreign, "shared"_view);
  EXPECT(!root_shared.is<Invalid>());
  EXPECT(state_shared.is<Library::Language::Foreign::State>());
  EXPECT(function_shared.is<Library::Language::Foreign::Function>());
  EXPECT(&root_shared != &state_shared);
  EXPECT(&state_shared != &function_shared);
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
    "// Conflict.\ndialect : Library;\nforeign \"C\" { public state value : "
    "Unsigned_64; public state value : Bool; }"_view,
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
