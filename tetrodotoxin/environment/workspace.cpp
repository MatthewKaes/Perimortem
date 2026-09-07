// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/workspace.hpp"

#include "ttx/lexical/tokenizer.hpp"

using namespace Tetrodotoxin;
using namespace Perimortem;

static auto text(Core::View::Bytes value) -> std::string {
  return value.is_empty() ? std::string()
                          : std::string(
                                reinterpret_cast<const char*>(value.get_data()),
                                value.get_size());
}

// Input owns only source bytes. Each provider chooses how to interpret them
// and retain its graph. Token visitation is optional lexical assistance, so a
// native provider can tokenize directly without paying for a second stream.
struct SourceInput {
  std::string path;
  std::string contents;
  size_t references = 1;
};

static auto input(tetrodotoxin_source_input_self* self) -> SourceInput& {
  return *reinterpret_cast<SourceInput*>(self);
}

static void TTX_CALL input_retain(tetrodotoxin_source_input_self* self) {
  ++input(self).references;
}

static void TTX_CALL input_release(tetrodotoxin_source_input_self* self) {
  auto& source = input(self);
  if (--source.references == 0) {
    delete &source;
  }
}

static auto borrowed_bytes(const std::string& value) -> ttx_borrowed_bytes {
  return {reinterpret_cast<const uint8_t*>(value.data()), value.size()};
}

static void TTX_CALL input_tokens(
    tetrodotoxin_source_input_self* self,
    tetrodotoxin_token_sink sink) {
  Memory::Allocator::Arena arena;
  const auto source = borrowed_bytes(input(self).contents);
  const auto path = borrowed_bytes(input(self).path);
  Ttx::Lexical::Tokenizer tokenizer(
      arena, {source.data, source.size}, {path.data, path.size});
  for (const auto token : tokenizer.get_tokens()) {
    sink.operations->token(
        sink.self, {
                     token.get_offset(),
                     token.get_line(),
                     token.get_column(),
                     token.get_size(),
                     static_cast<uint8_t>(token.get_code().get_type()),
                   });
  }
  sink.operations->completed(sink.self);
}

