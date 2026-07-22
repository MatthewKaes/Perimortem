// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/benchmark.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/archiver/reader.hpp"
#include "tetrodotoxin/archiver/writer.hpp"
#include "tetrodotoxin/model/addressables/field.hpp"
#include "tetrodotoxin/model/addressables/parameter.hpp"
#include "tetrodotoxin/model/apps/lifecycle.hpp"
#include "tetrodotoxin/model/callables/static.hpp"
#include "tetrodotoxin/model/constants/flag.hpp"
#include "tetrodotoxin/model/interfaces/located.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
#include "tetrodotoxin/model/renderables/constant.hpp"
#include "tetrodotoxin/model/renderables/push.hpp"
#include "tetrodotoxin/model/renderables/resource.hpp"
#include "tetrodotoxin/model/renderables/value.hpp"
#include "tetrodotoxin/model/renders/contract.hpp"
#include "tetrodotoxin/model/shaders/program.hpp"
#include "tetrodotoxin/model/stages/implemented.hpp"
#include "tetrodotoxin/model/stages/required.hpp"
#include "tetrodotoxin/model/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/bodies/builder.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/types/boolean.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Ttx::Concept;
using namespace Validation;

class Shape {
 public:
  Count types = 0;
  Count fields = 0;
  Count depth = 0;
  Count callables = 0;
  Count body_operations = 0;
  Count dependencies = 0;
  Count cross_edges = 0;
  Count render_facts = 0;
  Count shader_stages = 0;
  Count interfaces = 0;
  Count terminals = 0;
  Count terminal_bytes = 0;
};

class SemanticArchiveFixture {
 public:
  explicit SemanticArchiveFixture(Shape shape)
      : root(arena, {}),
        definitions(arena),
        dependencies(arena),
        manifest_dependencies(arena),
        terminals(arena) {
    boolean = &arena.construct<Ttx::Model::Types::Boolean>();
    truth =
        &arena.construct<Tetrodotoxin::Model::Constants::Flag>(*boolean, True);
    retain(*boolean);
    retain(*truth);
    build_dependencies(shape.dependencies);
    build_types(shape.types);
    build_fields(shape.fields);
    build_depth(shape.depth);
    build_callables(shape.callables);
    build_body(shape.body_operations);
    build_cross_edges(shape.cross_edges);
    build_render_shader(
        shape.render_facts, shape.shader_stages, shape.interfaces);
    build_terminals(shape.terminals, shape.terminal_bytes);

    package = &arena.construct<Tetrodotoxin::Model::Packages::Precompiled>(
        arena, root, dependencies.get_view(), definitions.get_view(),
        terminals.get_view());
    manifest = &arena.construct<Manifest>(
        "Benchmark.Semantic"_view, Version(1, 0),
        manifest_dependencies.get_view());
    View::Bytes written = Tetrodotoxin::Archiver::Writer::write(
        arena, *manifest, *package, package->get_terminals());
    if (written.is_empty()) {
      abort();
    }
    buffer = written;
  }

  auto get_buffer() const -> View::Bytes { return buffer; }
  auto get_manifest() const -> const Manifest& { return *manifest; }
  auto get_package() const -> const Tetrodotoxin::Model::Package& {
    return *package;
  }
  auto get_dependencies() const
      -> View::Vector<Reference<Tetrodotoxin::Model::Package>> {
    return dependencies;
  }

 private:
  auto name(const char* prefix, Count index) -> View::Bytes {
    Static::Bytes<64> text;
    int written = snprintf(
        Data::cast<char>(text.get_data()), text.get_size(), "%s%06llu", prefix,
        (unsigned long long)index);
    if (written <= 0 || Count(written) >= text.get_size()) {
      abort();
    }
    return arena.proxy(View::Bytes(text.get_data(), Count(written)));
  }

  auto retain(const Abstract& definition) -> void {
    definitions.insert(Reference<Abstract>(definition));
  }

  auto publish(const Abstract& definition) -> void {
    retain(definition);
    Bool exported = root.add_export(definition);
    if (!exported) {
      abort();
    }
  }

