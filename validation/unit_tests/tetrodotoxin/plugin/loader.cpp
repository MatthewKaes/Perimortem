// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/plugin/loader.hpp"

#include "validation/unit_test.hpp"

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <ftw.h>
#include <string>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "puffer/publisher.hpp"
#include "tetrodotoxin/environment/provider_closure.hpp"
#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/query.hpp"
#include "tetrodotoxin/terminal/graph_text.hpp"
#include "tetrodotoxin/terminal/query.hpp"
#include "ttx/query.hpp"
#include "ttx/model/requirement.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness PluginLoader = {
  .name = "Tetrodotoxin::Plugin::Loader"_view,
};

static auto remove_entry(const char* path, const struct stat*, S32, struct FTW*)
    -> S32 {
  return remove(path);
}

static auto same(ttx_borrowed_bytes value, const char* text) -> bool {
  const size_t size = std::strlen(text);
  return value.size == size &&
         (size == 0 || std::memcmp(value.data, text, size) == 0);
}

static auto runfile_path(const char* relative, const char* fallback)
    -> std::string {
  const char* runfiles = std::getenv("RUNFILES_DIR");
  if (runfiles == nullptr) {
    runfiles = std::getenv("TEST_SRCDIR");
  }
  if (runfiles != nullptr) {
    std::string path = std::string(runfiles) + "/_main/" + relative;
    if (access(path.c_str(), R_OK) == 0) {
      return path;
    }
  }
  return fallback;
}

static auto plugin_path() -> std::string {
  return runfile_path(
      "validation/plugin_fixture.so", ".bin/bin/validation/plugin_fixture.so");
}

static auto graph_text_plugin_path() -> std::string {
  return runfile_path(
      "tetrodotoxin/terminal/graph_text_plugin.so",
      ".bin/bin/tetrodotoxin/terminal/graph_text_plugin.so");
}

struct SourceInput {
  tetrodotoxin_source_input_ops operations;
  const char* path;
  const char* source;
};

static auto source_input(tetrodotoxin_source_input_self* self) -> SourceInput& {
  return *reinterpret_cast<SourceInput*>(self);
}

static auto TTX_CALL source_path(tetrodotoxin_source_input_self* self)
    -> ttx_borrowed_bytes {
  SourceInput& input = source_input(self);
  return {
    .data = reinterpret_cast<const uint8_t*>(input.path),
    .size = std::strlen(input.path),
  };
}

static auto TTX_CALL source_bytes(tetrodotoxin_source_input_self* self)
    -> ttx_borrowed_bytes {
  SourceInput& input = source_input(self);
  return {
    .data = reinterpret_cast<const uint8_t*>(input.source),
    .size = std::strlen(input.source),
  };
}

static void TTX_CALL source_tokens(
    tetrodotoxin_source_input_self*,
    tetrodotoxin_token_sink result) {
  result.operations->completed(result.self);
}

