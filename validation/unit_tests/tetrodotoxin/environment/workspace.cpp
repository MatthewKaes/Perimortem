// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

struct WorkspaceTrace {
  View::Bytes interpreted[8]{};
  View::Bytes linked[8]{};
  View::Bytes finalized[8]{};
  Count interpretations = 0;
  Count links = 0;
  Count finalizations = 0;
};

static WorkspaceTrace* active_workspace_trace = nullptr;

class WorkspaceMonograph final : public Language::Monograph {
 public:
  TTX_CONTRACT(WorkspaceMonograph, Language::Monograph);

  WorkspaceMonograph(
      Allocator::Arena& arena,
      const Language::Dialect& dialect,
      const Documentation& documentation,
      Abstract& context,
      WorkspaceTrace& trace,
      View::Bytes fact,
      Span fact_span,
      View::Bytes diagnostic_path)
      : Monograph(arena, dialect, documentation, context),
        trace(trace),
        fact(fact),
        fact_span(fact_span),
        diagnostic_path(diagnostic_path) {}

  auto link(Cursor& cursor) -> Bool override {
    trace.linked[trace.links++] = fact;
    if (fact == "link_fail"_view) {
      cursor.create_expression_error(
          fact_span, "Workspace test link failure."_view);
      return False;
    }
    return True;
  }

  auto finalize(Cursor& cursor) -> Bool override {
    trace.finalized[trace.finalizations++] = fact;
    if (fact == "finalize_fail"_view) {
      cursor.create_expression_error(
          fact_span, "Workspace test finalization failure."_view);
      return False;
    }
    return True;
  }

  auto get_name() const -> View::Bytes override { return "WorkspaceTest"_view; }

  constexpr auto get_fact() const -> View::Bytes { return fact; }

  constexpr auto get_diagnostic_path() const -> View::Bytes {
    return diagnostic_path;
  }

 private:
  WorkspaceTrace& trace;
  View::Bytes fact;
  Span fact_span;
  View::Bytes diagnostic_path;
};

class WorkspaceDialect : public Language::Dialect {
 public:
  WorkspaceDialect(View::Bytes name)
      : Language::Dialect(name), trace(*active_workspace_trace) {}

  auto interpret(
      Cursor& cursor,
      const Documentation& documentation,
      const Anchor&,
      Abstract& context) -> Option<Language::Monograph&> override {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_error("Workspace test source requires one fact."_view);
      return {};
    }

    Token fact_token = cursor.current();
    View::Bytes fact = cursor.get_text();
    cursor.consume();
    if (!cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Workspace test source accepts exactly one fact."_view);
      return {};
    }

    trace.interpreted[trace.interpretations++] = fact;
    if (fact == "reject"_view) {
      cursor.create_expression_error(
          Span(fact_token), "Workspace test interpretation failure."_view);
      return {};
    }

    Allocator::Arena& arena = cursor.get_arena();
    auto& monograph = arena.construct<WorkspaceMonograph>(
        arena, *this, documentation, context, trace, fact, Span(fact_token),
        cursor.get_source_path());
    cursor.get_associations().create(
        Anchor::create(Span(fact_token)), monograph);
    return monograph;
  }

 private:
  WorkspaceTrace& trace;
};

class DependentWorkspaceDialect final : public WorkspaceDialect {
 public:
  DependentWorkspaceDialect(View::Bytes name, WorkspaceDialect& dependency)
      : WorkspaceDialect(name) {
    (void)dependency;
  }
};

static Harness EnvironmentWorkspace = {
  .name = "Tetrodotoxin::Environment::Workspace"_view,
  .setup = []() { Diagnostics::Log::set_sink(Test::capture_sink); },
  .teardown =
      []() { Diagnostics::Log::set_sink(Diagnostics::Log::default_sink); },
};

static constexpr View::Bytes source_prefix =
    "// Workspace source.\ndialect : Trace;\n"_view;

static auto make_source(View::Bytes fact) -> Dynamic::Bytes {
  Dynamic::Bytes source(source_prefix);
  source.concat(fact);
  return source;
}