  auto build_dependencies(Count count) -> void {
    dependencies.reset(count);
    manifest_dependencies.reset(count);
    for (Count i = 0; i < count; i++) {
      auto& dependency_type = arena.construct<Ttx::Model::Types::Boolean>();
      auto& dependency_root =
          arena.construct<Tetrodotoxin::Model::Namespace>(arena, View::Bytes());
      const Static::Vector<Reference<Abstract>, 1> dependency_definitions = {{
        dependency_type,
      }};
      auto& dependency =
          arena.construct<Tetrodotoxin::Model::Packages::Precompiled>(
              arena, dependency_root,
              View::Vector<Reference<Tetrodotoxin::Model::Package>>(),
              dependency_definitions);
      dependencies.insert(Reference<Tetrodotoxin::Model::Package>(dependency));
      manifest_dependencies.insert(
          Dependency(name("Benchmark.Dependency", i), Version(1, 0)));
    }
  }

  auto build_types(Count count) -> void {
    for (Count i = 0; i < count; i++) {
      auto& field = arena.construct<Tetrodotoxin::Model::Addressables::Field>(
          name("value", i), *boolean);
      auto& type = arena.construct<Tetrodotoxin::Model::Types::Structure>(
          arena, name("Type", i), Documentation::get_empty());
      Bool added = type.add_field(field, True);
      if (!added) {
        abort();
      }
      Bool completed = type.complete();
      if (!completed) {
        abort();
      }
      publish(type);
      retain(field);
    }
  }

  auto build_fields(Count count) -> void {
    if (count == 0) {
      return;
    }
    auto& type = arena.construct<Tetrodotoxin::Model::Types::Structure>(
        arena, "FieldOwner"_view, Documentation::get_empty());
    publish(type);
    for (Count i = 0; i < count; i++) {
      auto& field = arena.construct<Tetrodotoxin::Model::Addressables::Field>(
          name("field", i), *boolean);
      Bool added = type.add_field(field, True);
      if (!added) {
        abort();
      }
      retain(field);
    }
    Bool completed = type.complete();
    if (!completed) {
      abort();
    }
  }

  auto build_depth(Count count) -> void {
    const Ttx::Model::Type* child = boolean;
    for (Count i = 0; i < count; i++) {
      auto& field = arena.construct<Tetrodotoxin::Model::Addressables::Field>(
          "child"_view, *child);
      auto& type = arena.construct<Tetrodotoxin::Model::Types::Structure>(
          arena, name("Depth", i), Documentation::get_empty());
      Bool added = type.add_field(field, True);
      if (!added) {
        abort();
      }
      Bool completed = type.complete();
      if (!completed) {
        abort();
      }
      retain(type);
      retain(field);
      child = &type;
    }
    if (count != 0) {
      Bool exported = root.add_export(*child);
      if (!exported) {
        abort();
      }
    }
  }

  auto build_callables(Count count) -> void {
    for (Count i = 0; i < count; i++) {
      auto& parameter =
          arena.construct<Tetrodotoxin::Model::Addressables::Parameter>(
              "value"_view, *boolean);
      const Static::Vector<Reference<Ttx::Model::Addressable>, 1> parameters = {
        {
          parameter,
        }};
      auto& callable = arena.construct<Tetrodotoxin::Model::Callables::Static>(
          arena, name("Callable", i), parameters,
          View::Vector<Reference<Abstract>>(), Documentation::get_empty());
      publish(callable);
      retain(parameter);
    }
  }

  auto build_body(Count count) -> void {
    if (count == 0) {
      return;
    }
    Ttx::Model::Layouts::Fluid empty;
    Ttx::Model::Bodies::Builder builder(arena, empty, empty);
    for (Count i = 0; i < count; i++) {
      Ttx::Model::Bodies::ValueId value = builder.constant(*truth);
      if (!value.is_valid()) {
        abort();
      }

      if (i + 1 < count) {
        Ttx::Model::Bodies::BlockId next(Unsigned_32(i + 1));
        Bool jumped = builder.jump(next);
        if (!jumped) {
          abort();
        }
        Ttx::Model::Bodies::BlockId begun = builder.begin_block();
        if (begun != next) {
          abort();
        }
      }
    }
    Bool returned = builder.return_values();
    if (!returned) {
      abort();
    }
    Ttx::Model::Body body = builder.finish();
    if (!body.is_valid()) {
      abort();
    }
    auto& lifecycle = arena.construct<Tetrodotoxin::Model::Apps::Lifecycle>(
        arena, "Body"_view, View::Vector<Reference<Ttx::Model::Addressable>>(),
        View::Vector<Reference<Abstract>>(), body, Documentation::get_empty());
    publish(lifecycle);
  }