static auto fixture_source(const char* source) -> SourceInput {
  return {
    .operations =
        {
          .header =
              {
                .size = sizeof(tetrodotoxin_source_input_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .diagnostic_path = source_path,
          .bytes = source_bytes,
          .visit_tokens = source_tokens,
        },
    .path = "fixture.ttx",
    .source = source,
  };
}

struct HostRequirement {
  ttx_host_requirements_ops operations;
  ttx_abstract requirement;
  bool resolved;
};

static void TTX_CALL resolve_requirement(
    ttx_host_requirements_self* self,
    const ttx_requirement_descriptor* descriptor,
    ttx_requirement_result result) {
  auto& host = *reinterpret_cast<HostRequirement*>(self);
  const bool matches =
      descriptor != nullptr &&
      same(descriptor->package_coordinate, "Validation.Contracts") &&
      same(descriptor->exported_route, "Echo") &&
      same(descriptor->contract_version, "1") &&
      descriptor->content_sha256[0] == 1;
  if (!matches) {
    result.operations->missing(result.self);
    return;
  }
  host.resolved = true;
  result.operations->resolved(result.self, host.requirement);
}

static auto descriptor_matches(
    const ttx_requirement_descriptor* descriptor,
    const char* package,
    const char* route,
    const char* version,
    uint8_t digest) -> bool {
  return descriptor != nullptr &&
         same(descriptor->package_coordinate, package) &&
         same(descriptor->exported_route, route) &&
         same(descriptor->contract_version, version) &&
         descriptor->content_sha256[0] == digest;
}

static void TTX_CALL resolve_terminal_requirement(
    ttx_host_requirements_self*,
    const ttx_requirement_descriptor* descriptor,
    ttx_requirement_result result) {
  if (!descriptor_matches(
          descriptor, "Tetrodotoxin.Terminal", "Provider", "1.0", 0x54)) {
    result.operations->missing(result.self);
    return;
  }
  result.operations->resolved(result.self, tetrodotoxin_terminal_requirement());
}

PERIMORTEM_UNIT_TEST(PluginLoader, graph_text_uses_host_semantic_abi) {
  const ttx_host_requirements_ops host_operations = {
    .header =
        {
          .size = sizeof(ttx_host_requirements_ops),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .resolve = resolve_terminal_requirement,
  };
  Plugin::LoadFailure failure = Plugin::LoadFailure::InvalidProtocol;
  auto loaded = Plugin::Loaded::open(
      graph_text_plugin_path(),
      {
        .operations = &host_operations,
        .self = reinterpret_cast<ttx_host_requirements_self*>(
            const_cast<ttx_host_requirements_ops*>(&host_operations)),
      },
      failure);
  ASSERT(loaded);

  std::vector<Plugin::ProviderDescriptor> providers;
  ASSERT(loaded->providers(providers));
  ASSERT_EQ(providers.size(), size_t(1));
  EXPECT(providers[0].package_coordinate == "Tetrodotoxin.GraphText");
  EXPECT(providers[0].exported_route == "Graph");
  ASSERT(providers[0].terminal.operations != nullptr);
  EXPECT_EQ(
      Ttx::relation(
          providers[0].candidate, tetrodotoxin_terminal_requirement()),
      TTX_INTERFACE_SATISFIED);

  Environment::Toolchain toolchain;
  Environment::Workspace workspace(toolchain);
  static constexpr uint8_t route[] = "Graph.graph";
  auto begun = Terminal::begin(
      providers[0].terminal, {.data = route, .size = sizeof(route) - 1},
      ttx_unknown(), workspace.get_provider_handle(), ttx_unknown(),
      ttx_unknown());
  ASSERT(begun.state == Terminal::BeginState::Requested);
  auto observed = Terminal::observe(begun.request);
  ASSERT(observed.state == Terminal::ProductState::Produced);

  char root[] = "/tmp/graph-text-plugin-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  Perimortem::Core::View::Bytes root_view =
      Perimortem::Core::NullTerminated::to_view(root);
  ASSERT(
      Puffer::Publisher(root_view, "Example/1.0"_view)
          .publish(observed.products));
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Bytes output_path(arena, root_view);
  output_path.concat("/Example/1.0/Graph.graph"_view);
  auto output = Perimortem::System::File::read(output_path.get_view());
  ASSERT(output && output->get_size() >= Count(11));
  EXPECT_TEXT(output->get_view().slice(0, 11), "ttx.graph 2"_view);

  begun.request.operations->release(begun.request.self);
  EXPECT(loaded->close());
  nftw(root, remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}

PERIMORTEM_UNIT_TEST(PluginLoader, loads_typed_terminal_provider) {
  Ttx::Requirement requirement("Echo"_bytes);
  HostRequirement host = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_host_requirements_ops),
                .abi_major = TTX_PLUGIN_ABI_MAJOR,
                .abi_minor = TTX_PLUGIN_ABI_MINOR,
              },
          .resolve = resolve_requirement,
        },
    .requirement = requirement.get_abi(),
    .resolved = false,
  };
  Plugin::LoadFailure failure = Plugin::LoadFailure::InvalidProtocol;
  auto loaded = Plugin::Loaded::open(
      plugin_path(),
      {
        .operations = &host.operations,
        .self = reinterpret_cast<ttx_host_requirements_self*>(&host),
      },
      failure);
  ASSERT(loaded);

  std::vector<Plugin::ProviderDescriptor> providers;
  ASSERT(loaded->providers(providers));
  ASSERT_EQ(providers.size(), size_t(3));
  EXPECT(providers[1].package_coordinate == "Validation.Plugin");
  EXPECT(providers[1].exported_route == "TerminalProvider");
  EXPECT(providers[1].terminal.operations != nullptr);
  EXPECT(ttx_abstract_same(
      providers[1].candidate,
      providers[1].terminal.operations->candidate(providers[1].terminal.self)));
  EXPECT_EQ(
      Ttx::relation(providers[1].candidate, requirement.get_abi()),
      TTX_INTERFACE_SATISFIED);
  EXPECT(loaded->close());
}

