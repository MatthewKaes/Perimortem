// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "validation/unit_test.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

struct WorkspaceTrace {
  const Language::Dialect* instances[4]{};
  View::Bytes expected_facts[4]{};
  View::Bytes expected_documentation[4]{};
  View::Bytes interpreted_facts[16]{};
  View::Bytes interpreted_paths[16]{};
  Count dialect_constructions = 0;
  Count monograph_constructions[4]{};
  Count dialect_destructions[4]{};
  Count monograph_destructions[4]{};
  Count interpretation_count = 0;
  Count post_passes = 0;
  Unsigned_8 destruction_phases[8]{};
  Count destruction_count = 0;
  Bool inspect_arena_state = false;
  Bool monograph_hosts_alive = true;
  Bool monograph_arena_state_valid = true;
  Bool dialect_arena_state_valid = true;
};

static WorkspaceTrace* active_trace = nullptr;

class WorkspaceDialect : public Language::Dialect {
 public:
  WorkspaceDialect(Abstract& registry)
      : Dialect(registry),
        trace(*active_trace),
        identity(trace.dialect_constructions) {
    trace.instances[identity] = this;
    trace.dialect_constructions++;
  }

  ~WorkspaceDialect() override {
    if (trace.inspect_arena_state &&
        !(retained_state == trace.expected_facts[identity])) {
      trace.dialect_arena_state_valid = false;
    }

    alive = false;
    trace.dialect_destructions[identity]++;
    trace.destruction_phases[trace.destruction_count] = 2;
    trace.destruction_count++;
  }

  auto interpret(
      Allocator::Arena& domain,
      Cursor& cursor,
      const Documentation& documentation,
      Abstract& registry) -> Option<Monograph&> override;

  auto get_identity() const -> Count { return identity; }
  auto get_alive() -> Bool& { return alive; }

 private:
  WorkspaceTrace& trace;
  Count identity;
  View::Bytes retained_state;
  Bool alive = true;
};

class WorkspaceMonograph : public Language::Dialect::Monograph {
 public:
  WorkspaceMonograph(
      Allocator::Arena& domain,
      const Documentation& documentation,
      WorkspaceDialect& host,
      WorkspaceTrace& trace,
      View::Bytes fact,
      View::Bytes diagnostic_path,
      Count host_identity)
      : Monograph(domain, documentation, host),
        trace(trace),
        fact(fact),
        diagnostic_path(diagnostic_path),
        host_alive(host.get_alive()),
        host_identity(host_identity) {}

  ~WorkspaceMonograph() override {
    if (!host_alive) {
      trace.monograph_hosts_alive = false;
    }

    if (trace.inspect_arena_state) {
      const Bool fact_valid = fact == trace.expected_facts[host_identity];
      const Bool documentation_valid =
          documentation.line_count() == 1 &&
          documentation.get_line(0) ==
              trace.expected_documentation[host_identity];
      if (!fact_valid || !documentation_valid) {
        trace.monograph_arena_state_valid = false;
      }
    }

    trace.monograph_destructions[host_identity]++;
    trace.destruction_phases[trace.destruction_count] = 1;
    trace.destruction_count++;
  }

  auto get_name() const -> View::Bytes override { return fact; }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

  auto post_pass(Errors&) -> void override { trace.post_passes++; }

  auto get_fact() const -> View::Bytes { return fact; }
  auto get_diagnostic_path() const -> View::Bytes { return diagnostic_path; }

 private:
  WorkspaceTrace& trace;
  View::Bytes fact;
  View::Bytes diagnostic_path;
  Bool& host_alive;
  Count host_identity;
};

auto WorkspaceDialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Abstract& registry) -> Option<Monograph&> {
  View::Bytes fact = cursor.get_text();
  View::Bytes diagnostic_path = cursor.get_source_path();
  trace.interpreted_facts[trace.interpretation_count] = fact;
  trace.interpreted_paths[trace.interpretation_count] = diagnostic_path;
  trace.interpretation_count++;

  if (fact == "reject"_view) {
    return {};
  }

  retained_state = fact;
  auto& monograph = domain.construct<WorkspaceMonograph>(
      domain, documentation, *this, trace, fact, diagnostic_path, identity);
  trace.monograph_constructions[identity]++;
  return monograph;
}