  auto build_cross_edges(Count count) -> void {
    if (count == 0 || dependencies.is_empty()) {
      return;
    }
    auto& type = arena.construct<Tetrodotoxin::Model::Types::Structure>(
        arena, "CrossEdges"_view, Documentation::get_empty());
    publish(type);
    for (Count i = 0; i < count; i++) {
      const auto& dependency = dependencies[i % dependencies.get_size()].get();
      const auto& target =
          dependency.get_definition(0).assume<Ttx::Model::Type>();
      auto& field = arena.construct<Tetrodotoxin::Model::Addressables::Field>(
          name("external", i), target);
      Bool added = type.add_field(field, True);
      if (!added) {
        abort();
      }
      retain(field);
    }
    Bool completed = type.complete();
    if (!completed) {
      abort();
    }
  }

  auto build_render_shader(
      Count fact_count,
      Count stage_count,
      Count interface_count) -> void {
    if (fact_count == 0 && stage_count == 0 && interface_count == 0) {
      return;
    }
    Managed::Vector<Reference<Tetrodotoxin::Model::Renderables::Value>> values(
        arena);
    auto& constants =
        arena.construct<Tetrodotoxin::Model::Namespace>(arena, "constant"_view);
    auto& pushes =
        arena.construct<Tetrodotoxin::Model::Namespace>(arena, "push"_view);
    auto& resources =
        arena.construct<Tetrodotoxin::Model::Namespace>(arena, "resource"_view);
    retain(constants);
    retain(pushes);
    retain(resources);
    for (Count i = 0; i < fact_count; i++) {
      auto& value = arena.construct<Tetrodotoxin::Model::Renderables::Value>(
          name("value", i), *boolean, *truth, Documentation::get_empty());
      auto& source = arena.construct<Tetrodotoxin::Model::Addressables::Field>(
          name("source", i), *boolean);
      auto& constant =
          arena.construct<Tetrodotoxin::Model::Renderables::Constant>(
              name("constant", i), *truth, Documentation::get_empty());
      auto& push = arena.construct<Tetrodotoxin::Model::Renderables::Push>(
          name("push", i), source, Documentation::get_empty());
      auto& resource =
          arena.construct<Tetrodotoxin::Model::Renderables::Resource>(
              name("resource", i), source, 0, Unsigned_32(i),
              Documentation::get_empty());
      Bool constant_exported = constants.add_export(constant);
      Bool push_exported = pushes.add_export(push);
      Bool resource_exported = resources.add_export(resource);
      if (!constant_exported || !push_exported || !resource_exported) {
        abort();
      }
      values.insert(Reference<Tetrodotoxin::Model::Renderables::Value>(value));
      retain(value);
      retain(source);
      retain(constant);
      retain(push);
      retain(resource);
    }

    Count actual_stages = Math::max(stage_count, Count(interface_count != 0));
    Managed::Vector<Reference<Tetrodotoxin::Model::Stages::Required>> required(
        arena);
    Managed::Vector<Reference<Tetrodotoxin::Model::Stages::Implemented>>
        implemented(arena);
    for (Count stage = 0; stage < actual_stages; stage++) {
      Managed::Vector<Reference<Ttx::Model::Addressable>> parameters(arena);
      for (Count i = 0; i < interface_count; i++) {
        auto& parameter =
            arena.construct<Tetrodotoxin::Model::Interfaces::Located>(
                name("input", stage * interface_count + i), *boolean,
                Unsigned_32(i));
        parameters.insert(Reference<Ttx::Model::Addressable>(parameter));
        retain(parameter);
      }
      auto& contract = arena.construct<Tetrodotoxin::Model::Stages::Required>(
          arena, name("stage", stage),
          stage == 0
              ? Tetrodotoxin::Model::Stages::Required::Execution::Vertex
              : Tetrodotoxin::Model::Stages::Required::Execution::Fragment,
          parameters.get_view(),
          View::Vector<Reference<Ttx::Model::Addressable>>(),
          View::Vector<Reference<Ttx::Model::Addressable>>(),
          Documentation::get_empty());
      Ttx::Model::Bodies::Builder builder(
          arena, contract.get_parameters(), contract.get_results());
      Bool returned = builder.return_values();
      if (!returned) {
        abort();
      }
      Ttx::Model::Body body = builder.finish();
      auto& stage_owner =
          arena.construct<Tetrodotoxin::Model::Stages::Implemented>(
              arena, contract, parameters.get_view(),
              View::Vector<Reference<Ttx::Model::Addressable>>(), body,
              Documentation::get_empty());
      required.insert(
          Reference<Tetrodotoxin::Model::Stages::Required>(contract));
      implemented.insert(
          Reference<Tetrodotoxin::Model::Stages::Implemented>(stage_owner));
      retain(contract);
      retain(stage_owner);
    }
    const Abstract& render = Tetrodotoxin::Model::Renders::Contract::construct(
        arena, "Render"_view, values.get_view(), constants, pushes, resources,
        required.get_view(), Documentation::get_empty());
    if (!render.is<Tetrodotoxin::Model::Render>()) {
      abort();
    }
    publish(render);
    if (actual_stages != 0) {
      const Abstract& shader = Tetrodotoxin::Model::Shaders::Program::construct(
          arena, "Shader"_view, render.assume<Tetrodotoxin::Model::Render>(),
          implemented.get_view(), Documentation::get_empty());
      if (!shader.is<Tetrodotoxin::Model::Shader>()) {
        abort();
      }
      publish(shader);
    }
  }