static auto contains_diagnostic(const Errors& errors, View::Bytes fragment)
    -> Bool {
  Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(arena, index), fragment) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, abstract_contract) {
  Environment::Workspace workspace;

  EXPECT_TEXT(workspace.get_name(), "Workspace"_view);
  EXPECT(&workspace.resolve() == &workspace);
  EXPECT(workspace.get_documentation().is_empty());
  EXPECT(&workspace.resolve_context("missing"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, dialects_map_names_to_instances) {
  WorkspaceTrace trace;
  active_workspace_trace = &trace;
  Environment::Workspace workspace;
  auto* first = workspace.install_dialect<WorkspaceDialect>("First"_view);
  ASSERT(first);
  auto* second = workspace.install_dialect<WorkspaceDialect>("Second"_view);
  ASSERT(second);
  EXPECT(first != second);
  EXPECT_NOT(workspace.install_dialect<WorkspaceDialect>("First"_view));

  Allocator::Arena outside_arena;
  WorkspaceDialect outside("Outside"_view);
  EXPECT_NOT(workspace.install_dialect<DependentWorkspaceDialect>(
      "RejectedDependency"_view, outside));
  EXPECT(workspace.install_dialect<DependentWorkspaceDialect>(
      "AcceptedDependency"_view, *first));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, unknown_dialect_is_lexical_error) {
  Environment::Workspace workspace;
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Unknown"_view, "unknown.ttx"_view,
      "// Unknown.\ndialect : Missing;\nvalue"_view);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  EXPECT(contains_diagnostic(errors, "Unknown dialect Missing"_view));
  EXPECT(&workspace.resolve_context("Unknown"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, direct_source_is_atomic) {
  WorkspaceTrace trace;
  active_workspace_trace = &trace;
  Environment::Workspace workspace;
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Trace"_view));

  Errors accepted_errors;
  auto accepted_source = make_source("complete"_view);
  auto accepted = workspace.interpret_source(
      accepted_errors, "Accepted"_view, "accepted.ttx"_view, accepted_source);
  ASSERT(accepted && accepted->is<WorkspaceMonograph>());
  EXPECT(&workspace.resolve_context("Accepted"_view) == &*accepted);
  EXPECT(accepted_errors.is_empty());

  static constexpr View::Bytes failures[] = {
    "reject"_view,
    "link_fail"_view,
    "finalize_fail"_view,
  };
  for (Count index = 0; index < 3; index++) {
    Errors errors;
    Dynamic::Bytes semantic_name("Rejected"_view);
    semantic_name.append('0' + index);
    auto source = make_source(failures[index]);
    auto rejected = workspace.interpret_source(
        errors, semantic_name, "rejected.ttx"_view, source);
    EXPECT_NOT(rejected);
    EXPECT_NOT(errors.is_empty());
    EXPECT(
        &workspace.resolve_context(semantic_name) == &Invalid::get_invalid());
  }

  ASSERT_EQ(trace.interpretations, Count(4));
  ASSERT_EQ(trace.links, Count(3));
  ASSERT_EQ(trace.finalizations, Count(2));
  EXPECT_TEXT(trace.linked[0], "complete"_view);
  EXPECT_TEXT(trace.finalized[0], "complete"_view);
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, completed_source_retains_its_arena) {
  WorkspaceTrace trace;
  active_workspace_trace = &trace;
  Environment::Workspace workspace;
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Trace"_view));
  Dynamic::Bytes path("retained.ttx"_view);
  Dynamic::Bytes source = make_source("retained"_view);
  Errors errors;
  auto interpreted =
      workspace.interpret_source(errors, "Retained"_view, path, source);
  ASSERT(interpreted && interpreted->is<WorkspaceMonograph>());
  const auto& monograph = static_cast<const WorkspaceMonograph&>(*interpreted);

  path.set('x');
  source.set('x');
  EXPECT_TEXT(monograph.get_fact(), "retained"_view);
  EXPECT_TEXT(monograph.get_diagnostic_path(), "retained.ttx"_view);
  EXPECT_TEXT(
      monograph.get_documentation().get_line(0), "Workspace source."_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    EnvironmentWorkspace,
    completed_source_retains_its_associations) {
  WorkspaceTrace trace;
  active_workspace_trace = &trace;
  Errors errors;
  Environment::Workspace workspace;
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Trace"_view));
  auto source = make_source("retained_cursor"_view);
  auto interpreted = workspace.interpret_source(
      errors, "CursorSource"_view, "cursor-source.ttx"_view, source);
  ASSERT(interpreted);

  auto associations = workspace.get_associations(*interpreted);
  ASSERT(associations);
  auto selected = associations->find_at(source_prefix.get_size());
  ASSERT(selected);
  EXPECT(&*selected == &*interpreted);
  EXPECT(errors.is_empty());

  Environment::Workspace unrelated;
  EXPECT_NOT(unrelated.get_associations(*interpreted));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, duplicate_name_is_not_reopened) {
  WorkspaceTrace trace;
  active_workspace_trace = &trace;
  Environment::Workspace workspace;
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Trace"_view));
  Errors first_errors;
  auto source = make_source("first"_view);
  auto first = workspace.interpret_source(
      first_errors, "Same"_view, "first.ttx"_view, source);
  ASSERT(first);

  Errors second_errors;
  auto second_source = make_source("second"_view);
  auto second = workspace.interpret_source(
      second_errors, "Same"_view, "second.ttx"_view, second_source);
  EXPECT_NOT(second);
  EXPECT_NOT(second_errors.is_empty());
  EXPECT(&workspace.resolve_context("Same"_view) == &*first);
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, package_is_a_completed_table) {
  Environment::Workspace workspace;
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<Library::Dialect>("Library"_view));
  Errors errors;
  auto imported = workspace.import_package(
      errors, "validation/data/ttx/package_resources"_view, "Resources"_view,
      "package.ttx"_view, "Validation.Resources"_view, Version(1, 0));
  ASSERT(imported && imported->is<Package::Language::Monograph>());
  const auto& package =
      static_cast<const Package::Language::Monograph&>(*imported);

  ASSERT_EQ(package.get_sources().get_size(), Count(2));
  const Abstract& first = package.resolve_context("SharedA"_view).resolve();
  const Abstract& second = package.resolve_context("SharedB"_view).resolve();
  EXPECT(first.is<Library::Language::Monograph>());
  EXPECT(second.is<Library::Language::Monograph>());
  EXPECT(&workspace.resolve_context("Resources"_view) == &package);
  EXPECT(&workspace.resolve_context("SharedA"_view) == &Invalid::get_invalid());
  EXPECT(workspace.get_associations(package));
  EXPECT(workspace.get_associations(
      static_cast<const Language::Monograph&>(first)));
  EXPECT(workspace.get_associations(
      static_cast<const Language::Monograph&>(second)));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    EnvironmentWorkspace,
    source_free_package_failures_are_system_logs) {
  Environment::Workspace workspace;
  Errors errors;
  auto unopened = workspace.import_package(
      errors, "validation/data/ttx/package_resources/missing-root"_view,
      "Missing"_view, "package.ttx"_view, "Validation.Missing"_view,
      Version(1, 0));

  EXPECT_NOT(unopened);
  EXPECT(errors.is_empty());
  EXPECT(
      Test::error_contains(
          "Package import could not open its confined filesystem root"_view));

  auto unreadable = workspace.import_package(
      errors, "validation/data/ttx/package_resources"_view, "Missing"_view,
      "missing-package.ttx"_view, "Validation.Missing"_view, Version(1, 0));
  EXPECT_NOT(unreadable);
  EXPECT(errors.is_empty());
  EXPECT(
      Test::error_contains(
          "Package manifest `missing-package.ttx` could not be read"_view));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, package_manifest_is_not_a_source) {
  Environment::Workspace workspace;
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Manifest"_view, "package.ttx"_view,
      "// Package.\ndialect : Package;\nsource Main from \"main.ttx\";"_view);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  EXPECT(
      &workspace.resolve_context("Manifest"_view) == &Invalid::get_invalid());
}