static auto expected_unknown_diagnostic(View::Bytes hint) -> Dynamic::Bytes {
  Dynamic::Bytes expected(
      "\x1b[38;2;227;62;60m"
      "\x1b[1m"
      "[ERROR] "
      "\x1b[38;2;222;122;101m"
      "\x1b[3m"
      "\x1b[38;2;255;102;102m"
      "unknown.ttx:2:1:\n"
      "\x1b[0m"
      "\x1b[38;2;222;122;101m"
      "\x1b[1m"
      "Unknown dialect Missing can't be used to interpret this source.\n"
      "\x1b[38;2;255;201;107m"
      "    2"
      "\x1b[38;2;245;147;85m"
      " | dialect : Missing;\n"
      "\x1b[38;2;245;147;85m"
      "      | "
      "\x1b[38;2;255;201;107m"
      "^-------\n"
      "\x1b[38;2;245;147;85m"
      "Note: "
      "\x1b[38;2;255;201;107m"_view);
  expected.concat(hint);
  expected.concat("\n\n\x1b[0m"_view);
  return expected;
}

static auto render_unknown(Environment::Workspace& workspace)
    -> Dynamic::Bytes {
  Dynamic::Bytes path("unknown.ttx"_view);
  Dynamic::Bytes contents("// Unknown\ndialect : Missing;\nignored"_view);
  Errors errors;
  const auto imported =
      workspace.import_source("Unknown"_view, path, contents, errors);

  path.set('x');
  contents.set('x');

  Allocator::Arena render_arena;
  View::Bytes rendered = errors.render_message(render_arena, 0);
  if (imported || errors.get_size() != 1 ||
      &workspace.resolve_context("Unknown"_view) != &Invalid::get_invalid()) {
    return Dynamic::Bytes("unexpected unknown import state"_view);
  }

  return Dynamic::Bytes(rendered);
}

static constexpr Count temporary_package_path_capacity = 160;

static auto join_package_path(View::Bytes base, View::Bytes member)
    -> Dynamic::Bytes {
  Dynamic::Bytes path(base);
  path.append('/');
  path.concat(member);
  return path;
}

static auto native_package_path(Dynamic::Bytes& path) -> char* {
  path.append('\0');
  return Data::cast<char>(path.get_access().get_data());
}

static auto remove_package_member(View::Bytes root, View::Bytes member)
    -> void {
  Dynamic::Bytes path = join_package_path(root, member);
  File::remove(path);
}

static auto cleanup_package_tree(View::Bytes root) -> void {
  if (root.is_empty()) {
    return;
  }

  remove_package_member(root, "package.ttx"_view);
  remove_package_member(root, "first.ttx"_view);
  remove_package_member(root, "second.ttx"_view);
  remove_package_member(root, "replacement.ttx"_view);
  remove_package_member(root, "unknown.ttx"_view);
  remove_package_member(root, "malformed.ttx"_view);
  remove_package_member(root, "rejected.ttx"_view);
  remove_package_member(root, "keep.ttx"_view);
  remove_package_member(root, "duplicate.ttx"_view);
  remove_package_member(root, "later.ttx"_view);
  remove_package_member(root, "after.ttx"_view);
  remove_package_member(root, "nested/package.ttx"_view);
  remove_package_member(root, "nested/deep.ttx"_view);
  remove_package_member(root, "nested/failures.ttx"_view);
  remove_package_member(root, "nested"_view);
  File::remove(root);
}

class TemporaryWorkspacePackage {
 public:
  TemporaryWorkspacePackage() {
    Signed_32 written = snprintf(
        Data::cast<char>(root_path.get_data()), root_path.get_size(),
        "/tmp/tetrodotoxin_workspace_package_XXXXXX");
    if (written <= 0 || Count(written) >= root_path.get_size()) {
      return;
    }

    char* created = mkdtemp(Data::cast<char>(root_path.get_data()));
    valid = created != nullptr;
  }

  TemporaryWorkspacePackage(const TemporaryWorkspacePackage&) = delete;
  auto operator=(const TemporaryWorkspacePackage&)
      -> TemporaryWorkspacePackage& = delete;

  ~TemporaryWorkspacePackage() {
    if (valid) {
      cleanup_package_tree(get_root());
    }
  }

  operator bool() const { return bool(valid); }