  auto build_terminals(Count count, Count bytes) -> void {
    View::Bytes content;
    if (bytes != 0) {
      Unsigned_8* storage = arena.allocate(bytes);
      for (Count i = 0; i < bytes; i++) {
        storage[i] = Unsigned_8(i * 29 + 7);
      }
      content = View::Bytes(storage, bytes);
    }
    terminals.reset(count);
    for (Count i = 0; i < count; i++) {
      terminals.insert(
          Tetrodotoxin::Model::Terminal(arena, name("terminal", i), content));
    }
  }

  Allocator::Arena arena;
  Tetrodotoxin::Model::Namespace root;
  Managed::Vector<Reference<Abstract>> definitions;
  Managed::Vector<Reference<Tetrodotoxin::Model::Package>> dependencies;
  Managed::Vector<Dependency> manifest_dependencies;
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals;
  const Ttx::Model::Types::Boolean* boolean = nullptr;
  const Tetrodotoxin::Model::Constants::Flag* truth = nullptr;
  const Tetrodotoxin::Model::Packages::Precompiled* package = nullptr;
  const Manifest* manifest = nullptr;
  Dynamic::Bytes buffer;
};

static SemanticArchiveFixture* active = nullptr;
static Unsigned_8* active_storage = nullptr;

template <
    Count types,
    Count fields,
    Count depth,
    Count callables,
    Count body_operations,
    Count dependencies,
    Count cross_edges,
    Count render_facts,
    Count shader_stages,
    Count interfaces,
    Count terminals,
    Count terminal_bytes>
static auto setup_semantic() -> void {
  Shape shape;
  shape.types = types;
  shape.fields = fields;
  shape.depth = depth;
  shape.callables = callables;
  shape.body_operations = body_operations;
  shape.dependencies = dependencies;
  shape.cross_edges = cross_edges;
  shape.render_facts = render_facts;
  shape.shader_stages = shader_stages;
  shape.interfaces = interfaces;
  shape.terminals = terminals;
  shape.terminal_bytes = terminal_bytes;
  active_storage = Bibliotheca::check_out(sizeof(SemanticArchiveFixture)).ptr;
  active = new (active_storage) SemanticArchiveFixture(shape);
}

static auto teardown_semantic() -> void {
  active->~SemanticArchiveFixture();
  Bibliotheca::remit(active_storage);
  active = nullptr;
  active_storage = nullptr;
}

