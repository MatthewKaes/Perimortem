// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/benchmark.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/archiver/reader.hpp"
#include "tetrodotoxin/archiver/writer.hpp"
#include "tetrodotoxin/model/environment.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/compiled.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
#include "tetrodotoxin/model/packages/sources.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "tetrodotoxin/model/terminal.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/block.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Ttx::Concept;
using namespace Validation;

class BenchmarkDialect final : public Tetrodotoxin::Model::Dialect {
 public:
  auto get_name() const -> View::Bytes override { return "Benchmark"_view; }

  auto evaluate(Ttx::Lexical::Cursor&, const Abstract&) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class ArchiveFixture {
 public:
  ArchiveFixture(
      Count local_count,
      Count dependency_count,
      Count terminal_count,
      Count terminal_size,
      Count depth_count = 0,
      Count alias_count = 0,
      Count documented_count = 0,
      Count documentation_lines = 0)
      : source(environment, {}, "benchmark.ttx"_view),
        root(arena, {}),
        manifest_dependencies(arena),
        terminals(arena) {
    build_locals(local_count);
    build_depth(depth_count);
    build_aliases(alias_count);
    build_documented(documented_count, documentation_lines);
    build_dependencies(dependency_count);
    build_terminals(terminal_count, terminal_size);

    const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 1> members = {{
      source,
    }};
    const Abstract& constructed =
        Tetrodotoxin::Model::Packages::Sources::construct(arena, members, root);
    if (!constructed.is<Tetrodotoxin::Model::Packages::Interpreted>()) {
      abort();
    }
    package = &constructed.assume<Tetrodotoxin::Model::Package>();
    manifest = &arena.construct<Manifest>(
        "Benchmark.Package"_view, Version(1, 0),
        manifest_dependencies.get_view());
    View::Bytes written = Tetrodotoxin::Archiver::Writer::write(
        arena, *manifest, *package, terminals.get_view());
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
    return package->get_dependencies();
  }
  auto get_terminals() const -> View::Vector<Tetrodotoxin::Model::Terminal> {
    return terminals;
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

  auto content(Count size) -> View::Bytes {
    if (size == 0) {
      return {};
    }

    Unsigned_8* data = arena.allocate(size);
    for (Count i = 0; i < size; i++) {
      data[i] = Unsigned_8((i * 37 + 11) & 0xff);
    }
    return View::Bytes(data, size);
  }

  auto build_locals(Count count) -> void {
    for (Count i = 0; i < count; i++) {
      auto& definition = arena.construct<Tetrodotoxin::Model::Namespace>(
          arena, name("Local", i));
      if (!root.add_export(definition)) {
        abort();
      }
    }
  }

  auto build_depth(Count count) -> void {
    if (count == 0) {
      return;
    }

    Tetrodotoxin::Model::Namespace* child = nullptr;
    for (Count i = count; i > 0; i--) {
      auto& parent = arena.construct<Tetrodotoxin::Model::Namespace>(
          arena, name("Depth", i - 1));
      if (child != nullptr && !parent.add_export(*child)) {
        abort();
      }

      child = &parent;
    }

    if (!root.add_export(*child)) {
      abort();
    }
  }

  auto build_aliases(Count count) -> void {
    if (count == 0) {
      return;
    }

    auto& target = arena.construct<Tetrodotoxin::Model::Namespace>(
        arena, "AliasTarget"_view);
    if (!root.add_root(target)) {
      abort();
    }

    for (Count i = 0; i < count; i++) {
      auto& alias =
          arena.construct<Ttx::Model::Alias>(name("Alias", i), target);
      if (!root.add_export(alias)) {
        abort();
      }
    }
  }

  auto build_documented(Count count, Count line_count) -> void {
    for (Count i = 0; i < count; i++) {
      Managed::Vector<View::Bytes> lines(arena);
      lines.reset(line_count);
      for (Count line = 0; line < line_count; line++) {
        lines.insert(name("Documentation", i * line_count + line));
      }

      auto& documentation =
          arena.construct<Ttx::Model::Documentations::Block>(lines.get_view());
      auto& definition = arena.construct<Tetrodotoxin::Model::Namespace>(
          arena, name("Documented", i), documentation);
      if (!root.add_export(definition)) {
        abort();
      }
    }
  }

  auto build_dependencies(Count count) -> void {
    manifest_dependencies.reset(count);
    for (Count i = 0; i < count; i++) {
      auto& dependency_root =
          arena.construct<Tetrodotoxin::Model::Namespace>(arena, View::Bytes{});
      auto& target = arena.construct<Tetrodotoxin::Model::Namespace>(
          arena, name("Target", i));
      if (!dependency_root.add_export(target)) {
        abort();
      }

      const Static::Vector<Reference<Abstract>, 1> definitions = {{target}};
      auto& dependency =
          arena.construct<Tetrodotoxin::Model::Packages::Precompiled>(
              arena, dependency_root,
              View::Vector<Reference<Tetrodotoxin::Model::Package>>(),
              definitions, View::Vector<Tetrodotoxin::Model::Terminal>());
      View::Bytes package_name = name("Benchmark.Dependency", i);
      View::Bytes local_name = name("Dependency", i);
      Version version(1, Unsigned_16(i + 1));
      if (!environment.resolve(
              dialect, local_name, package_name, version, dependency,
              dependency.get_documentation())) {
        abort();
      }

      auto& alias =
          arena.construct<Ttx::Model::Alias>(name("External", i), target);
      if (!root.add_export(alias)) {
        abort();
      }
      manifest_dependencies.insert(Dependency(package_name, version));
    }
  }

  auto build_terminals(Count count, Count size) -> void {
    terminals.reset(count);
    View::Bytes payload = content(size);
    for (Count i = 0; i < count; i++) {
      terminals.insert(
          Tetrodotoxin::Model::Terminal(arena, name("artifact", i), payload));
    }
  }

  Allocator::Arena arena;
  Tetrodotoxin::Model::Environment environment;
  BenchmarkDialect dialect;
  Tetrodotoxin::Model::Source source;
  Tetrodotoxin::Model::Namespace root;
  Managed::Vector<Dependency> manifest_dependencies;
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals;
  const Tetrodotoxin::Model::Package* package = nullptr;
  const Manifest* manifest = nullptr;
  Dynamic::Bytes buffer;
};

static ArchiveFixture* active_fixture = nullptr;
static Unsigned_8* active_storage = nullptr;

template <
    Count locals,
    Count dependencies,
    Count terminals,
    Count terminal_size>
auto setup_fixture() -> void {
  active_storage = Bibliotheca::check_out(sizeof(ArchiveFixture)).ptr;
  active_fixture = new (active_storage)
      ArchiveFixture(locals, dependencies, terminals, terminal_size);
}

template <Count depth, Count aliases, Count documented, Count lines>
auto setup_graph_fixture() -> void {
  active_storage = Bibliotheca::check_out(sizeof(ArchiveFixture)).ptr;
  active_fixture = new (active_storage)
      ArchiveFixture(0, 0, 0, 0, depth, aliases, documented, lines);
}

static auto teardown_fixture() -> void {
  active_fixture->~ArchiveFixture();
  Bibliotheca::remit(active_storage);
  active_fixture = nullptr;
  active_storage = nullptr;
}

static auto read_manifest() -> void {
  Benchmark::start_time();
  Allocator::Arena arena;
  const Manifest* manifest =
      Reader(active_fixture->get_buffer()).read_manifest(arena);
  if (manifest == nullptr) {
    abort();
  }
  Count accumulator = manifest->get_dependencies().get_size();
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

static auto read_package() -> void {
  Benchmark::start_time();
  Allocator::Arena arena;
  const Abstract& restored = Reader(active_fixture->get_buffer())
                                 .read_package(
                                     arena, active_fixture->get_manifest(),
                                     active_fixture->get_dependencies());
  if (!restored.is<Tetrodotoxin::Model::Packages::Compiled>()) {
    abort();
  }

  const auto& package =
      restored.assume<Tetrodotoxin::Model::Packages::Compiled>();
  Count accumulator =
      package.get_definition_count() + package.get_terminals().get_size();
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

static auto write_fixture_package() -> void {
  Benchmark::start_time();
  Allocator::Arena arena;
  View::Bytes written = Tetrodotoxin::Archiver::Writer::write(
      arena, active_fixture->get_manifest(), active_fixture->get_package(),
      active_fixture->get_terminals());
  if (written.is_empty()) {
    abort();
  }
  Count accumulator = written.get_size();
  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

#define ARCHIVE_SWEEP_CASE(                                        \
    symbol, label, locals, dependencies, terminals, size)          \
  static Harness symbol = {                                        \
    .name = label##_view,                                          \
    .setup = setup_fixture<locals, dependencies, terminals, size>, \
    .teardown = teardown_fixture,                                  \
  };                                                               \
  PERIMORTEM_BENCHMARK(symbol, read_manifest) {                    \
    ::read_manifest();                                             \
  }                                                                \
  PERIMORTEM_BENCHMARK(symbol, read_package) {                     \
    ::read_package();                                              \
  }                                                                \
  PERIMORTEM_BENCHMARK(symbol, write_package) {                    \
    ::write_fixture_package();                                     \
  }

#define ARCHIVE_GRAPH_CASE(symbol, label, depth, aliases, documented, lines) \
  static Harness symbol = {                                                  \
    .name = label##_view,                                                    \
    .setup = setup_graph_fixture<depth, aliases, documented, lines>,         \
    .teardown = teardown_fixture,                                            \
  };                                                                         \
  PERIMORTEM_BENCHMARK(symbol, read_manifest) {                              \
    ::read_manifest();                                                       \
  }                                                                          \
  PERIMORTEM_BENCHMARK(symbol, read_package) {                               \
    ::read_package();                                                        \
  }                                                                          \
  PERIMORTEM_BENCHMARK(symbol, write_package) {                              \
    ::write_fixture_package();                                               \
  }

ARCHIVE_SWEEP_CASE(Graph0016, "Archive Graph 16", 16, 0, 0, 0);
ARCHIVE_SWEEP_CASE(Graph0064, "Archive Graph 64", 64, 0, 0, 0);
ARCHIVE_SWEEP_CASE(Graph0256, "Archive Graph 256", 256, 0, 0, 0);
ARCHIVE_SWEEP_CASE(Graph1024, "Archive Graph 1024", 1024, 0, 0, 0);
ARCHIVE_SWEEP_CASE(Graph4096, "Archive Graph 4096", 4096, 0, 0, 0);

ARCHIVE_GRAPH_CASE(Depth0016, "Archive Depth 16", 16, 0, 0, 0);
ARCHIVE_GRAPH_CASE(Depth0064, "Archive Depth 64", 64, 0, 0, 0);
ARCHIVE_GRAPH_CASE(Depth0256, "Archive Depth 256", 256, 0, 0, 0);
ARCHIVE_GRAPH_CASE(Depth1024, "Archive Depth 1024", 1024, 0, 0, 0);
ARCHIVE_GRAPH_CASE(Depth4096, "Archive Depth 4096", 4096, 0, 0, 0);

ARCHIVE_GRAPH_CASE(Aliases0016, "Archive Aliases 16", 0, 16, 0, 0);
ARCHIVE_GRAPH_CASE(Aliases0064, "Archive Aliases 64", 0, 64, 0, 0);
ARCHIVE_GRAPH_CASE(Aliases0256, "Archive Aliases 256", 0, 256, 0, 0);
ARCHIVE_GRAPH_CASE(Aliases1024, "Archive Aliases 1024", 0, 1024, 0, 0);
ARCHIVE_GRAPH_CASE(Aliases4096, "Archive Aliases 4096", 0, 4096, 0, 0);

ARCHIVE_GRAPH_CASE(
    Documentation0016,
    "Archive Documentation 16x4",
    0,
    0,
    16,
    4);
ARCHIVE_GRAPH_CASE(
    Documentation0064,
    "Archive Documentation 64x4",
    0,
    0,
    64,
    4);
ARCHIVE_GRAPH_CASE(
    Documentation0256,
    "Archive Documentation 256x4",
    0,
    0,
    256,
    4);
ARCHIVE_GRAPH_CASE(
    Documentation1024,
    "Archive Documentation 1024x4",
    0,
    0,
    1024,
    4);

ARCHIVE_SWEEP_CASE(Dependencies0001, "Archive Dependencies 1", 0, 1, 0, 0);
ARCHIVE_SWEEP_CASE(Dependencies0008, "Archive Dependencies 8", 0, 8, 0, 0);
ARCHIVE_SWEEP_CASE(Dependencies0032, "Archive Dependencies 32", 0, 32, 0, 0);
ARCHIVE_SWEEP_CASE(Dependencies0128, "Archive Dependencies 128", 0, 128, 0, 0);
ARCHIVE_SWEEP_CASE(Dependencies0512, "Archive Dependencies 512", 0, 512, 0, 0);

ARCHIVE_SWEEP_CASE(Terminals0001, "Archive Terminals 1", 0, 0, 1, 32);
ARCHIVE_SWEEP_CASE(Terminals0016, "Archive Terminals 16", 0, 0, 16, 32);
ARCHIVE_SWEEP_CASE(Terminals0064, "Archive Terminals 64", 0, 0, 64, 32);
ARCHIVE_SWEEP_CASE(Terminals0256, "Archive Terminals 256", 0, 0, 256, 32);
ARCHIVE_SWEEP_CASE(Terminals1024, "Archive Terminals 1024", 0, 0, 1024, 32);

ARCHIVE_SWEEP_CASE(Payload0001K, "Archive Payload 1K", 0, 0, 1, 1 << 10);
ARCHIVE_SWEEP_CASE(Payload0064K, "Archive Payload 64K", 0, 0, 1, 1 << 16);
ARCHIVE_SWEEP_CASE(Payload1024K, "Archive Payload 1M", 0, 0, 1, 1 << 20);
ARCHIVE_SWEEP_CASE(Payload8192K, "Archive Payload 8M", 0, 0, 1, 1 << 23);

ARCHIVE_SWEEP_CASE(MixedSmall, "Archive Mixed Small", 64, 8, 8, 1 << 10);
ARCHIVE_SWEEP_CASE(MixedMedium, "Archive Mixed Medium", 512, 32, 32, 1 << 12);
ARCHIVE_SWEEP_CASE(MixedLarge, "Archive Mixed Large", 2048, 128, 64, 1 << 14);

#undef ARCHIVE_SWEEP_CASE