  auto get_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(root_path.get_data()));
  }

  auto write(View::Bytes member, View::Bytes contents) const -> Bool {
    Dynamic::Bytes path = join_package_path(get_root(), member);
    return File::write(contents, path);
  }

  auto remove(View::Bytes member) const -> Bool {
    Dynamic::Bytes path = join_package_path(get_root(), member);
    return File::remove(path);
  }

  auto create_directory(View::Bytes member) const -> Bool {
    Dynamic::Bytes path = join_package_path(get_root(), member);
    Signed_32 created = mkdir(native_package_path(path), S_IRWXU);
    return created == 0;
  }

  auto replace(View::Bytes replacement, View::Bytes target) const -> Bool {
    Dynamic::Bytes replacement_path =
        join_package_path(get_root(), replacement);
    Dynamic::Bytes target_path = join_package_path(get_root(), target);
    Signed_32 moved = rename(
        native_package_path(replacement_path),
        native_package_path(target_path));
    return moved == 0;
  }

 private:
  Static::Bytes<temporary_package_path_capacity> root_path;
  Bool valid = False;
};

static auto contains(View::Bytes text, View::Bytes fragment) -> Bool {
  if (fragment.is_empty()) {
    return True;
  }

  if (fragment.get_size() > text.get_size()) {
    return False;
  }

  for (Count start = 0; start + fragment.get_size() <= text.get_size();
       start++) {
    Bool matches = True;
    for (Count i = 0; i < fragment.get_size(); i++) {
      if (text[start + i] != fragment[i]) {
        matches = False;
        break;
      }
    }

    if (matches) {
      return True;
    }
  }

  return False;
}

static auto has_diagnostic(const Errors& errors, View::Bytes fragment) -> Bool {
  Allocator::Arena arena;
  for (Count i = 0; i < errors.get_size(); i++) {
    if (contains(errors.render_message(arena, i), fragment)) {
      return True;
    }
  }

  return False;
}