static auto read_semantic_manifest() -> void {
  Benchmark::start_time();
  Allocator::Arena arena;
  const Manifest* manifest = Reader(active->get_buffer()).read_manifest(arena);
  if (manifest == nullptr) {
    abort();
  }
  Count accumulator = manifest->get_dependencies().get_size();
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

static auto read_semantic_package() -> void {
  Benchmark::start_time();
  Allocator::Arena arena;
  const Abstract& restored =
      Reader(active->get_buffer())
          .read_package(
              arena, active->get_manifest(), active->get_dependencies());
  if (!restored.is<Tetrodotoxin::Model::Packages::Compiled>()) {
    abort();
  }
  Count accumulator =
      restored.assume<Tetrodotoxin::Model::Package>().get_definition_count();
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

static auto write_semantic_package() -> void {
  Benchmark::start_time();
  Allocator::Arena arena;
  const auto& package = active->get_package();
  View::Bytes written = Writer::write(
      arena, active->get_manifest(), package,
      package.assume<Tetrodotoxin::Model::Packages::Compiled>()
          .get_terminals());
  if (written.is_empty()) {
    abort();
  }
  Count accumulator = written.get_size();
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

#define SEMANTIC_CASE(symbol, label, ...)       \
  static Harness symbol = {                     \
    .name = label##_view,                       \
    .setup = setup_semantic<__VA_ARGS__>,       \
    .teardown = teardown_semantic,              \
  };                                            \
  PERIMORTEM_BENCHMARK(symbol, read_manifest) { \
    read_semantic_manifest();                   \
  }                                             \
  PERIMORTEM_BENCHMARK(symbol, read_package) {  \
    read_semantic_package();                    \
  }                                             \
  PERIMORTEM_BENCHMARK(symbol, write_package) { \
    write_semantic_package();                   \
  }

// types, fields, depth, callables, body ops, dependencies, cross edges,
// render facts, shader stages, interfaces, terminals, terminal bytes
SEMANTIC_CASE(
    Types16,
    "Archive Semantic Types 16",
    16,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Types64,
    "Archive Semantic Types 64",
    64,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Types256,
    "Archive Semantic Types 256",
    256,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Fields16,
    "Archive Semantic Fields 16",
    0,
    16,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Fields64,
    "Archive Semantic Fields 64",
    0,
    64,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Fields256,
    "Archive Semantic Fields 256",
    0,
    256,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Depth16,
    "Archive Semantic Depth 16",
    0,
    0,
    16,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Depth64,
    "Archive Semantic Depth 64",
    0,
    0,
    64,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Depth256,
    "Archive Semantic Depth 256",
    0,
    0,
    256,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Callables16,
    "Archive Semantic Callables 16",
    0,
    0,
    0,
    16,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Callables64,
    "Archive Semantic Callables 64",
    0,
    0,
    0,
    64,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Callables256,
    "Archive Semantic Callables 256",
    0,
    0,
    0,
    256,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Body16,
    "Archive Semantic Body 16",
    0,
    0,
    0,
    0,
    16,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Body64,
    "Archive Semantic Body 64",
    0,
    0,
    0,
    0,
    64,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Body256,
    "Archive Semantic Body 256",
    0,
    0,
    0,
    0,
    256,
    0,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Dependencies1,
    "Archive Semantic Dependencies 1",
    1,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Dependencies8,
    "Archive Semantic Dependencies 8",
    1,
    0,
    0,
    0,
    0,
    8,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Dependencies32,
    "Archive Semantic Dependencies 32",
    1,
    0,
    0,
    0,
    0,
    32,
    0,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Cross1,
    "Archive Semantic Cross Edges 1",
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Cross8,
    "Archive Semantic Cross Edges 8",
    0,
    0,
    0,
    0,
    0,
    8,
    8,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Cross32,
    "Archive Semantic Cross Edges 32",
    0,
    0,
    0,
    0,
    0,
    32,
    32,
    0,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Render4,
    "Archive Semantic Render Facts 4",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    4,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Render16,
    "Archive Semantic Render Facts 16",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    16,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Render64,
    "Archive Semantic Render Facts 64",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    64,
    0,
    0,
    0,
    0);
SEMANTIC_CASE(
    Stages1,
    "Archive Semantic Shader Stages 1",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    4,
    0,
    0);
SEMANTIC_CASE(
    Stages4,
    "Archive Semantic Shader Stages 4",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    4,
    4,
    0,
    0);
SEMANTIC_CASE(
    Stages16,
    "Archive Semantic Shader Stages 16",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    16,
    4,
    0,
    0);
SEMANTIC_CASE(
    Interfaces4,
    "Archive Semantic Interfaces 4",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    4,
    0,
    0);
SEMANTIC_CASE(
    Interfaces16,
    "Archive Semantic Interfaces 16",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    16,
    0,
    0);
SEMANTIC_CASE(
    Interfaces64,
    "Archive Semantic Interfaces 64",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    64,
    0,
    0);
SEMANTIC_CASE(
    Terminals1,
    "Archive Semantic Terminals 1",
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    32);
SEMANTIC_CASE(
    Terminals16,
    "Archive Semantic Terminals 16",
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    16,
    32);
SEMANTIC_CASE(
    Terminals64,
    "Archive Semantic Terminals 64",
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    64,
    32);
SEMANTIC_CASE(
    Payload1K,
    "Archive Semantic Payload 1K",
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1 << 10);
SEMANTIC_CASE(
    Payload64K,
    "Archive Semantic Payload 64K",
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1 << 16);
SEMANTIC_CASE(
    Payload1M,
    "Archive Semantic Payload 1M",
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1 << 20);

#undef SEMANTIC_CASE
