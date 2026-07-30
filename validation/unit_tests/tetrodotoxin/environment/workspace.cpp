// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

struct WorkspaceTrace {
  const Language::Dialect* instances[4]{};
  View::Bytes expected_facts[4]{};
  View::Bytes expected_documentation[4]{};
  Count dialect_constructions = 0;
  Count monograph_constructions[4]{};
  Count dialect_destructions[4]{};
  Count monograph_destructions[4]{};
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
      Count host_identity)
      : Monograph(domain, documentation, host),
        trace(trace),
        fact(fact),
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

  auto get_fact() const -> View::Bytes { return fact; }

 private:
  WorkspaceTrace& trace;
  View::Bytes fact;
  Bool& host_alive;
  Count host_identity;
};

auto WorkspaceDialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Abstract& registry) -> Option<Monograph&> {
  View::Bytes fact = cursor.get_text();
  if (fact == "reject"_view) {
    return {};
  }

  retained_state = fact;
  auto& monograph = domain.construct<WorkspaceMonograph>(
      domain, documentation, *this, trace, fact, identity);
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
  const Bool imported =
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
        expected_unknown_diagnostic("No dialects are installed."_view));
  }

  {
    Environment::Workspace workspace;
    EXPECT(workspace.install_dialect<WorkspaceDialect>("Alpha"_view));
    EXPECT_TEXT(
        render_unknown(workspace),
        expected_unknown_diagnostic("Installed dialect: Alpha."_view));
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
  EXPECT(workspace.import_source(
      semantic_name, diagnostic_path, contents, errors));

  semantic_name.set('x');
  diagnostic_path.set('x');
  contents.set('x');

  const Abstract& imported = workspace.resolve_context("Main"_view);
  const auto& monograph = static_cast<const WorkspaceMonograph&>(imported);
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