PERIMORTEM_UNIT_TEST(PluginLoader, loads_foreign_provider) {
  Ttx::Requirement requirement("Echo"_bytes);
  HostRequirement host = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_host_requirements_ops),
                .abi_major = TTX_PLUGIN_ABI_MAJOR,
                .abi_minor = TTX_PLUGIN_ABI_MINOR,
              },
          .resolve = resolve_requirement,
        },
    .requirement = requirement.get_abi(),
    .resolved = false,
  };
  Plugin::LoadFailure failure = Plugin::LoadFailure::InvalidProtocol;
  auto loaded = Plugin::Loaded::open(
      plugin_path(),
      {
        .operations = &host.operations,
        .self = reinterpret_cast<ttx_host_requirements_self*>(&host),
      },
      failure);

  ASSERT(loaded);
  EXPECT(host.resolved);

  std::vector<Plugin::ProviderDescriptor> providers;
  ASSERT(loaded->providers(providers));
  ASSERT_EQ(providers.size(), size_t(3));
  EXPECT(providers[0].package_coordinate == "Validation.Plugin");
  EXPECT(providers[0].exported_route == "EchoProvider");
  EXPECT(providers[0].export_version == "1");
  EXPECT_EQ(providers[0].content_sha256[0], uint8_t(2));
  EXPECT(same(
      providers[0].candidate.operations->name(providers[0].candidate),
      "C plugin provider"));
  EXPECT_EQ(
      Ttx::relation(providers[0].candidate, requirement.get_abi()),
      TTX_INTERFACE_SATISFIED);

  std::vector<Plugin::RequirementDescriptor> dependencies;
  ASSERT(loaded->dependencies(providers[0].candidate, dependencies));
  ASSERT_EQ(dependencies.size(), size_t(1));
  EXPECT(dependencies[0].package_coordinate == "Validation.Contracts");
  EXPECT(dependencies[0].exported_route == "Echo");
  EXPECT(dependencies[0].contract_version == "1");
  EXPECT_EQ(dependencies[0].content_sha256[0], uint8_t(1));
  EXPECT(loaded->close());
}

PERIMORTEM_UNIT_TEST(PluginLoader, loads_typed_dialect_provider) {
  Ttx::Requirement requirement("Echo"_bytes);
  HostRequirement host = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_host_requirements_ops),
                .abi_major = TTX_PLUGIN_ABI_MAJOR,
                .abi_minor = TTX_PLUGIN_ABI_MINOR,
              },
          .resolve = resolve_requirement,
        },
    .requirement = requirement.get_abi(),
    .resolved = false,
  };
  Plugin::LoadFailure failure = Plugin::LoadFailure::InvalidProtocol;
  auto loaded = Plugin::Loaded::open(
      plugin_path(),
      {
        .operations = &host.operations,
        .self = reinterpret_cast<ttx_host_requirements_self*>(&host),
      },
      failure);
  ASSERT(loaded);

  std::vector<Plugin::ProviderDescriptor> providers;
  ASSERT(loaded->providers(providers));
  ASSERT_EQ(providers.size(), size_t(3));
  const Plugin::ProviderDescriptor& descriptor = providers[2];
  EXPECT(descriptor.package_coordinate == "Validation.Plugin");
  EXPECT(descriptor.exported_route == "DialectProvider");
  ASSERT(descriptor.dialect.operations != nullptr);
  EXPECT(ttx_abstract_same(
      descriptor.candidate,
      descriptor.dialect.operations->candidate(descriptor.dialect.self)));
  EXPECT_EQ(
      Ttx::relation(descriptor.candidate, tetrodotoxin_dialect_requirement()),
      TTX_INTERFACE_SATISFIED);

  {
    Environment::Toolchain toolchain;
    ASSERT(toolchain.install(descriptor.dialect));
    EXPECT_NOT(toolchain.install(descriptor.dialect));
    const auto installed = toolchain.find_provider("Fixture"_view);
    ASSERT(installed);

    SourceInput input = fixture_source("fixture");
    const auto interpreted = Language::interpret(
        *installed,
        {
          .operations = &input.operations,
          .self = reinterpret_cast<tetrodotoxin_source_input_self*>(&input),
        },
        ttx_unknown());
    ASSERT(interpreted.state == Language::InterpretationState::Constructed);
    ASSERT(interpreted.graph.operations != nullptr);
    EXPECT(ttx_abstract_same(
        descriptor.candidate,
        interpreted.graph.operations->root(interpreted.graph.self)));
    EXPECT(ttx_abstract_same(
        descriptor.candidate,
        interpreted.graph.operations->dialect(interpreted.graph.self)));
    const auto validation = Language::validate(interpreted.graph);
    EXPECT(validation.state == Language::ValidationState::Accepted);
    const ttx_context context = ttx_context_create();
    ASSERT(context.operations != nullptr);
    const auto production = Language::produce(interpreted.graph, context);
    EXPECT(production.state == Language::ProductionState::None);
    context.operations->release(context);
    interpreted.graph.operations->release(interpreted.graph.self);

    Environment::Workspace workspace(toolchain);
    const auto retained = workspace.interpret_source(
        *installed, "Portable"_view, "portable.fixture"_view, "fixture"_view);
    ASSERT(
        retained.state ==
        Environment::Workspace::ProviderSourceState::Accepted);
    EXPECT(ttx_abstract_same(retained.root, descriptor.candidate));
    const ttx_context retained_context = ttx_context_create();
    ASSERT(retained_context.operations != nullptr);
    Perimortem::Memory::Allocator::Arena product_arena;
    const auto retained_product =
        workspace.produce(retained.root, retained_context, product_arena);
    EXPECT(retained_product.state == Language::ProductionState::None);
    retained_context.operations->release(retained_context);
    const ttx_abstract selected = Ttx::resolve_concept(
        workspace.get_handle(),
        {
          .data = reinterpret_cast<const uint8_t*>("Portable"),
          .size = 8,
        });
    EXPECT(ttx_abstract_same(selected, descriptor.candidate));
    const auto graph_text = Terminal::GraphText::write(
        {
          .data = reinterpret_cast<const uint8_t*>("fixture"),
          .size = 7,
        },
        descriptor.candidate, retained.root, workspace.get_handle());
    EXPECT(
        std::string(graph_text.begin(), graph_text.end())
            .find("name x4320706c7567696e2070726f7669646572") !=
        std::string::npos);
  }
  EXPECT(loaded->close());
}