static auto source_input(Core::View::Bytes path, Core::View::Bytes contents)
    -> tetrodotoxin_source_input {
  static const tetrodotoxin_source_input_ops operations = {
    .header =
        {sizeof(tetrodotoxin_source_input_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .diagnostic_path =
        [](tetrodotoxin_source_input_self* self) {
          return borrowed_bytes(input(self).path);
        },
    .bytes =
        [](tetrodotoxin_source_input_self* self) {
          return borrowed_bytes(input(self).contents);
        },
    .visit_tokens = input_tokens,
    .retain = input_retain,
    .release = input_release,
  };
  auto* owner = new SourceInput{text(path), text(contents)};
  return {
    &operations, reinterpret_cast<tetrodotoxin_source_input_self*>(owner)};
}

Environment::Workspace::Source::Source(Core::View::Bytes name)
    : name(text(name)) {}

Environment::Workspace::~Workspace() {
  // Keep the routing authorities alive until every provider has released its
  // current graph. Teardown may then observe an empty neighbor without finding
  // a dangling authority halfway through destruction of the source inventory.
  for (auto& source : sources) {
    source->graph = {};
  }
}

auto Environment::Workspace::Source::get_name() const -> Core::View::Bytes {
  const auto value = borrowed_bytes(name);
  return {value.data, value.size};
}

auto Environment::Workspace::Source::resolve(ttx_abstract) const
    -> ttx_abstract {
  return graph.root();
}

auto Environment::Workspace::Source::resolve_concept(
    ttx_borrowed_bytes route) const -> ttx_abstract {
  return Ttx::resolve_concept(graph.root(), route);
}

void Environment::Workspace::Source::visit_concepts(
    ttx_concept_sink sink) const {
  const auto root = graph.root();
  root->operations->visit_concepts(root, sink);
}

auto Environment::Workspace::find(Core::View::Bytes name) const -> Source* {
  for (const auto& source : sources) {
    if (source->get_name() == name) {
      return source.get();
    }
  }
  return nullptr;
}

auto Environment::Workspace::source_authority(Core::View::Bytes name)
    -> ttx_abstract {
  auto* existing = find(name);
  if (existing) {
    return existing->get_abi();
  }
  auto source = std::make_unique<Source>(name);
  const auto identity = source->get_abi();
  sources.push_back(std::move(source));
  return identity;
}

struct Acquisition {
  Environment::Workspace& workspace;
  bool complete = false;
  bool valid = true;
};

struct DependencyAdmission {
  bool answered = false;
  bool accepted = false;
};

static void TTX_CALL
    dependency_acquired(tetrodotoxin_dependency_result_self* self) {
  auto& result = *reinterpret_cast<DependencyAdmission*>(self);
  result.accepted = !result.answered;
  result.answered = true;
}
static void TTX_CALL
    dependency_rejected(tetrodotoxin_dependency_result_self* self) {
  auto& result = *reinterpret_cast<DependencyAdmission*>(self);
  result.answered = true;
  result.accepted = false;
}
static void TTX_CALL
    dependency_failed(tetrodotoxin_dependency_result_self* self, ttx_abstract) {
  dependency_rejected(self);
}

static void TTX_CALL acquire_source(
    tetrodotoxin_source_dependency_sink_self* self,
    tetrodotoxin_source_dependency dependency) {
  auto& acquisition = *reinterpret_cast<Acquisition*>(self);
  const auto* operations = dependency.operations;
  if (acquisition.complete || !operations || !dependency.self ||
      operations->header.abi_major != TTX_ABI_MAJOR ||
      operations->header.size < sizeof(tetrodotoxin_source_dependency_ops) ||
      !operations->kind || !operations->locator || !operations->acquire) {
    acquisition.valid = false;
    return;
  }
  // A source import retains the destination authority even before that source
  // is available. Acquiring bytes or package coordinates is the host's input
  // responsibility, while graph validation can already report the missing edge.
  if (dependency.operations->kind(dependency.self) !=
      TETRODOTOXIN_DEPENDENCY_SOURCE) {
    return;
  }
  const auto locator = dependency.operations->locator(dependency.self);
  const auto target =
      acquisition.workspace.source_authority({locator.data, locator.size});
  static const tetrodotoxin_dependency_result_ops result_operations = {
    .header =
        {sizeof(tetrodotoxin_dependency_result_ops), TTX_ABI_MAJOR,
         TTX_ABI_MINOR},
    .acquired = dependency_acquired,
    .rejected = dependency_rejected,
    .failed = dependency_failed,
  };
  DependencyAdmission admission;
  dependency.operations->acquire(
      dependency.self, target,
      {&result_operations,
       reinterpret_cast<tetrodotoxin_dependency_result_self*>(&admission)});
  acquisition.valid &= admission.answered && admission.accepted;
}

static void TTX_CALL
    acquired_sources(tetrodotoxin_source_dependency_sink_self* self) {
  auto& acquisition = *reinterpret_cast<Acquisition*>(self);
  acquisition.valid &= !acquisition.complete;
  acquisition.complete = true;
}

static void TTX_CALL failed_sources(
    tetrodotoxin_source_dependency_sink_self* self,
    ttx_abstract) {
  reinterpret_cast<Acquisition*>(self)->valid = false;
}

auto Environment::Workspace::interpret_source(
    tetrodotoxin_dialect_provider provider,
    Core::View::Bytes name,
    Core::View::Bytes path,
    Core::View::Bytes contents) -> Language::InterpretationState {
  if (name.is_empty() || !toolchain.is_installed(provider)) {
    return Language::InterpretationState::Invalid;
  }
  const auto input = source_input(path, contents);
  auto interpreted = Language::interpret(provider, input, get_abi());
  input.operations->release(input.self);
  if (interpreted.state != Language::InterpretationState::Constructed) {
    return interpreted.state;
  }
  auto graph = Language::SourceGraph::adopt(interpreted.graph);
  const auto root = graph.root();
  if (!root || !root->operations ||
      root->operations->header.abi_major != TTX_ABI_MAJOR ||
      root->operations->header.size < sizeof(ttx_abstract_ops)) {
    return Language::InterpretationState::Invalid;
  }
  if (!ttx_abstract_same(
          graph.get().operations->dialect(graph.get().self),
          provider.operations->candidate(provider.self))) {
    return Language::InterpretationState::Invalid;
  }

  source_authority(name);
  auto* slot = find(name);
  Acquisition acquisition{*this};
  static const tetrodotoxin_source_dependency_sink_ops operations = {
    .header =
        {sizeof(tetrodotoxin_source_dependency_sink_ops), TTX_ABI_MAJOR,
         TTX_ABI_MINOR},
    .dependency = acquire_source,
    .completed = acquired_sources,
    .failed = failed_sources,
  };
  const auto current = graph.get();
  current.operations->visit_dependencies(
      current.self,
      {&operations, reinterpret_cast<tetrodotoxin_source_dependency_sink_self*>(
                        &acquisition)});
  if (!acquisition.complete || !acquisition.valid) {
    return Language::InterpretationState::Invalid;
  }
  // Interpretation and acquisition finish before publication. Old generations
  // are released here unless an observer retained them beside its borrowed
  // flow.
  slot->graph = std::move(graph);
  ++slot->revision;
  return Language::InterpretationState::Constructed;
}

auto Environment::Workspace::observe_source(Core::View::Bytes name) const
    -> Language::SourceGraph {
  const auto* source = find(name);
  return source ? source->graph : Language::SourceGraph();
}

auto Environment::Workspace::retain_sources() const
    -> std::vector<Language::SourceGraph> {
  std::vector<Language::SourceGraph> closure;
  closure.reserve(sources.size());
  for (const auto& source : sources) {
    if (source->graph) {
      closure.push_back(source->graph);
    }
  }
  return closure;
}

auto Environment::Workspace::source_revision(Core::View::Bytes name) const
    -> uint64_t {
  const auto* source = find(name);
  return source ? source->revision : 0;
}

auto Environment::Workspace::resolve_concept(ttx_borrowed_bytes route) const
    -> ttx_abstract {
  const auto* source = find({route.data, route.size});
  return source ? source->get_abi() : ttx_unknown();
}

void Environment::Workspace::visit_concepts(ttx_concept_sink sink) const {
  for (const auto& source : sources) {
    const auto name = source->get_name();
    sink.operations->item(
        sink, {name.get_data(), name.get_size()}, source->get_abi());
  }
  sink.operations->completed(sink);
}
