// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "validation/unit_test.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/archive/member.hpp"
#include "tetrodotoxin/package/archive/writer.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/repository/input.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/span.hpp"

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
  const Abstract* installed_registries[4]{};
  const Abstract* interpretation_contexts[16]{};
  const Abstract* package_registry = nullptr;
  const Abstract* package_interpretation_contexts[4]{};
  const Abstract* resource_results[8]{};
  const Library::Language::Constant* literal_results[2]{};
  const Package::Resources* resources_owner = nullptr;
  View::Bytes expected_facts[4]{};
  View::Bytes expected_documentation[4]{};
  View::Bytes interpreted_facts[16]{};
  View::Bytes interpreted_paths[16]{};
  View::Bytes restored_facts[16]{};
  View::Bytes link_facts[32]{};
  View::Bytes finalize_facts[32]{};
  Count dialect_constructions = 0;
  Count monograph_constructions[4]{};
  Count dialect_destructions[4]{};
  Count monograph_destructions[4]{};
  Count interpretation_count = 0;
  Count restoration_count = 0;
  Count links = 0;
  Count finalizers = 0;
  Count package_interpretation_count = 0;
  Unsigned_8 destruction_phases[8]{};
  Count destruction_count = 0;
  Bool inspect_arena_state = false;
  Bool monograph_hosts_alive = true;
  Bool monograph_arena_state_valid = true;
  Bool dialect_arena_state_valid = true;
  Bool resources_sealed_during_link = false;
  Bool resource_link_seen = false;
};

static WorkspaceTrace* active_trace = nullptr;

class WorkspaceDialect : public Language::Dialect {
 public:
  WorkspaceDialect(Abstract& registry)
      : Dialect(registry),
        trace(*active_trace),
        identity(trace.dialect_constructions) {
    trace.instances[identity] = this;
    trace.installed_registries[identity] = &registry;
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
      Abstract& interpretation_context)
      -> Option<Language::Monograph&> override;

  auto restore(Allocator::Arena& domain, View::Bytes payload)
      -> Option<Language::Monograph&> override;

  auto get_identity() const -> Count { return identity; }
  auto get_alive() -> Bool& { return alive; }

 private:
  WorkspaceTrace& trace;
  Count identity;
  View::Bytes retained_state;
  Bool alive = true;
};

class TracedPackageDialect : public Package::Dialect {
 public:
  TracedPackageDialect(Abstract& registry)
      : Package::Dialect(registry), trace(*active_trace) {
    trace.package_registry = &registry;
  }

  auto interpret(
      Allocator::Arena& domain,
      Cursor& cursor,
      const Documentation& documentation,
      Abstract& interpretation_context)
      -> Option<Language::Monograph&> override {
    trace.package_interpretation_contexts[trace.package_interpretation_count] =
        &interpretation_context;
    trace.package_interpretation_count++;
    return Package::Dialect::interpret(
        domain, cursor, documentation, interpretation_context);
  }

 private:
  WorkspaceTrace& trace;
};

class ResourceMonograph : public Language::Monograph {
 public:
  ResourceMonograph(
      Allocator::Arena& domain,
      const Documentation& documentation,
      WorkspaceTrace& trace,
      const Package::Language::Monograph& package)
      : Monograph(domain, documentation), trace(trace), package(package) {}

  auto get_name() const -> View::Bytes override {
    return "ResourceConsumer"_view;
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

  auto link() -> Bool override {
    const Abstract& uncached =
        package.resolve_context("$[resources/later.bin]"_view);
    trace.resources_sealed_during_link = &uncached == &Invalid::get_invalid();
    trace.resource_link_seen = true;
    return True;
  }

 private:
  WorkspaceTrace& trace;
  const Package::Language::Monograph& package;
};

class ResourceDialect : public Language::Dialect {
 public:
  ResourceDialect(Abstract& registry)
      : Dialect(registry), trace(*active_trace) {}

  auto interpret(
      Allocator::Arena& domain,
      Cursor& cursor,
      const Documentation& documentation,
      Abstract& interpretation_context)
      -> Option<Language::Monograph&> override {
    if (!interpretation_context.is<Package::Language::Monograph>()) {
      return {};
    }

    auto& package =
        static_cast<Package::Language::Monograph&>(interpretation_context);
    Package::Resources& resources = package.get_resources();

    // Literal parsing asks Package directly for Resource bytes. No installed
    // Library Dialect is needed just to recover its binary wide scalar Types.
    Library::Language::Materializations materializations(domain);
    auto table = Library::Language::Parser::Literal::parse(
        domain, materializations, cursor, package);
    auto empty = Library::Language::Parser::Literal::parse(
        domain, materializations, cursor, package);
    if (!table || !empty || !cursor.matches(Code::Type::Terminal)) {
      return {};
    }

    trace.literal_results[0] = &*table;
    trace.literal_results[1] = &*empty;
    trace.resources_owner = &resources;
    trace.resource_results[0] =
        &package.resolve_context("$[resources/cache/../table.bin]"_view);
    trace.resource_results[1] =
        &package.resolve_context("$[resources/./table.bin]"_view);
    trace.resource_results[2] =
        &package.resolve_context("$[resources/empty.bin]"_view);
    trace.resource_results[3] =
        &package.resolve_context("$[resources/cache/../missing.bin]"_view);
    trace.resource_results[4] =
        &package.resolve_context("$[resources/missing.bin]"_view);
    trace.resource_results[5] = &package.resolve_context("$[]"_view);
    trace.resource_results[6] =
        &package.resolve_context("$[../outside.bin]"_view);
    trace.resource_results[7] = &package.resolve_context("Member"_view);

    return domain.construct<ResourceMonograph>(
        domain, documentation, trace, package);
  }

 private:
  WorkspaceTrace& trace;
};

class WorkspaceMonograph : public Language::Monograph {
 public:
  WorkspaceMonograph(
      Allocator::Arena& domain,
      const Documentation& documentation,
      WorkspaceDialect& host,
      WorkspaceTrace& trace,
      View::Bytes fact,
      View::Bytes diagnostic_path,
      Span fact_span,
      Count host_identity)
      : Monograph(domain, documentation),
        trace(trace),
        fact(fact),
        diagnostic_path(diagnostic_path),
        fact_span(fact_span),
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

  auto link() -> Bool override {
    trace.link_facts[trace.links] = fact;
    trace.links++;
    if (fact == "link_fail"_view) {
      report(
          Anchor::create(fact_span), "Link rejected the retained fact."_view,
          "Use a linkable fact."_view);
      return False;
    }

    return True;
  }

  auto finalize() -> Bool override {
    trace.finalize_facts[trace.finalizers] = fact;
    trace.finalizers++;
    if (fact == "finalize_fail"_view) {
      report(
          Anchor::create(fact_span),
          "Finalize rejected the retained fact."_view,
          "Use a finalizable fact."_view);
      return False;
    }

    return True;
  }