static Harness EnvironmentWorkspace = {
  .name = "Tetrodotoxin::Environment::Workspace"_view,
};

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, abstract_contract) {
  Environment::Workspace workspace;
  const Abstract& abstract = workspace;

  EXPECT_TEXT(abstract.get_name(), "Workspace"_view);
  EXPECT(abstract.get_documentation().is_empty());
  EXPECT(&abstract.resolve() == &abstract);
  EXPECT(&abstract.resolve_context("Missing"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, exact_dialect_installation) {
  WorkspaceTrace trace;
  active_trace = &trace;

  {
    Environment::Workspace workspace;
    Dynamic::Bytes first_name("Alpha"_view);

    EXPECT(workspace.install_dialect<WorkspaceDialect>(first_name));
    EXPECT_NOT(workspace.install_dialect<WorkspaceDialect>(first_name));
    EXPECT_EQ(trace.dialect_constructions, 1);
    EXPECT_EQ(trace.dialect_destructions[0], 0);
    first_name.set('x');

    {
      Dynamic::Bytes second_name("Beta"_view);
      EXPECT(workspace.install_dialect<WorkspaceDialect>(second_name));
    }

    EXPECT_EQ(trace.dialect_constructions, 2);
    EXPECT(trace.instances[0] != trace.instances[1]);
    EXPECT_EQ(
        static_cast<const WorkspaceDialect*>(trace.instances[0])
            ->get_identity(),
        0);
    EXPECT_EQ(
        static_cast<const WorkspaceDialect*>(trace.instances[1])
            ->get_identity(),
        1);
    EXPECT_TEXT(
        render_unknown(workspace),
        expected_unknown_diagnostic("Installed dialects: Alpha, Beta."_view));
  }

  EXPECT_EQ(trace.dialect_destructions[0], 1);
  EXPECT_EQ(trace.dialect_destructions[1], 1);
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, unknown_diagnostic_wording) {
  WorkspaceTrace trace;
  active_trace = &trace;

  {
    Environment::Workspace workspace;
    EXPECT_TEXT(
        render_unknown(workspace),
        expected_unknown_diagnostic("Installed dialects: <None>."_view));
  }

  {
    Environment::Workspace workspace;
    EXPECT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
    EXPECT_TEXT(
        render_unknown(workspace),
        expected_unknown_diagnostic("Installed dialects: Alpha."_view));
  }

  {
    Environment::Workspace workspace;
    EXPECT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
    EXPECT(workspace.install_dialect<WorkspaceDialect>("Beta"_view));
    EXPECT_TEXT(
        render_unknown(workspace),
        expected_unknown_diagnostic("Installed dialects: Alpha, Beta."_view));
  }

  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, owned_direct_import) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  EXPECT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));

  Dynamic::Bytes semantic_name("Main"_view);
  Dynamic::Bytes diagnostic_path("sources/main.ttx"_view);
  Dynamic::Bytes contents("// Main documentation\ndialect : Alpha;\nFact"_view);
  Errors errors;
  auto imported_result =
      workspace.import_source(semantic_name, diagnostic_path, contents, errors);
  ASSERT(imported_result);

  semantic_name.set('x');
  diagnostic_path.set('x');
  contents.set('x');

  const Abstract& imported = workspace.resolve_context("Main"_view);
  const auto& monograph = static_cast<const WorkspaceMonograph&>(imported);
  EXPECT(&*imported_result == &imported);
  EXPECT(&imported != &Invalid::get_invalid());
  EXPECT_TEXT(imported.get_name(), "Fact"_view);
  EXPECT_EQ(imported.get_documentation().line_count(), 1);
  EXPECT_TEXT(
      imported.get_documentation().get_line(0), "Main documentation"_view);
  EXPECT_TEXT(monograph.get_fact(), "Fact"_view);
  EXPECT(
      &workspace.resolve_context("sources/main.ttx"_view) ==
      &Invalid::get_invalid());
  EXPECT(errors.is_empty());

  Errors shared_path_errors;
  EXPECT(workspace.import_source(
      "Other"_view, "sources/main.ttx"_view,
      "// Other documentation\ndialect : Alpha;\nOther"_view,
      shared_path_errors));
  EXPECT_TEXT(workspace.resolve_context("Other"_view).get_name(), "Other"_view);
  EXPECT(shared_path_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, failed_import_nonpublication) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  EXPECT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));

  Errors parse_errors;
  EXPECT_NOT(workspace.import_source(
      "Malformed"_view, "malformed.ttx"_view, "dialect : Alpha;\nBad"_view,
      parse_errors));
  EXPECT(
      &workspace.resolve_context("Malformed"_view) == &Invalid::get_invalid());

  Errors rejected_errors;
  EXPECT_NOT(workspace.import_source(
      "Retry"_view, "retry.ttx"_view,
      "// Rejected\ndialect : Alpha;\nreject"_view, rejected_errors));
  EXPECT(&workspace.resolve_context("Retry"_view) == &Invalid::get_invalid());

  Errors retry_errors;
  EXPECT(workspace.import_source(
      "Retry"_view, "retry.ttx"_view,
      "// Accepted\ndialect : Alpha;\nAccepted"_view, retry_errors));
  const Abstract& retained = workspace.resolve_context("Retry"_view);
  const Count constructed = trace.monograph_constructions[0];

  Errors duplicate_errors;
  EXPECT_NOT(workspace.import_source(
      "Retry"_view, "other.ttx"_view, "// Other\ndialect : Alpha;\nOther"_view,
      duplicate_errors));
  EXPECT(&workspace.resolve_context("Retry"_view) == &retained);
  EXPECT_TEXT(retained.get_name(), "Accepted"_view);
  EXPECT_EQ(trace.monograph_constructions[0], constructed);
  EXPECT_EQ(duplicate_errors.get_size(), 1);
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, staged_fifo_retention) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  Errors errors;
  {
    Errors::Report report(errors, "prior-workspace.ttx"_view, View::Bytes());
    report << "Earlier independent diagnostic."_view;
  }

  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));

  {
    TemporaryWorkspacePackage package;
    ASSERT(package);
    ASSERT(package.create_directory("nested"_view));
    ASSERT(package.write(
        "package.ttx"_view,
        "// Root Package\n"
        "dialect : Package;\n"
        "source First from \"first.ttx\";\n"
        "source Nested from \"nested/./package.ttx\";\n"
        "source Group::Second from \"./second.ttx\";\n"_view));
    ASSERT(package.write(
        "nested/package.ttx"_view,
        "// Nested Package\n"
        "dialect : Package;\n"
        "source Deep from \"nested/deep.ttx\";\n"_view));
    ASSERT(package.write(
        "first.ttx"_view,
        "// First documentation\n"
        "dialect : Alpha;\n"
        "First"_view));
    ASSERT(package.write(
        "second.ttx"_view,
        "// Second documentation\n"
        "dialect : Alpha;\n"
        "Second"_view));
    ASSERT(package.write(
        "nested/deep.ttx"_view,
        "// Deep documentation\n"
        "dialect : Alpha;\n"
        "Deep"_view));

    Dynamic::Bytes package_root(package.get_root());
    Dynamic::Bytes root_name("Root"_view);
    Dynamic::Bytes root_route("./package.ttx"_view);
    auto imported_root =
        workspace.import_package(package_root, root_name, root_route, errors);
    ASSERT(imported_root);
    EXPECT((*imported_root).is<Package::Language::Monograph>());
    EXPECT(&*imported_root == &workspace.resolve_context("Root"_view));
    EXPECT_EQ(errors.get_size(), 1);

    package_root.set('x');
    root_name.set('x');
    root_route.set('x');

    ASSERT(package.write(
        "replacement.ttx"_view,
        "// Replacement\n"
        "dialect : Alpha;\n"
        "Replacement"_view));
    EXPECT(package.replace("replacement.ttx"_view, "first.ttx"_view));
    EXPECT(package.remove("second.ttx"_view));
  }

  ASSERT_EQ(trace.interpretation_count, 3);
  EXPECT_TEXT(trace.interpreted_facts[0], "First"_view);
  EXPECT_TEXT(trace.interpreted_facts[1], "Second"_view);
  EXPECT_TEXT(trace.interpreted_facts[2], "Deep"_view);
  EXPECT_TEXT(trace.interpreted_paths[0], "first.ttx"_view);
  EXPECT_TEXT(trace.interpreted_paths[1], "second.ttx"_view);
  EXPECT_TEXT(trace.interpreted_paths[2], "nested/deep.ttx"_view);

  const Abstract& root = workspace.resolve_context("Root"_view);
  ASSERT(root.is<Package::Language::Monograph>());
  const auto& root_package =
      static_cast<const Package::Language::Monograph&>(root);
  EXPECT_EQ(root_package.get_sources().get_size(), 3);

  const Abstract& first = workspace.resolve_context("First"_view);
  const Abstract& second = workspace.resolve_context("Group::Second"_view);
  const Abstract& deep = workspace.resolve_context("Deep"_view);
  ASSERT(&first != &Invalid::get_invalid());
  ASSERT(&second != &Invalid::get_invalid());
  ASSERT(&deep != &Invalid::get_invalid());

  const auto& first_monograph = static_cast<const WorkspaceMonograph&>(first);
  const auto& second_monograph = static_cast<const WorkspaceMonograph&>(second);
  const auto& deep_monograph = static_cast<const WorkspaceMonograph&>(deep);
  EXPECT_TEXT(first_monograph.get_fact(), "First"_view);
  EXPECT_TEXT(second_monograph.get_fact(), "Second"_view);
  EXPECT_TEXT(deep_monograph.get_fact(), "Deep"_view);
  EXPECT_TEXT(first_monograph.get_diagnostic_path(), "first.ttx"_view);
  EXPECT_TEXT(second_monograph.get_diagnostic_path(), "second.ttx"_view);
  EXPECT_TEXT(deep_monograph.get_diagnostic_path(), "nested/deep.ttx"_view);

  EXPECT(&workspace.resolve_context("Second"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("group::Second"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("package.ttx"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("first.ttx"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("nested/deep.ttx"_view) ==
      &Invalid::get_invalid());
  EXPECT_EQ(trace.post_passes, 0);
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, staged_failures) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  Errors errors;

  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));

  {
    TemporaryWorkspacePackage package;
    ASSERT(package);
    ASSERT(package.create_directory("nested"_view));
    ASSERT(package.write(
        "package.ttx"_view,
        "// Failure Package\n"
        "dialect : Package;\n"
        "source Keep from \"keep.ttx\";\n"
        "source Nested from \"nested/failures.ttx\";\n"
        "source Unknown from \"unknown.ttx\";\n"
        "source Malformed from \"malformed.ttx\";\n"
        "source Rejected from \"rejected.ttx\";\n"
        "source Missing from \"missing.ttx\";\n"
        "source Later from \"later.ttx\";\n"_view));
    ASSERT(package.write(
        "nested/failures.ttx"_view,
        "// Nested Failures\n"
        "dialect : Package;\n"
        "source Keep from \"duplicate.ttx\";\n"
        "source After from \"after.ttx\";\n"_view));
    ASSERT(
        package.write("keep.ttx"_view, "// Keep\ndialect : Alpha;\nKeep"_view));
    ASSERT(package.write(
        "unknown.ttx"_view, "// Unknown\ndialect : Missing;\nUnknown"_view));
    ASSERT(package.write(
        "malformed.ttx"_view, "dialect : Alpha;\nMalformed"_view));
    ASSERT(package.write(
        "rejected.ttx"_view, "// Rejected\ndialect : Alpha;\nreject"_view));
    ASSERT(package.write(
        "later.ttx"_view, "// Later\ndialect : Alpha;\nLater"_view));
    ASSERT(package.write(
        "duplicate.ttx"_view,
        "// Duplicate\n"
        "dialect : Alpha;\n"
        "Duplicate"_view));
    ASSERT(package.write(
        "after.ttx"_view, "// After\ndialect : Alpha;\nAfter"_view));

    EXPECT_NOT(workspace.import_package(
        package.get_root(), "Root"_view, "package.ttx"_view, errors));
  }

  ASSERT_EQ(trace.interpretation_count, 4);
  EXPECT_TEXT(trace.interpreted_facts[0], "Keep"_view);
  EXPECT_TEXT(trace.interpreted_facts[1], "reject"_view);
  EXPECT_TEXT(trace.interpreted_facts[2], "Later"_view);
  EXPECT_TEXT(trace.interpreted_facts[3], "After"_view);

  EXPECT(workspace.resolve_context("Root"_view)
             .is<Package::Language::Monograph>());
  EXPECT(workspace.resolve_context("Nested"_view)
             .is<Package::Language::Monograph>());
  EXPECT_TEXT(workspace.resolve_context("Keep"_view).get_name(), "Keep"_view);
  EXPECT_TEXT(workspace.resolve_context("Later"_view).get_name(), "Later"_view);
  EXPECT_TEXT(workspace.resolve_context("After"_view).get_name(), "After"_view);

  EXPECT(&workspace.resolve_context("Unknown"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("Malformed"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("Rejected"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Missing"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("duplicate.ttx"_view) ==
      &Invalid::get_invalid());

  EXPECT_EQ(errors.get_size(), 4);
  EXPECT(has_diagnostic(errors, "Unknown dialect Missing"_view));
  EXPECT(has_diagnostic(
      errors, "Source is missing required documentation comment."_view));
  EXPECT(has_diagnostic(
      errors,
      "Environment::Workspace Package import could not read semantic source "
      "Missing from logical route missing.ttx."_view));
  EXPECT(has_diagnostic(
      errors,
      "Semantic source Keep is already imported into the Workspace."_view));
  EXPECT(has_diagnostic(errors, "missing.ttx"_view));
  EXPECT_EQ(trace.post_passes, 0);
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, destruction_phases) {
  WorkspaceTrace trace;
  trace.inspect_arena_state = true;
  trace.expected_facts[0] = "First"_view;
  trace.expected_facts[1] = "Second"_view;
  trace.expected_documentation[0] = "First documentation"_view;
  trace.expected_documentation[1] = "Second documentation"_view;
  active_trace = &trace;

  {
    Environment::Workspace workspace;
    Errors errors;
    EXPECT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
    EXPECT(workspace.install_dialect<WorkspaceDialect>("Beta"_view));
    EXPECT(workspace.import_source(
        "First"_view, "first.ttx"_view,
        "// First documentation\ndialect : Alpha;\nFirst"_view, errors));
    EXPECT(workspace.import_source(
        "Second"_view, "second.ttx"_view,
        "// Second documentation\ndialect : Beta;\nSecond"_view, errors));
  }

  EXPECT_EQ(trace.monograph_constructions[0], 1);
  EXPECT_EQ(trace.monograph_constructions[1], 1);
  EXPECT_EQ(trace.monograph_destructions[0], 1);
  EXPECT_EQ(trace.monograph_destructions[1], 1);
  EXPECT_EQ(trace.dialect_destructions[0], 1);
  EXPECT_EQ(trace.dialect_destructions[1], 1);
  EXPECT_EQ(trace.destruction_count, 4);
  EXPECT_EQ(trace.destruction_phases[0], 1);
  EXPECT_EQ(trace.destruction_phases[1], 1);
  EXPECT_EQ(trace.destruction_phases[2], 2);
  EXPECT_EQ(trace.destruction_phases[3], 2);
  EXPECT(trace.monograph_hosts_alive);
  EXPECT(trace.monograph_arena_state_valid);
  EXPECT(trace.dialect_arena_state_valid);
  active_trace = nullptr;
}