PERIMORTEM_UNIT_TEST(PluginLoader, environment_retains_exact_export) {
  Ttx::Requirement requirement("Echo"_bytes);
  Plugin::RequirementDescriptor descriptor = {
    .package_coordinate = "Validation.Contracts",
    .exported_route = "Echo",
    .contract_version = "1",
    .content_sha256 = {},
  };
  descriptor.content_sha256[0] = 1;
  Environment::ProviderClosure closure({
    {
      .descriptor = descriptor,
      .identity = requirement.get_abi(),
    },
  });
  Environment::ProviderAdmissionFailure failure =
      Environment::ProviderAdmissionFailure::InvalidProtocol;
  const std::string path = plugin_path();
  ASSERT(closure.admit(path, failure));

  const auto providers = closure.providers(path);
  ASSERT_EQ(providers.size(), size_t(3));
  const auto selected = closure.select(path, providers[0]);
  ASSERT(selected);
  EXPECT_EQ(
      Ttx::relation(*selected, requirement.get_abi()), TTX_INTERFACE_SATISFIED);

  Plugin::ProviderDescriptor wrong = providers[0];
  wrong.content_sha256[0] ^= 0xFF;
  EXPECT_NOT(closure.select(path, wrong));

  const auto dialect = closure.select_dialect(path, providers[2]);
  ASSERT(dialect);
  EXPECT(ttx_abstract_same(
      providers[2].candidate, dialect->operations->candidate(dialect->self)));
  const auto by_route = closure.find_dialect(path, "DialectProvider");
  ASSERT(by_route);
  EXPECT(ttx_abstract_same(
      providers[2].candidate, by_route->operations->candidate(by_route->self)));
  EXPECT_NOT(closure.find_dialect(path, "Missing"));
}

PERIMORTEM_UNIT_TEST(PluginLoader, environment_selects_typed_terminal) {
  Plugin::RequirementDescriptor descriptor = {
    .package_coordinate = "Tetrodotoxin.Terminal",
    .exported_route = "Provider",
    .contract_version = "1.0",
    .content_sha256 = {},
  };
  descriptor.content_sha256[0] = 0x54;
  Environment::ProviderClosure closure({
    {
      .descriptor = descriptor,
      .identity = tetrodotoxin_terminal_requirement(),
    },
  });
  Environment::ProviderAdmissionFailure failure =
      Environment::ProviderAdmissionFailure::InvalidProtocol;
  const std::string path = graph_text_plugin_path();
  ASSERT(closure.admit(path, failure));

  auto providers = closure.providers(path);
  ASSERT_EQ(providers.size(), size_t(1));
  auto terminal = closure.select_terminal(path, providers[0]);
  ASSERT(terminal);
  EXPECT(ttx_abstract_same(
      providers[0].candidate, terminal->operations->candidate(terminal->self)));
  auto by_route = closure.find_terminal(path, "Graph");
  ASSERT(by_route);
  EXPECT(ttx_abstract_same(
      providers[0].candidate, by_route->operations->candidate(by_route->self)));
  EXPECT_NOT(closure.find_terminal(path, "Missing"));

  Plugin::ProviderDescriptor wrong = providers[0];
  wrong.content_sha256[0] ^= 0xFF;
  EXPECT_NOT(closure.select_terminal(path, wrong));
}
