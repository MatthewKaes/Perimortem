// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/workspace.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/archive/member.hpp"
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

class WorkspaceMonograph final : public Language::Monograph {
 public:
  TTX_CONTRACT(WorkspaceMonograph, Language::Monograph);

  WorkspaceMonograph(
      Allocator::Arena& arena,
      const Language::Dialect& dialect,
      const Documentation& documentation,
      Abstract& context,
      View::Bytes fact,
      Span fact_span,
      View::Bytes diagnostic_path)
      : Monograph(arena, dialect, documentation, context),
        fact(fact),
        fact_span(fact_span),
        diagnostic_path(diagnostic_path) {}

  auto link(Cursor& cursor) -> Bool override {
    if (fact == "link_fail"_view) {
      cursor.create_expression_error(
          fact_span, "Workspace test link failure."_view);
      return False;
    }
    return True;
  }

  auto finalize(Cursor& cursor) -> Bool override {
    if (fact == "finalize_fail"_view) {
      cursor.create_expression_error(
          fact_span, "Workspace test finalization failure."_view);
      return False;
    }
    return True;
  }

  auto link_restored() -> Bool override { return fact != "link_fail"_view; }

  auto finalize_restored() -> Bool override {
    return fact != "finalize_fail"_view;
  }

  auto get_name() const -> View::Bytes override { return "WorkspaceTest"_view; }

  constexpr auto get_fact() const -> View::Bytes { return fact; }

  constexpr auto get_diagnostic_path() const -> View::Bytes {
    return diagnostic_path;
  }

 private:
  View::Bytes fact;
  Span fact_span;
  View::Bytes diagnostic_path;
};

class WorkspaceDialect : public Language::Dialect {
 public:
  WorkspaceDialect(View::Bytes name) : Language::Dialect(name) {}

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

    if (fact == "reject"_view) {
      cursor.create_expression_error(
          Span(fact_token), "Workspace test interpretation failure."_view);
      return {};
    }

    Allocator::Arena& arena = cursor.get_arena();
    auto& monograph = arena.construct<WorkspaceMonograph>(
        arena, *this, documentation, context, fact, Span(fact_token),
        cursor.get_source_path());
    cursor.get_associations().create(
        Anchor::create(Span(fact_token)), monograph);
    return monograph;
  }

  auto restore(
      Allocator::Arena& arena,
      View::Bytes payload,
      Language::Persistence::Profile,
      const Documentation& documentation,
      Abstract& context) -> Option<Language::Monograph&> override {
    View::Bytes fact = arena.proxy(payload);
    auto& monograph = arena.construct<WorkspaceMonograph>(
        arena, *this, documentation, context, fact, Span(), View::Bytes());
    return monograph;
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
  Environment::Toolchain toolchain;
  Environment::Workspace workspace(toolchain);

  EXPECT_TEXT(workspace.get_name(), "Workspace"_view);
  EXPECT(&workspace.resolve() == &workspace);
  EXPECT(workspace.get_documentation().is_empty());
  EXPECT(&workspace.resolve_context("missing"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, unknown_dialect) {
  Environment::Toolchain toolchain;
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Unknown"_view, "unknown.ttx"_view,
      "// Unknown.\ndialect : Missing;\nvalue"_view);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  EXPECT(contains_diagnostic(errors, "Unknown dialect Missing"_view));
  EXPECT(&workspace.resolve_context("Unknown"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, publishes_sources) {
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<WorkspaceDialect>("Trace"_view));
  Environment::Workspace workspace(toolchain);

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
    Bool retained =
        workspace.resolve_context(semantic_name).is<WorkspaceMonograph>();
    EXPECT_EQ(retained, Bool(index != 0));
  }
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, retains_source) {
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<WorkspaceDialect>("Trace"_view));
  Environment::Workspace workspace(toolchain);
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

  auto associations = workspace.get_associations(*interpreted);
  ASSERT(associations);
  auto selected = associations->find_at(source_prefix.get_size());
  ASSERT(selected);
  EXPECT(&*selected == &*interpreted);

  Environment::Workspace unrelated(toolchain);
  EXPECT_NOT(unrelated.get_associations(*interpreted));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, keeps_first_source) {
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<WorkspaceDialect>("Trace"_view));
  Environment::Workspace workspace(toolchain);
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

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, imports_package) {
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Package::Dialect>("Package"_view));
  ASSERT(toolchain.install<Library::Dialect>("Library"_view));
  Environment::Workspace workspace(toolchain);
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

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, restores_package) {
  Package::Archive::Member member(
      "Main"_view, "Restored"_view, "restored"_view);
  Package::Archive::Archive archive(
      "Validation.Restored"_view, Version(1, 0), {}, View::Vector(&member, 1),
      {}, {});
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Package::Dialect>("Package"_view));
  ASSERT(toolchain.install<WorkspaceDialect>("Restored"_view));
  Environment::Workspace workspace(toolchain);

  auto restored = workspace.restore_package(archive, "RestoredPackage"_view);

  ASSERT(restored && restored->is<Package::Language::Monograph>());
  const auto& package =
      static_cast<const Package::Language::Monograph&>(*restored);
  const Abstract& selected = package.resolve_context("Main"_view).resolve();
  ASSERT(selected.is<WorkspaceMonograph>());
  EXPECT_TEXT(
      static_cast<const WorkspaceMonograph&>(selected).get_fact(),
      "restored"_view);
  EXPECT(&workspace.resolve_context("RestoredPackage"_view) == &package);

  Package::Archive::Member rejected_member(
      "Main"_view, "Restored"_view, "link_fail"_view);
  Package::Archive::Archive rejected_archive(
      "Validation.Rejected"_view, Version(1, 0), {},
      View::Vector(&rejected_member, 1), {}, {});
  Environment::Workspace rejected_workspace(toolchain);

  auto rejected = rejected_workspace.restore_package(
      rejected_archive, "RejectedPackage"_view);

  EXPECT_NOT(rejected);
  EXPECT(
      &rejected_workspace.resolve_context("RejectedPackage"_view) ==
      &Invalid::get_invalid());

  Package::Archive::Member unfinished_member(
      "Main"_view, "Restored"_view, "finalize_fail"_view);
  Package::Archive::Archive unfinished_archive(
      "Validation.Unfinished"_view, Version(1, 0), {},
      View::Vector(&unfinished_member, 1), {}, {});
  Environment::Workspace unfinished_workspace(toolchain);

  auto unfinished = unfinished_workspace.restore_package(
      unfinished_archive, "UnfinishedPackage"_view);

  EXPECT_NOT(unfinished);
  EXPECT(
      &unfinished_workspace.resolve_context("UnfinishedPackage"_view) ==
      &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, logs_import_errors) {
  Environment::Toolchain toolchain;
  Environment::Workspace workspace(toolchain);
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

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, rejects_manifest) {
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Package::Dialect>("Package"_view));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Manifest"_view, "package.ttx"_view,
      "// Package.\ndialect : Package;\nsource Main from \"main.ttx\";"_view);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  EXPECT(
      &workspace.resolve_context("Manifest"_view) == &Invalid::get_invalid());
}