  auto get_fact() const -> View::Bytes { return fact; }
  auto get_diagnostic_path() const -> View::Bytes { return diagnostic_path; }

 private:
  WorkspaceTrace& trace;
  View::Bytes fact;
  View::Bytes diagnostic_path;
  Span fact_span;
  Bool& host_alive;
  Count host_identity;
};

auto WorkspaceDialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Abstract& interpretation_context) -> Option<Language::Monograph&> {
  View::Bytes fact = cursor.get_text();
  Token fact_token = cursor.current();
  cursor.consume();
  View::Bytes diagnostic_path = cursor.get_source_path();
  trace.interpretation_contexts[trace.interpretation_count] =
      &interpretation_context;
  trace.interpreted_facts[trace.interpretation_count] = fact;
  trace.interpreted_paths[trace.interpretation_count] = diagnostic_path;
  trace.interpretation_count++;

  if (fact == "reject"_view) {
    return {};
  }

  retained_state = fact;
  auto& monograph = domain.construct<WorkspaceMonograph>(
      domain, documentation, *this, trace, fact, diagnostic_path,
      Span(fact_token), identity);
  trace.monograph_constructions[identity]++;
  return monograph;
}

auto WorkspaceDialect::restore(Allocator::Arena& domain, View::Bytes payload)
    -> Option<Language::Monograph&> {
  trace.restored_facts[trace.restoration_count] = payload;
  trace.restoration_count++;
  if (payload == "restore_fail"_view) {
    return {};
  }

  View::Bytes retained_fact = domain.proxy(payload);
  auto& monograph = domain.construct<WorkspaceMonograph>(
      domain, Documentation::get_empty(), *this, trace, retained_fact,
      View::Bytes(), Span(), identity);
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
      "^------\n"
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
      workspace.interpret_source(errors, "Unknown"_view, path, contents);

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
  remove_package_member(root, "main.ttx"_view);
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
  remove_package_member(root, "sibling.ttx"_view);
  remove_package_member(root, "sibling_member.ttx"_view);
  remove_package_member(root, "dependency.ttxa"_view);
  remove_package_member(root, "first.ttxa"_view);
  remove_package_member(root, "second.ttxa"_view);
  remove_package_member(root, "cycle.ttxa"_view);
  remove_package_member(root, "transitive.ttxa"_view);
  remove_package_member(root, "unsupported.ttxa"_view);
  remove_package_member(root, "mismatch.ttxa"_view);
  remove_package_member(root, "invalid.ttxa"_view);
  remove_package_member(root, "source_free.bin"_view);
  remove_package_member(root, "consumer.ttx"_view);
  remove_package_member(root, "resources/table.bin"_view);
  remove_package_member(root, "resources/empty.bin"_view);
  remove_package_member(root, "resources/later.bin"_view);
  remove_package_member(root, "resources"_view);
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

static auto write_archive(
    const TemporaryWorkspacePackage& package,
    View::Bytes route,
    View::Bytes identity,
    Version version,
    View::Vector<Package::Language::Dependency> dependencies,
    View::Vector<Package::Archive::Member> members) -> Bool {
  Package::Archive::Archive archive(
      identity, version, dependencies, members, View::Vector<View::Bytes>(),
      View::Vector<Package::Archive::Export>());
  auto encoded = Package::Archive::Writer::write(archive);
  if (!encoded) {
    return False;
  }

  return package.write(route, *encoded);
}

static auto returns_selection_error(
    const Result<Language::Monograph&, Package::Repository::SelectionError>&
        result,
    Package::Repository::SelectionError expected) -> Bool {
  return result.visit(
      [](const Language::Monograph&) { return False; },
      [&](Package::Repository::SelectionError actual) {
        return actual == expected ? True : False;
      });
}

static auto rejects_selection_failure(
    Count scenario,
    Package::Repository::SelectionError expected,
    View::Bytes expected_name) -> Bool {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  if (!package ||
      !package.write(
          "package.ttx"_view,
          "// Selection Package\n"
          "dialect : Package;\n"
          "resolve Dependency : Pkg.Expected = \"1.0\";\n"
          "source Main from \"main.ttx\";\n"_view) ||
      !package.write(
          "main.ttx"_view, "// Main\ndialect : Alpha;\nMainFact"_view)) {
    active_trace = nullptr;
    return False;
  }

  Dynamic::Bytes archive_location =
      join_package_path(package.get_root(), "dependency.ttxa"_view);
  if (scenario == 2) {
    if (!package.write("dependency.ttxa"_view, "invalid"_view)) {
      active_trace = nullptr;
      return False;
    }
  } else if (scenario == 3 || scenario == 4) {
    Static::Vector<Package::Archive::Member, 1> members = {{
      Package::Archive::Member("Value"_view, "Alpha"_view, "Fact"_view),
    }};
    View::Bytes identity =
        scenario == 4 ? "Pkg.Actual"_view : "Pkg.Expected"_view;
    Package::Archive::Archive archive(
        identity, Version(1, 0), View::Vector<Package::Language::Dependency>(),
        members, View::Vector<View::Bytes>(),
        View::Vector<Package::Archive::Export>());
    auto encoded = Package::Archive::Writer::write(archive);
    if (!encoded) {
      active_trace = nullptr;
      return False;
    }

    if (scenario == 3) {
      encoded->get_access()[4] = 2;
    }

    if (!package.write("dependency.ttxa"_view, *encoded)) {
      active_trace = nullptr;
      return False;
    }
  }

  Package::Repository::Input input(
      "Pkg.Expected"_view, Version(1, 0), archive_location,
      View::Vector<Package::Repository::Artifact>());
  View::Vector<Package::Repository::Input> inputs =
      scenario == 0 ? View::Vector<Package::Repository::Input>()
                    : View::Vector<Package::Repository::Input>(&input, 1);
  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, inputs, View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  if (!repository ||
      !workspace.install_dialect<Package::Dialect>("Package"_view) ||
      !workspace.install_dialect<WorkspaceDialect>("Alpha"_view)) {
    active_trace = nullptr;
    return False;
  }

  Errors errors;
  auto imported = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.Root"_view, Version(1, 0), *repository);
  Bool rejected =
      returns_selection_error(imported, expected) && errors.get_size() == 1 &&
      has_diagnostic(errors, expected_name) &&
      &workspace.resolve_context("Root"_view) == &Invalid::get_invalid() &&
      expected != Package::Repository::SelectionError::Unknown;
  active_trace = nullptr;
  return rejected;
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
  auto imported_result = workspace.interpret_source(
      errors, semantic_name, diagnostic_path, contents);
  ASSERT(imported_result);
  EXPECT(trace.installed_registries[0] == &workspace);
  EXPECT(trace.interpretation_contexts[0] == &workspace);

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
  ASSERT(workspace.link(errors));
  EXPECT(&workspace.resolve_context("Main"_view) == &imported);
  ASSERT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("Main"_view) == &imported);
  EXPECT_EQ(trace.links, Count(1));
  EXPECT_EQ(trace.finalizers, Count(1));

  Errors shared_path_errors;
  EXPECT(workspace.interpret_source(
      shared_path_errors, "Other"_view, "sources/main.ttx"_view,
      "// Other documentation\ndialect : Alpha;\nOther"_view));
  EXPECT_TEXT(workspace.resolve_context("Other"_view).get_name(), "Other"_view);
  ASSERT(workspace.link(shared_path_errors));
  ASSERT(workspace.finalize(shared_path_errors));
  EXPECT(shared_path_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, failed_import_nonpublication) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  EXPECT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));

  Errors parse_errors;
  EXPECT_NOT(workspace.interpret_source(
      parse_errors, "Malformed"_view, "malformed.ttx"_view,
      "dialect : Alpha;\nBad"_view));
  EXPECT(
      &workspace.resolve_context("Malformed"_view) == &Invalid::get_invalid());

  Errors rejected_errors;
  EXPECT_NOT(workspace.interpret_source(
      rejected_errors, "Retry"_view, "retry.ttx"_view,
      "// Rejected\ndialect : Alpha;\nreject"_view));
  EXPECT(&workspace.resolve_context("Retry"_view) == &Invalid::get_invalid());

  Errors retry_errors;
  EXPECT(workspace.interpret_source(
      retry_errors, "Retry"_view, "retry.ttx"_view,
      "// Accepted\ndialect : Alpha;\nAccepted"_view));
  const Abstract& retained = workspace.resolve_context("Retry"_view);
  const Count constructed = trace.monograph_constructions[0];

  Errors duplicate_errors;
  EXPECT_NOT(workspace.interpret_source(
      duplicate_errors, "Retry"_view, "other.ttx"_view,
      "// Other\ndialect : Alpha;\nOther"_view));
  EXPECT(&workspace.resolve_context("Retry"_view) == &retained);
  EXPECT_TEXT(retained.get_name(), "Accepted"_view);
  EXPECT_EQ(trace.monograph_constructions[0], constructed);
  EXPECT_EQ(duplicate_errors.get_size(), 1);
  ASSERT(workspace.link(retry_errors));
  ASSERT(workspace.finalize(retry_errors));
  EXPECT(&workspace.resolve_context("Retry"_view) == &retained);
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, explicit_source_closure) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));

  auto first = workspace.interpret_source(
      errors, "First"_view, "first.ttx"_view,
      "// First\ndialect : Alpha;\nFirstFact"_view);
  ASSERT(first);
  EXPECT(&workspace.resolve_context("First"_view) == &*first);
  EXPECT_NOT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("First"_view) == &*first);

  ASSERT(workspace.link(errors));
  EXPECT_NOT(workspace.link(errors));
  Count interpretations = trace.interpretation_count;
  EXPECT_NOT(workspace.interpret_source(
      errors, "Overlap"_view, "overlap.ttx"_view,
      "// Overlap\ndialect : Alpha;\nOverlapFact"_view));
  EXPECT_EQ(trace.interpretation_count, interpretations);
  ASSERT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("First"_view) == &*first);
  EXPECT_NOT(workspace.finalize(errors));
  EXPECT_EQ(trace.links, Count(1));
  EXPECT_EQ(trace.finalizers, Count(1));

  auto failed = workspace.interpret_source(
      errors, "Failed"_view, "failed.ttx"_view,
      "// Failed\ndialect : Alpha;\nfinalize_fail"_view);
  ASSERT(failed);
  ASSERT(workspace.link(errors));
  EXPECT_NOT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("Failed"_view) == &Invalid::get_invalid());
  EXPECT(has_diagnostic(errors, "Finalize rejected the retained fact."_view));

  auto retried = workspace.interpret_source(
      errors, "Failed"_view, "retried.ttx"_view,
      "// Retried\ndialect : Alpha;\nRetriedFact"_view);
  ASSERT(retried);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(&workspace.resolve_context("Failed"_view) == &*retried);

  auto abandoned = workspace.interpret_source(
      errors, "Abandoned"_view, "abandoned.ttx"_view,
      "// Abandoned\ndialect : Alpha;\nAbandonedFact"_view);
  ASSERT(abandoned);
  EXPECT(&workspace.resolve_context("Abandoned"_view) == &*abandoned);
  workspace.abandon();
  EXPECT(
      &workspace.resolve_context("Abandoned"_view) == &Invalid::get_invalid());

  auto linked = workspace.interpret_source(
      errors, "Linked"_view, "linked.ttx"_view,
      "// Linked\ndialect : Alpha;\nLinkedFact"_view);
  ASSERT(linked);
  ASSERT(workspace.link(errors));
  workspace.abandon();
  EXPECT(&workspace.resolve_context("Linked"_view) == &Invalid::get_invalid());
  EXPECT_EQ(trace.links, Count(4));
  EXPECT_EQ(trace.finalizers, Count(3));
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, staged_fifo_retention) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  Errors errors;
  {
    Errors::Report report(
        errors, "prior-workspace.ttx"_view, View::Bytes(),
        Anchor::create(Span()));
    report << "Earlier independent diagnostic."_view;
  }

  ASSERT(workspace.install_dialect<TracedPackageDialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  ASSERT(repository);

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
        "source Group::Second from \"./second.ttx\";\n"
        "source Sibling from \"sibling.ttx\";\n"_view));
    ASSERT(package.write(
        "nested/package.ttx"_view,
        "// Nested Package\n"
        "dialect : Package;\n"
        "source Deep from \"nested/deep.ttx\";\n"_view));
    ASSERT(package.write(
        "sibling.ttx"_view,
        "// Sibling Package\n"
        "dialect : Package;\n"
        "source Peer from \"sibling_member.ttx\";\n"_view));
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
    ASSERT(package.write(
        "sibling_member.ttx"_view,
        "// Peer documentation\n"
        "dialect : Alpha;\n"
        "Peer"_view));

    Dynamic::Bytes package_root(package.get_root());
    Dynamic::Bytes root_name("Root"_view);
    Dynamic::Bytes root_route("./package.ttx"_view);
    auto imported = workspace.import_package(
        errors, package_root, root_name, root_route, "Pkg.Root"_view,
        Version(1, 0), *repository);
    auto imported_root = imported.visit(
        [](Language::Monograph& root) { return &root; },
        [](Package::Repository::SelectionError) {
          return static_cast<Language::Monograph*>(nullptr);
        });
    ASSERT(imported_root);
    EXPECT(imported_root->is<Package::Language::Monograph>());
    EXPECT(imported_root == &workspace.resolve_context("Root"_view));
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

  ASSERT_EQ(trace.interpretation_count, 4);
  EXPECT_TEXT(trace.interpreted_facts[0], "First"_view);
  EXPECT_TEXT(trace.interpreted_facts[1], "Second"_view);
  EXPECT_TEXT(trace.interpreted_facts[2], "Deep"_view);
  EXPECT_TEXT(trace.interpreted_facts[3], "Peer"_view);
  EXPECT_TEXT(trace.interpreted_paths[0], "first.ttx"_view);
  EXPECT_TEXT(trace.interpreted_paths[1], "second.ttx"_view);
  EXPECT_TEXT(trace.interpreted_paths[2], "nested/deep.ttx"_view);
  EXPECT_TEXT(trace.interpreted_paths[3], "sibling_member.ttx"_view);

  const Abstract& root = workspace.resolve_context("Root"_view);
  ASSERT(root.is<Package::Language::Monograph>());
  const auto& root_package =
      static_cast<const Package::Language::Monograph&>(root);
  EXPECT_EQ(root_package.get_sources().get_size(), 4);

  const Abstract& first = root_package.resolve_context("First"_view).resolve();
  const Abstract& second =
      root_package.resolve_context("Group::Second"_view).resolve();
  const Abstract& nested =
      root_package.resolve_context("Nested"_view).resolve();
  ASSERT(nested.is<Package::Language::Monograph>());
  const auto& nested_package =
      static_cast<const Package::Language::Monograph&>(nested);
  const Abstract& deep = nested_package.resolve_context("Deep"_view).resolve();
  const Abstract& sibling =
      root_package.resolve_context("Sibling"_view).resolve();
  ASSERT(sibling.is<Package::Language::Monograph>());
  const auto& sibling_package =
      static_cast<const Package::Language::Monograph&>(sibling);
  const Abstract& peer = sibling_package.resolve_context("Peer"_view).resolve();
  ASSERT(&first != &Invalid::get_invalid());
  ASSERT(&second != &Invalid::get_invalid());
  ASSERT(&deep != &Invalid::get_invalid());
  ASSERT(&peer != &Invalid::get_invalid());

  // Parsing success cannot reveal which Abstract reached a Dialect. Compare
  // the borrowed identities so each nested member proves its exact owner.
  ASSERT_EQ(trace.package_interpretation_count, 3);
  EXPECT(trace.package_registry == &workspace);
  EXPECT(trace.installed_registries[0] == &workspace);
  EXPECT(trace.package_interpretation_contexts[0] == &workspace);
  EXPECT(trace.package_interpretation_contexts[1] == &root_package);
  EXPECT(trace.package_interpretation_contexts[2] == &root_package);
  EXPECT(trace.interpretation_contexts[0] == &root_package);
  EXPECT(trace.interpretation_contexts[1] == &root_package);
  EXPECT(trace.interpretation_contexts[2] == &nested_package);
  EXPECT(trace.interpretation_contexts[3] == &sibling_package);
  EXPECT(trace.interpretation_contexts[0] != &workspace);
  EXPECT(trace.interpretation_contexts[1] != &workspace);
  EXPECT(trace.interpretation_contexts[2] != &workspace);
  EXPECT(trace.interpretation_contexts[3] != &workspace);
  EXPECT(trace.interpretation_contexts[2] != &sibling_package);
  EXPECT(trace.interpretation_contexts[3] != &nested_package);

  const auto& first_monograph = static_cast<const WorkspaceMonograph&>(first);
  const auto& second_monograph = static_cast<const WorkspaceMonograph&>(second);
  const auto& deep_monograph = static_cast<const WorkspaceMonograph&>(deep);
  const auto& peer_monograph = static_cast<const WorkspaceMonograph&>(peer);
  EXPECT_TEXT(first_monograph.get_fact(), "First"_view);
  EXPECT_TEXT(second_monograph.get_fact(), "Second"_view);
  EXPECT_TEXT(deep_monograph.get_fact(), "Deep"_view);
  EXPECT_TEXT(peer_monograph.get_fact(), "Peer"_view);
  EXPECT_TEXT(first_monograph.get_diagnostic_path(), "first.ttx"_view);
  EXPECT_TEXT(second_monograph.get_diagnostic_path(), "second.ttx"_view);
  EXPECT_TEXT(deep_monograph.get_diagnostic_path(), "nested/deep.ttx"_view);
  EXPECT_TEXT(peer_monograph.get_diagnostic_path(), "sibling_member.ttx"_view);

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
  EXPECT(&workspace.resolve_context("First"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("Group::Second"_view) ==
      &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Nested"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Sibling"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Deep"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Peer"_view) == &Invalid::get_invalid());
  ASSERT_EQ(trace.links, 4);
  ASSERT_EQ(trace.finalizers, 4);
  EXPECT_TEXT(trace.link_facts[0], "First"_view);
  EXPECT_TEXT(trace.link_facts[1], "Second"_view);
  EXPECT_TEXT(trace.link_facts[2], "Deep"_view);
  EXPECT_TEXT(trace.link_facts[3], "Peer"_view);
  EXPECT_TEXT(trace.finalize_facts[0], "First"_view);
  EXPECT_TEXT(trace.finalize_facts[1], "Second"_view);
  EXPECT_TEXT(trace.finalize_facts[2], "Deep"_view);
  EXPECT_TEXT(trace.finalize_facts[3], "Peer"_view);
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, staged_failures) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  Errors errors;

  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  ASSERT(repository);

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

    auto imported = workspace.import_package(
        errors, package.get_root(), "Root"_view, "package.ttx"_view,
        "Pkg.Root"_view, Version(1, 0), *repository);
    EXPECT(returns_selection_error(
        imported, Package::Repository::SelectionError::Unknown));
  }

  ASSERT_EQ(trace.interpretation_count, 5);
  EXPECT_TEXT(trace.interpreted_facts[0], "Keep"_view);
  EXPECT_TEXT(trace.interpreted_facts[1], "reject"_view);
  EXPECT_TEXT(trace.interpreted_facts[2], "Later"_view);
  EXPECT_TEXT(trace.interpreted_facts[3], "Duplicate"_view);
  EXPECT_TEXT(trace.interpreted_facts[4], "After"_view);

  EXPECT(&workspace.resolve_context("Root"_view) == &Invalid::get_invalid());

  EXPECT(&workspace.resolve_context("Unknown"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("Malformed"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("Rejected"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Missing"_view) == &Invalid::get_invalid());
  EXPECT(
      &workspace.resolve_context("duplicate.ttx"_view) ==
      &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Keep"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Later"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("After"_view) == &Invalid::get_invalid());
  EXPECT(&workspace.resolve_context("Nested"_view) == &Invalid::get_invalid());

  EXPECT_EQ(errors.get_size(), 2);
  EXPECT(has_diagnostic(errors, "Unknown dialect Missing"_view));
  EXPECT(has_diagnostic(
      errors, "Source is missing required documentation comment."_view));
  EXPECT(
      Test::error_contains(
          "Environment::Workspace Package import failed. reason=the staged "
          "semantic source could not be read semantic_name=Missing "
          "logical_route=missing.ttx storage_error=Unreadable"_view,
          Diagnostics::Log::Level::Info));
  EXPECT_EQ(trace.links, Count(0));
  EXPECT_EQ(trace.finalizers, Count(0));

  Errors later_errors;
  ASSERT(workspace.interpret_source(
      later_errors, "Recovered"_view, "recovered.ttx"_view,
      "// Recovered\ndialect : Alpha;\nRecovered"_view));
  ASSERT(workspace.link(later_errors));
  ASSERT(workspace.finalize(later_errors));
  EXPECT_EQ(trace.links, Count(1));
  EXPECT_EQ(trace.finalizers, Count(1));
  EXPECT_TEXT(trace.link_facts[0], "Recovered"_view);
  EXPECT_TEXT(trace.finalize_facts[0], "Recovered"_view);
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, resource_lifecycle) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  Errors errors;

  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  ASSERT(workspace.install_dialect<ResourceDialect>("Resource"_view));
  ASSERT(repository);

  {
    TemporaryWorkspacePackage package;
    ASSERT(package);
    ASSERT(package.create_directory("resources"_view));
    ASSERT(package.write(
        "package.ttx"_view,
        "// Resource Package\n"
        "dialect : Package;\n"
        "source Member from \"main.ttx\";\n"
        "source Consumer from \"consumer.ttx\";\n"_view));
    ASSERT(package.write(
        "main.ttx"_view, "// Member\ndialect : Alpha;\nMemberFact"_view));
    ASSERT(package.write(
        "consumer.ttx"_view,
        "// Consumer\n"
        "dialect : Resource;\n"
        "$[resources/table.bin] $[resources/empty.bin]"_view));
    ASSERT(package.write("resources/table.bin"_view, "table bytes"_view));
    ASSERT(package.write("resources/empty.bin"_view, View::Bytes()));
    ASSERT(package.write("resources/later.bin"_view, "later bytes"_view));

    auto imported = workspace.import_package(
        errors, package.get_root(), "Root"_view, "package.ttx"_view,
        "Pkg.Root"_view, Version(1, 0), *repository);
    Bool selected = imported.visit(
        [](Language::Monograph&) { return True; },
        [](Package::Repository::SelectionError) { return False; });
    ASSERT(selected);
  }

  for (Count i = 0; i < 8; i++) {
    ASSERT(trace.resource_results[i] != nullptr);
  }
  ASSERT(trace.literal_results[0] != nullptr);
  ASSERT(trace.literal_results[1] != nullptr);

  EXPECT(trace.resource_link_seen);
  EXPECT(trace.resources_sealed_during_link);
  EXPECT(trace.resource_results[0]->is<Tetrodotoxin::Language::Resource>());
  EXPECT(trace.resource_results[2]->is<Tetrodotoxin::Language::Resource>());
  EXPECT(trace.resource_results[3]->is<Tetrodotoxin::Language::Error>());
  EXPECT(trace.resource_results[5]->is<Tetrodotoxin::Language::Error>());
  EXPECT(trace.resource_results[6]->is<Tetrodotoxin::Language::Error>());
  EXPECT(trace.resource_results[0] == trace.resource_results[1]);
  EXPECT(trace.resource_results[3] == trace.resource_results[4]);
  EXPECT(trace.resource_results[5] != trace.resource_results[6]);

  const auto& table = static_cast<const Tetrodotoxin::Language::Resource&>(
      *trace.resource_results[0]);
  const auto& empty = static_cast<const Tetrodotoxin::Language::Resource&>(
      *trace.resource_results[2]);
  EXPECT_TEXT(table.get_value(), "table bytes"_view);
  EXPECT(empty.get_value().is_empty());
  const auto& table_literal =
      static_cast<const Library::Language::Constants::Bytes&>(
          *trace.literal_results[0]);
  const auto& empty_literal =
      static_cast<const Library::Language::Constants::Bytes&>(
          *trace.literal_results[1]);
  EXPECT_TEXT(table_literal.get_value(), "table bytes"_view);
  EXPECT(empty_literal.get_value().is_empty());
  EXPECT_TEXT(
      trace.resource_results[7]->resolve().get_name(), "MemberFact"_view);

  const Abstract& root = workspace.resolve_context("Root"_view);
  ASSERT(root.is<Package::Language::Monograph>());
  const auto& root_package =
      static_cast<const Package::Language::Monograph&>(root);
  ASSERT(trace.resources_owner != nullptr);
  EXPECT(trace.resources_owner == &root_package.get_resources());
  EXPECT(
      &root_package.resolve_context("$[resources/table.bin]"_view) ==
      trace.resource_results[0]);
  EXPECT(
      &root_package.resolve_context("$[resources/missing.bin]"_view) ==
      trace.resource_results[3]);
  EXPECT(
      &root_package.resolve_context("$[resources/later.bin]"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &root_package.resolve_context("Member"_view) ==
      trace.resource_results[7]);
  EXPECT(&workspace.resolve_context("Member"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, source_free_archive_consumer) {
  WorkspaceTrace trace;
  active_trace = &trace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// Consumer Package\n"
      "dialect : Package;\n"
      "resolve Dependency : Pkg.Dependency = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view,
      "// Consumer Main\n"
      "dialect : Alpha;\n"
      "AuthoredFact"_view));
  ASSERT(package.write("source_free.bin"_view, "must not be read"_view));

  // The producer owns every semantic input and the encoded product only until
  // the physical Archive is written. Destroying that storage before the
  // consumer exists catches any accidental same object round trip.
  {
    Allocator::Arena producer_arena;
    Managed::Vector<Package::Archive::Member> members(producer_arena);
    members.insert(
        Package::Archive::Member(
            producer_arena.proxy("First"_view),
            producer_arena.proxy("Alpha"_view),
            producer_arena.proxy("RestoredFirst"_view)));
    members.insert(
        Package::Archive::Member(
            producer_arena.proxy("Second"_view),
            producer_arena.proxy("Alpha"_view),
            producer_arena.proxy("RestoredSecond"_view)));
    Package::Archive::Archive archive(
        producer_arena.proxy("Pkg.Dependency"_view), Version(1, 0),
        View::Vector<Package::Language::Dependency>(), members,
        View::Vector<View::Bytes>(), View::Vector<Package::Archive::Export>());
    auto encoded = Package::Archive::Writer::write(archive);
    ASSERT(encoded);
    ASSERT(package.write("dependency.ttxa"_view, *encoded));
  }

  Environment::Workspace workspace;
  Errors errors;
  const Package::Language::Monograph* retained_dependency = nullptr;
  // Repository owns the decoded Archive for this transaction and declares no
  // source or native product. The fresh Workspace must copy every retained
  // identity and payload result before the Repository Arena disappears.
  {
    Allocator::Arena repository_arena;
    Dynamic::Bytes archive_location =
        join_package_path(package.get_root(), "dependency.ttxa"_view);
    Package::Repository::Input input(
        "Pkg.Dependency"_view, Version(1, 0), archive_location,
        View::Vector<Package::Repository::Artifact>());
    auto repository = Package::Repository::Repository::create(
        repository_arena, View::Vector<Package::Repository::Input>(&input, 1),
        View::Vector<Package::Repository::Output>(),
        View::Vector<Package::Repository::Output>());
    ASSERT(repository);
    ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
    ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));

    auto imported = workspace.import_package(
        errors, package.get_root(), "Root"_view, "package.ttx"_view,
        "Pkg.Root"_view, Version(1, 0), *repository);
    auto imported_root = imported.visit(
        [](Language::Monograph& root) { return &root; },
        [](Package::Repository::SelectionError) {
          return static_cast<Language::Monograph*>(nullptr);
        });
    ASSERT(imported_root);
    ASSERT(imported_root->is<Package::Language::Monograph>());
    const auto& root =
        static_cast<const Package::Language::Monograph&>(*imported_root);
    const Abstract& authored = root.resolve_context("Main"_view).resolve();
    const Abstract& dependency =
        root.resolve_context("Dependency"_view).resolve();
    ASSERT(dependency.is<Package::Language::Monograph>());
    retained_dependency =
        &static_cast<const Package::Language::Monograph&>(dependency);

    EXPECT_TEXT(authored.get_name(), "AuthoredFact"_view);
    EXPECT_TEXT(
        retained_dependency->resolve_context("First"_view).resolve().get_name(),
        "RestoredFirst"_view);
    EXPECT_TEXT(
        retained_dependency->resolve_context("Second"_view)
            .resolve()
            .get_name(),
        "RestoredSecond"_view);
    EXPECT(&workspace.resolve_context("Main"_view) == &Invalid::get_invalid());
    EXPECT(&workspace.resolve_context("First"_view) == &Invalid::get_invalid());
    EXPECT(
        &workspace.resolve_context("Second"_view) == &Invalid::get_invalid());
    EXPECT(
        &workspace.resolve_context("Dependency"_view) ==
        &Invalid::get_invalid());
    EXPECT(errors.is_empty());
  }

  // Only Workspace storage survives this point, so both Alias traversals prove
  // the consumer did not retain producer or Repository views.
  ASSERT(retained_dependency);
  EXPECT_TEXT(
      retained_dependency->resolve_context("First"_view).resolve().get_name(),
      "RestoredFirst"_view);
  EXPECT_TEXT(
      retained_dependency->resolve_context("Second"_view).resolve().get_name(),
      "RestoredSecond"_view);
  EXPECT(
      &retained_dependency->resolve_context("$[source_free.bin]"_view) ==
      &Invalid::get_invalid());
  ASSERT_EQ(trace.restoration_count, 2);
  EXPECT_TEXT(trace.restored_facts[0], "RestoredFirst"_view);
  EXPECT_TEXT(trace.restored_facts[1], "RestoredSecond"_view);
  ASSERT_EQ(trace.links, 3);
  ASSERT_EQ(trace.finalizers, 3);
  EXPECT_TEXT(trace.link_facts[0], "AuthoredFact"_view);
  EXPECT_TEXT(trace.link_facts[1], "RestoredFirst"_view);
  EXPECT_TEXT(trace.link_facts[2], "RestoredSecond"_view);
  EXPECT_TEXT(trace.finalize_facts[0], "AuthoredFact"_view);
  EXPECT_TEXT(trace.finalize_facts[1], "RestoredFirst"_view);
  EXPECT_TEXT(trace.finalize_facts[2], "RestoredSecond"_view);
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, exact_key_reuse_and_conflict) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// Reuse Package\n"
      "dialect : Package;\n"
      "resolve First : Pkg.Shared = \"1.0\";\n"
      "resolve Again : Pkg.Shared = \"1.0\";\n"
      "resolve Conflict : Pkg.Shared = \"2.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Main\ndialect : Alpha;\nMainFact"_view));

  Static::Vector<Package::Archive::Member, 1> first_members = {{
    Package::Archive::Member("Value"_view, "Alpha"_view, "SharedOne"_view),
  }};
  Static::Vector<Package::Archive::Member, 1> second_members = {{
    Package::Archive::Member("Value"_view, "Alpha"_view, "SharedTwo"_view),
  }};
  ASSERT(write_archive(
      package, "first.ttxa"_view, "Pkg.Shared"_view, Version(1, 0),
      View::Vector<Package::Language::Dependency>(), first_members));
  ASSERT(write_archive(
      package, "second.ttxa"_view, "Pkg.Shared"_view, Version(2, 0),
      View::Vector<Package::Language::Dependency>(), second_members));

  Allocator::Arena repository_arena;
  Dynamic::Bytes first_location =
      join_package_path(package.get_root(), "first.ttxa"_view);
  Dynamic::Bytes second_location =
      join_package_path(package.get_root(), "second.ttxa"_view);
  Static::Vector<Package::Repository::Input, 2> inputs = {{
    Package::Repository::Input(
        "Pkg.Shared"_view, Version(1, 0), first_location,
        View::Vector<Package::Repository::Artifact>()),
    Package::Repository::Input(
        "Pkg.Shared"_view, Version(2, 0), second_location,
        View::Vector<Package::Repository::Artifact>()),
  }};
  auto repository = Package::Repository::Repository::create(
      repository_arena, inputs, View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  Errors errors;

  auto imported = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.Root"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      imported, Package::Repository::SelectionError::Unknown));
  EXPECT(&workspace.resolve_context("Root"_view) == &Invalid::get_invalid());
  EXPECT_EQ(trace.restoration_count, 1);
  EXPECT_TEXT(trace.restored_facts[0], "SharedOne"_view);
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(has_diagnostic(errors, "Conflict resolves Pkg.Shared"_view));
  EXPECT(has_diagnostic(
      errors,
      "the Package identity is already retained with another Version"_view));
  EXPECT(
      has_diagnostic(errors, "resolve Conflict : Pkg.Shared = \"2.0\";"_view));
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(
    EnvironmentWorkspace,
    failed_finalize_keeps_cache_incomplete) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// First Package\n"
      "dialect : Package;\n"
      "resolve Shared : Pkg.Shared = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Main\ndialect : Alpha;\nFirstFact"_view));

  Static::Vector<Package::Archive::Member, 1> members = {{
    Package::Archive::Member("Value"_view, "Alpha"_view, "finalize_fail"_view),
  }};
  ASSERT(write_archive(
      package, "first.ttxa"_view, "Pkg.Shared"_view, Version(1, 0),
      View::Vector<Package::Language::Dependency>(), members));

  Allocator::Arena repository_arena;
  Dynamic::Bytes archive_location =
      join_package_path(package.get_root(), "first.ttxa"_view);
  Package::Repository::Input input(
      "Pkg.Shared"_view, Version(1, 0), archive_location,
      View::Vector<Package::Repository::Artifact>());
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(&input, 1),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  Errors first_errors;

  auto first_import = workspace.import_package(
      first_errors, package.get_root(), "FirstRoot"_view, "package.ttx"_view,
      "Pkg.First"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      first_import, Package::Repository::SelectionError::Unknown));
  EXPECT(
      &workspace.resolve_context("FirstRoot"_view) == &Invalid::get_invalid());
  ASSERT_EQ(trace.restoration_count, 1);
  EXPECT_TEXT(trace.restored_facts[0], "finalize_fail"_view);
  ASSERT_EQ(trace.links, 2);
  ASSERT_EQ(trace.finalizers, 2);
  EXPECT_TEXT(trace.link_facts[0], "FirstFact"_view);
  EXPECT_TEXT(trace.link_facts[1], "finalize_fail"_view);
  EXPECT_TEXT(trace.finalize_facts[0], "FirstFact"_view);
  EXPECT_TEXT(trace.finalize_facts[1], "finalize_fail"_view);
  EXPECT_EQ(first_errors.get_size(), 1);
  EXPECT(has_diagnostic(
      first_errors, "Finalize rejected the retained fact."_view));
  EXPECT(has_diagnostic(
      first_errors, "resolve Shared : Pkg.Shared = \"1.0\";"_view));

  ASSERT(package.write(
      "package.ttx"_view,
      "// Second Package\n"
      "dialect : Package;\n"
      "resolve Shared : Pkg.Shared = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Main\ndialect : Alpha;\nSecondFact"_view));
  Errors second_errors;

  auto second_import = workspace.import_package(
      second_errors, package.get_root(), "SecondRoot"_view, "package.ttx"_view,
      "Pkg.Second"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      second_import, Package::Repository::SelectionError::Unknown));
  EXPECT(
      &workspace.resolve_context("SecondRoot"_view) == &Invalid::get_invalid());
  EXPECT_EQ(trace.restoration_count, 1);
  EXPECT_EQ(trace.links, 2);
  EXPECT_EQ(trace.finalizers, 2);
  EXPECT_EQ(second_errors.get_size(), 1);
  EXPECT(has_diagnostic(
      second_errors,
      "the exact Package key was already restored but did not complete"_view));
  EXPECT(has_diagnostic(
      second_errors, "resolve Shared : Pkg.Shared = \"1.0\";"_view));
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, active_cycle_inherited_chain) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// Cycle Package\n"
      "dialect : Package;\n"
      "resolve Cycle : Pkg.Cycle = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Main\ndialect : Alpha;\nMainFact"_view));

  Static::Vector<Package::Language::Dependency, 1> cycle_dependencies = {{
    Package::Language::Dependency("Self"_view, "Pkg.Cycle"_view, Version(1, 0)),
  }};
  Static::Vector<Package::Archive::Member, 1> cycle_members = {{
    Package::Archive::Member("Value"_view, "Alpha"_view, "CycleFact"_view),
  }};
  ASSERT(write_archive(
      package, "cycle.ttxa"_view, "Pkg.Cycle"_view, Version(1, 0),
      cycle_dependencies, cycle_members));

  Allocator::Arena repository_arena;
  Dynamic::Bytes archive_location =
      join_package_path(package.get_root(), "cycle.ttxa"_view);
  Package::Repository::Input input(
      "Pkg.Cycle"_view, Version(1, 0), archive_location,
      View::Vector<Package::Repository::Artifact>());
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(&input, 1),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  Errors errors;

  auto imported = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.Root"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      imported, Package::Repository::SelectionError::Unknown));
  EXPECT(&workspace.resolve_context("Root"_view) == &Invalid::get_invalid());
  EXPECT_EQ(trace.restoration_count, 1);
  EXPECT_TEXT(trace.restored_facts[0], "CycleFact"_view);
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(has_diagnostic(errors, "Cycle resolves Pkg.Cycle"_view));
  EXPECT(has_diagnostic(errors, "Self resolves Pkg.Cycle"_view));
  EXPECT(
      has_diagnostic(errors, "the exact Package key is already active"_view));
  EXPECT(has_diagnostic(errors, "resolve Cycle : Pkg.Cycle = \"1.0\";"_view));
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, transitive_typed_failure_chain) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// Transitive Package\n"
      "dialect : Package;\n"
      "resolve Middle : Pkg.Middle = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Main\ndialect : Alpha;\nMainFact"_view));

  Static::Vector<Package::Language::Dependency, 1> dependencies = {{
    Package::Language::Dependency(
        "Missing"_view, "Pkg.Missing"_view, Version(1, 0)),
  }};
  Static::Vector<Package::Archive::Member, 1> members = {{
    Package::Archive::Member("Value"_view, "Alpha"_view, "MiddleFact"_view),
  }};
  ASSERT(write_archive(
      package, "transitive.ttxa"_view, "Pkg.Middle"_view, Version(1, 0),
      dependencies, members));

  Allocator::Arena repository_arena;
  Dynamic::Bytes archive_location =
      join_package_path(package.get_root(), "transitive.ttxa"_view);
  Package::Repository::Input input(
      "Pkg.Middle"_view, Version(1, 0), archive_location,
      View::Vector<Package::Repository::Artifact>());
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(&input, 1),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  Errors errors;

  auto imported = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.Root"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      imported, Package::Repository::SelectionError::NotDeclared));
  EXPECT(&workspace.resolve_context("Root"_view) == &Invalid::get_invalid());
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(has_diagnostic(errors, "Middle resolves Pkg.Middle"_view));
  EXPECT(has_diagnostic(errors, "Missing resolves Pkg.Missing"_view));
  EXPECT(has_diagnostic(errors, "NotDeclared"_view));
  EXPECT(has_diagnostic(errors, "resolve Middle : Pkg.Middle = \"1.0\";"_view));
  EXPECT(
      Test::error_contains(
          "selection_error=NotDeclared requested_identity=Pkg.Missing "
          "requested_version=1.0"_view,
          Diagnostics::Log::Level::Info));
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, independent_dependency_reports) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// Independent Package\n"
      "dialect : Package;\n"
      "resolve First : Pkg.First = \"1.0\";\n"
      "resolve Second : Pkg.Second = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Main\ndialect : Alpha;\nMainFact"_view));

  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  Errors errors;

  auto imported = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.Root"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      imported, Package::Repository::SelectionError::NotDeclared));
  EXPECT(&workspace.resolve_context("Root"_view) == &Invalid::get_invalid());
  EXPECT_EQ(errors.get_size(), 2);
  EXPECT(has_diagnostic(errors, "First resolves Pkg.First"_view));
  EXPECT(has_diagnostic(errors, "Second resolves Pkg.Second"_view));
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, typed_selection_failures) {
  EXPECT(rejects_selection_failure(
      0, Package::Repository::SelectionError::NotDeclared, "NotDeclared"_view));
  EXPECT(rejects_selection_failure(
      1, Package::Repository::SelectionError::Unreadable, "Unreadable"_view));
  EXPECT(rejects_selection_failure(
      2, Package::Repository::SelectionError::InvalidFormat,
      "InvalidFormat"_view));
  EXPECT(rejects_selection_failure(
      3, Package::Repository::SelectionError::UnsupportedFormat,
      "UnsupportedFormat"_view));
  EXPECT(rejects_selection_failure(
      4, Package::Repository::SelectionError::PackageKeyMismatch,
      "PackageKeyMismatch"_view));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, context_free_typed_failure) {
  Package::Language::Dependency dependencies[] = {
    Package::Language::Dependency(
        "Dependency"_view, "Pkg.Missing"_view, Version(1, 0)),
  };
  Environment::Workspace registry;
  Allocator::Arena arena;
  Environment::Dialects dialects(arena, registry);
  Environment::Retention retention(arena);
  Environment::Resolution resolution(arena, dialects, retention);
  auto& root = Package::Language::Monograph::create_synthetic(
      arena, Documentation::get_empty(), dependencies);
  retention.retain(root, {});

  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  Errors errors;

  auto imported = resolution.resolve(
      errors, 0, "Pkg.Root"_view, Version(1, 0), root, *repository);
  EXPECT(returns_selection_error(
      imported, Package::Repository::SelectionError::NotDeclared));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, member_restoration_failures) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// Member Failure Package\n"
      "dialect : Package;\n"
      "resolve MissingDialect : Pkg.MissingDialect = \"1.0\";\n"
      "resolve RestoreFailure : Pkg.RestoreFailure = \"1.0\";\n"
      "resolve FailedAgain : Pkg.RestoreFailure = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Main\ndialect : Alpha;\nMainFact"_view));

  Static::Vector<Package::Archive::Member, 1> missing_members = {{
    Package::Archive::Member("Value"_view, "Missing"_view, "Ignored"_view),
  }};
  Static::Vector<Package::Archive::Member, 1> failed_members = {{
    Package::Archive::Member("Value"_view, "Alpha"_view, "restore_fail"_view),
  }};
  ASSERT(write_archive(
      package, "first.ttxa"_view, "Pkg.MissingDialect"_view, Version(1, 0),
      View::Vector<Package::Language::Dependency>(), missing_members));
  ASSERT(write_archive(
      package, "second.ttxa"_view, "Pkg.RestoreFailure"_view, Version(1, 0),
      View::Vector<Package::Language::Dependency>(), failed_members));

  Allocator::Arena repository_arena;
  Dynamic::Bytes first_location =
      join_package_path(package.get_root(), "first.ttxa"_view);
  Dynamic::Bytes second_location =
      join_package_path(package.get_root(), "second.ttxa"_view);
  Static::Vector<Package::Repository::Input, 2> inputs = {{
    Package::Repository::Input(
        "Pkg.MissingDialect"_view, Version(1, 0), first_location,
        View::Vector<Package::Repository::Artifact>()),
    Package::Repository::Input(
        "Pkg.RestoreFailure"_view, Version(1, 0), second_location,
        View::Vector<Package::Repository::Artifact>()),
  }};
  auto repository = Package::Repository::Repository::create(
      repository_arena, inputs, View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  Errors errors;

  auto imported = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.Root"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      imported, Package::Repository::SelectionError::Unknown));
  EXPECT(&workspace.resolve_context("Root"_view) == &Invalid::get_invalid());
  EXPECT_EQ(trace.restoration_count, 1);
  EXPECT_TEXT(trace.restored_facts[0], "restore_fail"_view);
  EXPECT_EQ(errors.get_size(), 3);
  EXPECT(has_diagnostic(
      errors, "an Archive member names a Dialect that is not installed"_view));
  EXPECT(has_diagnostic(
      errors, "an Archive member payload could not be restored"_view));
  EXPECT(has_diagnostic(
      errors,
      "the exact Package key was already restored but did not complete"_view));
  EXPECT(has_diagnostic(errors, "resolve MissingDialect"_view));
  EXPECT(has_diagnostic(errors, "resolve RestoreFailure"_view));
  EXPECT(has_diagnostic(errors, "resolve FailedAgain"_view));
  active_trace = nullptr;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, link_finalize_order_and_reuse) {
  WorkspaceTrace trace;
  active_trace = &trace;
  Environment::Workspace workspace;
  TemporaryWorkspacePackage package;
  ASSERT(package);
  ASSERT(package.write(
      "package.ttx"_view,
      "// First Package\n"
      "dialect : Package;\n"
      "source First from \"first.ttx\";\n"
      "source Second from \"second.ttx\";\n"_view));
  ASSERT(package.write(
      "first.ttx"_view, "// First\ndialect : Alpha;\nlink_fail"_view));
  ASSERT(package.write(
      "second.ttx"_view, "// Second\ndialect : Alpha;\nGood"_view));

  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
  Errors errors;

  auto first_import = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.First"_view, Version(1, 0), *repository);
  EXPECT(returns_selection_error(
      first_import, Package::Repository::SelectionError::Unknown));
  ASSERT_EQ(trace.links, 2);
  EXPECT_EQ(trace.finalizers, Count(0));
  EXPECT_TEXT(trace.link_facts[0], "link_fail"_view);
  EXPECT_TEXT(trace.link_facts[1], "Good"_view);
  EXPECT_EQ(errors.get_size(), 1);
  EXPECT(has_diagnostic(errors, "Link rejected the retained fact."_view));
  EXPECT(&workspace.resolve_context("Root"_view) == &Invalid::get_invalid());

  ASSERT(workspace.interpret_source(
      errors, "Direct"_view, "direct.ttx"_view,
      "// Direct\ndialect : Alpha;\nDirectFact"_view));
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT_EQ(trace.links, Count(3));
  EXPECT_EQ(trace.finalizers, Count(1));
  ASSERT(package.write(
      "package.ttx"_view,
      "// Second Package\n"
      "dialect : Package;\n"
      "source Later from \"main.ttx\";\n"_view));
  ASSERT(package.write(
      "main.ttx"_view, "// Later\ndialect : Alpha;\nLaterFact"_view));

  auto second_import = workspace.import_package(
      errors, package.get_root(), "OtherRoot"_view, "package.ttx"_view,
      "Pkg.Second"_view, Version(1, 0), *repository);
  Bool second_selected = second_import.visit(
      [](Language::Monograph&) { return True; },
      [](Package::Repository::SelectionError) { return False; });
  EXPECT(second_selected);
  ASSERT_EQ(trace.links, 4);
  ASSERT_EQ(trace.finalizers, 2);
  EXPECT_TEXT(trace.link_facts[2], "DirectFact"_view);
  EXPECT_TEXT(trace.link_facts[3], "LaterFact"_view);
  EXPECT_TEXT(trace.finalize_facts[0], "DirectFact"_view);
  EXPECT_TEXT(trace.finalize_facts[1], "LaterFact"_view);
  EXPECT_EQ(errors.get_size(), 1);
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
    EXPECT(workspace.interpret_source(
        errors, "First"_view, "first.ttx"_view,
        "// First documentation\ndialect : Alpha;\nFirst"_view));
    EXPECT(workspace.interpret_source(
        errors, "Second"_view, "second.ttx"_view,
        "// Second documentation\ndialect : Beta;\nSecond"_view));
    EXPECT(workspace.link(errors));
    EXPECT(workspace.finalize(errors));
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
