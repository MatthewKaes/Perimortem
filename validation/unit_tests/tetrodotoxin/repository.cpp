// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/package/repository.hpp"

#include "validation/unit_test.hpp"

#include <stdlib.h>
#include <unistd.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/version.hpp"

#include "tetrodotoxin/archiver/format.hpp"
#include "tetrodotoxin/archiver/writer.hpp"
#include "tetrodotoxin/interpreter/dialects/package.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"
#include "tetrodotoxin/model/dependencies/package.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/compiled.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
#include "tetrodotoxin/model/packages/sources.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "tetrodotoxin/puffer/package/materializer.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Ttx::Concept;
using namespace Validation;

static Harness RepositoryTests = {
  .name = "Tetrodotoxin::Repository"_view,
};

// The cycle fixture supplies only the real Package operations needed to create
// an authored cyclic dependency graph. It does not provide package identity or
// a second semantic representation.
class CyclicPackage final : public Tetrodotoxin::Model::Package {
 public:
  CyclicPackage(
      Allocator::Arena& arena,
      const Tetrodotoxin::Model::Namespace& exports)
      : exports(exports), dependencies(arena) {}

  auto add_dependency(const Tetrodotoxin::Model::Package& dependency) -> void {
    dependencies.insert(Reference<Tetrodotoxin::Model::Package>(dependency));
  }

  auto get_documentation() const -> const Documentation& override {
    return exports.get_documentation();
  }
  auto get_export_count() const -> Count override {
    return exports.get_export_count();
  }
  auto get_export(Count index) const -> const Abstract& override {
    return exports.get_export(index);
  }
  auto get_dependencies() const
      -> View::Vector<Reference<Tetrodotoxin::Model::Package>> override {
    return dependencies;
  }
  auto get_definition_count() const -> Count override { return 0; }
  auto get_definition(Count) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_definition_id(const Abstract&) const -> Count override {
    return Count(-1);
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return exports.resolve_context(route);
  }

 private:
  const Tetrodotoxin::Model::Namespace& exports;
  Managed::Vector<Reference<Tetrodotoxin::Model::Package>> dependencies;
};

static auto evaluate_package(
    Tetrodotoxin::Model::Source& source,
    Tetrodotoxin::Interpreter::Dialects::Package& dialect)
    -> const Tetrodotoxin::Model::Package& {
  Ttx::Lexical::Errors errors;
  const Abstract& exports = source.evaluate(dialect, errors);
  if (!errors.is_empty() || !exports.is<Tetrodotoxin::Model::Namespace>()) {
    __builtin_trap();
  }
  const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 1> sources = {{
    source,
  }};
  const Abstract& result = Tetrodotoxin::Model::Packages::Sources::construct(
      source.get_arena(), sources,
      exports.assume<Tetrodotoxin::Model::Namespace>());
  if (!result.is<Tetrodotoxin::Model::Package>()) {
    __builtin_trap();
  }
  return result.assume<Tetrodotoxin::Model::Package>();
}

static auto add_dependency(
    Tetrodotoxin::Model::Environment& environment,
    const Tetrodotoxin::Model::Dialect& dialect,
    View::Bytes package_name,
    Version version,
    View::Bytes local_name,
    const Tetrodotoxin::Model::Package& package) -> Bool {
  return environment.resolve(
      dialect, local_name, package_name, version, package,
      package.get_documentation());
}

static auto build_terminals(
    Allocator::Arena& arena,
    Managed::Vector<Tetrodotoxin::Model::Terminal>& terminals) -> void {
  Tetrodotoxin::Linker::Linker linker;
  Unsigned_16 section = linker.add_section(
      Tetrodotoxin::Linker::Object::Section::Type::Program, "\xC3"_view);
  auto symbol = Tetrodotoxin::Linker::Object::Symbol::create_function(
      "module_entry"_view, section,
      Tetrodotoxin::Linker::Object::Symbol::Visibility::Global);
  symbol.set_range({0, 1});
  linker.add_symbol(symbol);
  Dynamic::Bytes library = linker.build_library("module.o"_view);

  terminals.insert(
      Tetrodotoxin::Model::Terminal(
          arena, "hello.txt"_view, "Hello Tetrodotoxin!"_view));
  terminals.insert(
      Tetrodotoxin::Model::Terminal(arena, "x86_64.a"_view, library));
}

static auto write_package(
    const Manifest& manifest,
    const Tetrodotoxin::Model::Package& package,
    View::Vector<Tetrodotoxin::Model::Terminal> terminals) -> Dynamic::Bytes {
  Allocator::Arena arena;
  View::Bytes output = Tetrodotoxin::Archiver::Writer::write(
      arena, manifest, package, terminals);
  return Dynamic::Bytes(output);
}

PERIMORTEM_UNIT_TEST(RepositoryTests, register_file_owns_buffer) {
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source source(environment, {}, "package.ttx"_view);
  const Tetrodotoxin::Model::Package& package =
      evaluate_package(source, dialect);
  Manifest manifest(
      "Example.Stored"_view, Version(1, 3), View::Vector<Dependency>());
  Dynamic::Bytes buffer = write_package(
      manifest, package, View::Vector<Tetrodotoxin::Model::Terminal>());
  ASSERT_NOT(buffer.is_empty());

  char path[] = "/tmp/tetrodotoxin-archive-XXXXXX";
  int descriptor = mkstemp(path);
  ASSERT(descriptor >= 0);
  close(descriptor);
  View::Bytes archive_path = NullTerminated::to_view(path);
  ASSERT(File::write(buffer, archive_path));

  Tetrodotoxin::Puffer::Resolution::Package::Repository repository;
  EXPECT(repository.register_file(archive_path));
  EXPECT_NOT(repository.register_file("/tmp/tetrodotoxin-missing"_view));
  ASSERT(
      repository.find_manifest("Example.Stored"_view, Version(1, 3)) !=
      nullptr);
  ASSERT(repository.resolve("Example.Stored"_view, Version(1, 3))
             .is<Tetrodotoxin::Model::Package>());

  EXPECT(File::remove(archive_path));
  EXPECT(repository.resolve("Example.Stored"_view, Version(1, 3))
             .is<Tetrodotoxin::Model::Package>());
}

static auto append_path(Dynamic::Bytes& path, View::Bytes segment) -> void {
  if (!path.is_empty() && path[path.get_size() - 1] != '/') {
    path.append('/');
  }

  path.concat(segment);
}

static auto remove_directory(View::Bytes path) -> void {
  char text[512];
  if (path.get_size() >= sizeof(text)) {
    return;
  }

  for (Count i = 0; i < path.get_size(); i++) {
    text[i] = char(path[i]);
  }
  text[path.get_size()] = '\0';
  rmdir(text);
}

static auto skip_size(
    Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>& reader)
    -> Unsigned_64 {
  Unsigned_64 value = 0;
  Count shift = 0;
  while (shift < 64) {
    Unsigned_8 byte = reader.read_unsigned_8();
    value |= Unsigned_64(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      return value;
    }

    shift += 7;
  }

  return Unsigned_64(-1);
}

static auto skip_reference(
    Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>& reader) -> void {
  Unsigned_8 code = reader.read_unsigned_8();
  skip_size(reader);
  if (code == Unsigned_8(
                  Tetrodotoxin::Archiver::Format::ReferenceCode::
                      DependencyDefinition)) {
    skip_size(reader);
  }
}

static auto external_dependency_offset(View::Bytes buffer) -> Count {
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> header(buffer);
  header.read_bytes(Tetrodotoxin::Archiver::Format::magic.get_size());
  header.read_unsigned_32();
  Unsigned_64 graph_offset = 0;
  for (Count i = 0; i <= Count(Tetrodotoxin::Archiver::Format::Section::Graph);
       i++) {
    graph_offset = header.read_unsigned_64();
  }

  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(
      buffer.slice(Count(graph_offset)));
  Count documentation_count = Count(skip_size(reader));
  for (Count i = 0; i < documentation_count; i++) {
    skip_size(reader);
  }
  Count export_count = Count(skip_size(reader));
  for (Count i = 0; i < export_count; i++) {
    skip_reference(reader);
  }

  Count record_count = Count(skip_size(reader));
  for (Count i = 0; i < record_count; i++) {
    Unsigned_8 code = reader.read_unsigned_8();
    skip_size(reader);
    Count line_count = Count(skip_size(reader));
    for (Count k = 0; k < line_count; k++) {
      skip_size(reader);
    }

    if (code ==
        Unsigned_8(Tetrodotoxin::Archiver::Format::RecordCode::Namespace)) {
      Count child_count = Count(skip_size(reader));
      for (Count k = 0; k < child_count; k++) {
        skip_reference(reader);
        reader.read_unsigned_8();
      }
      continue;
    }

    Unsigned_8 reference = reader.read_unsigned_8();
    if (reference ==
            Unsigned_8(
                Tetrodotoxin::Archiver::Format::ReferenceCode::Dependency) ||
        reference == Unsigned_8(
                         Tetrodotoxin::Archiver::Format::ReferenceCode::
                             DependencyDefinition)) {
      return Count(graph_offset) + reader.get_location();
    }

    skip_size(reader);
  }

  return Count(-1);
}

PERIMORTEM_UNIT_TEST(RepositoryTests, recursive_restore_and_materialize) {
  Tetrodotoxin::Model::Environment empty_environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source core_source(
      empty_environment,
      "public Nested : group { public Leaf : group {} }"_view, "core.ttx"_view);
  const Tetrodotoxin::Model::Package& core =
      evaluate_package(core_source, dialect);

  Allocator::Arena terminal_arena;
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(terminal_arena);
  build_terminals(terminal_arena, terminals);
  Manifest core_manifest(
      "Example.Core"_view, Version(1, 2), View::Vector<Dependency>());
  Dynamic::Bytes core_buffer =
      write_package(core_manifest, core, terminals.get_view());
  ASSERT_NOT(core_buffer.is_empty());

  Tetrodotoxin::Model::Environment consumer_environment;
  ASSERT(add_dependency(
      consumer_environment, dialect, "Example.Core"_view, Version(1, 2),
      "Provider"_view, core));
  Tetrodotoxin::Model::Source consumer_source(
      consumer_environment, "public Core : alias = Provider;"_view,
      "consumer.ttx"_view);
  const Tetrodotoxin::Model::Package& consumer =
      evaluate_package(consumer_source, dialect);
  const Static::Vector<Dependency, 1> consumer_dependencies = {{
    Dependency("Example.Core"_view, Version(1, 2)),
  }};
  Manifest consumer_manifest(
      "Example.Consumer"_view, Version(1, 0), consumer_dependencies);
  Dynamic::Bytes consumer_buffer =
      write_package(consumer_manifest, consumer, terminals.get_view());
  ASSERT_NOT(consumer_buffer.is_empty());

  Dynamic::Bytes invalid_dependency(consumer_buffer);
  Count dependency_offset = external_dependency_offset(invalid_dependency);
  ASSERT(dependency_offset != Count(-1));
  invalid_dependency.get_access()[dependency_offset] = 0x7f;
  Tetrodotoxin::Puffer::Resolution::Package::Repository invalid_repository;
  ASSERT(invalid_repository.register_buffer(core_buffer));
  ASSERT(invalid_repository.register_buffer(invalid_dependency));
  EXPECT(invalid_repository.resolve("Example.Consumer"_view, Version(1, 0))
             .is<Invalid>());

  Dynamic::Bytes core_one_buffer = write_package(
      Manifest("Example.Core"_view, Version(1, 0), View::Vector<Dependency>()),
      core, terminals.get_view());
  ASSERT_NOT(core_one_buffer.is_empty());

  Tetrodotoxin::Puffer::Resolution::Package::Repository repository;
  EXPECT(repository.register_buffer(
      consumer_buffer, "/buffers/consumer.puffer"_view));
  EXPECT(repository.register_buffer(core_buffer, "/buffers/core.puffer"_view));
  EXPECT(repository.register_buffer(core_buffer, "/another/core.puffer"_view));
  EXPECT(repository.register_buffer(core_one_buffer));
  ASSERT(
      repository.find_manifest("Example.Core"_view, Version(1, 2)) != nullptr);

  core_buffer = Dynamic::Bytes();
  consumer_buffer = Dynamic::Bytes();
  core_one_buffer = Dynamic::Bytes();
  const Abstract& restored_consumer =
      repository.resolve("Example.Consumer"_view, Version(1, 0));
  ASSERT(restored_consumer.is<Tetrodotoxin::Model::Packages::Compiled>());
  const Abstract& repeated_consumer =
      repository.resolve("Example.Consumer"_view, Version(1, 0));
  EXPECT(&restored_consumer == &repeated_consumer);

  const Abstract& restored_core =
      repository.resolve("Example.Core"_view, Version(1, 2));
  const Abstract& restored_core_one =
      repository.resolve("Example.Core"_view, Version(1, 0));
  ASSERT(restored_core.is<Tetrodotoxin::Model::Package>());
  ASSERT(restored_core_one.is<Tetrodotoxin::Model::Package>());
  EXPECT(&restored_core != &restored_core_one);

  // Interpretation and a future REPL use the same explicit environment: the
  // host resolves exact packages once, then every Source borrows that context.
  Tetrodotoxin::Model::Environment interpreted_environment;
  ASSERT(repository.resolve(
      interpreted_environment, dialect, "Core"_view, "Example.Core"_view,
      Version(1, 2)));
  Tetrodotoxin::Model::Source interpreted_source(
      interpreted_environment, "public Selected : alias = Core;"_view,
      "dynamic.ttx"_view);
  const Tetrodotoxin::Model::Package& dynamically_interpreted =
      evaluate_package(interpreted_source, dialect);
  ASSERT_EQ(dynamically_interpreted.get_dependencies().get_size(), Count(1));
  EXPECT(
      &dynamically_interpreted.get_dependencies()[0].get() == &restored_core);

  const auto& consumer_package =
      restored_consumer.assume<Tetrodotoxin::Model::Package>();
  ASSERT_EQ(consumer_package.get_dependencies().get_size(), Count(1));
  EXPECT(&consumer_package.get_dependencies()[0].get() == &restored_core);
  const Abstract& core_alias = restored_consumer.resolve_context("Core"_view);
  ASSERT(core_alias.is<Ttx::Model::Alias>());
  EXPECT(&core_alias.resolve() == &restored_core);
  EXPECT(core_alias.resolve_context("Nested"_view)
             .is<Tetrodotoxin::Model::Namespace>());

  char root_text[] = "/tmp/tetrodotoxin-packages-XXXXXX";
  char* root = mkdtemp(root_text);
  ASSERT(root != nullptr);
  View::Bytes root_path = NullTerminated::to_view(root);
  const auto& compiled_core =
      restored_core.assume<Tetrodotoxin::Model::Packages::Compiled>();
  EXPECT(
      Tetrodotoxin::Puffer::Package::Materializer::materialize(
          root_path, core_manifest, compiled_core));

  Dynamic::Bytes package_directory(root_path);
  append_path(package_directory, "Example.Core"_view);
  Dynamic::Bytes version_directory(package_directory);
  append_path(version_directory, "1.2"_view);
  Dynamic::Bytes text_path(version_directory);
  append_path(text_path, "hello.txt"_view);
  Dynamic::Bytes library_path(version_directory);
  append_path(library_path, "x86_64.a"_view);
  EXPECT_TEXT(File::read(text_path), "Hello Tetrodotoxin!"_view);
  Dynamic::Bytes materialized_library = File::read(library_path);
  ASSERT(materialized_library.get_size() > 8);
  EXPECT_TEXT(materialized_library.slice(0, 8), "!<arch>\n"_view);
  EXPECT(
      Algorithm::search(materialized_library, "module_entry"_view) !=
      Count(-1));
  Dynamic::Bytes unversioned(package_directory);
  append_path(unversioned, "hello.txt"_view);
  EXPECT_NOT(File::exists(unversioned));

  File::remove(text_path);
  File::remove(library_path);
  remove_directory(version_directory);
  remove_directory(package_directory);
  rmdir(root);
}

PERIMORTEM_UNIT_TEST(RepositoryTests, conflicts_missing_and_cycles) {
  Tetrodotoxin::Model::Environment empty_environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source core_source(
      empty_environment, "public Nested : group {}"_view, "core.ttx"_view);
  const Tetrodotoxin::Model::Package& core =
      evaluate_package(core_source, dialect);
  Allocator::Arena terminal_arena;
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(terminal_arena);
  build_terminals(terminal_arena, terminals);
  Manifest core_manifest(
      "Example.Core"_view, Version(1, 2), View::Vector<Dependency>());
  Dynamic::Bytes core_buffer =
      write_package(core_manifest, core, terminals.get_view());

  Tetrodotoxin::Model::Source conflict_source(
      empty_environment, "public Different : group {}"_view,
      "conflict.ttx"_view);
  const Tetrodotoxin::Model::Package& conflict =
      evaluate_package(conflict_source, dialect);
  Dynamic::Bytes conflict_buffer =
      write_package(core_manifest, conflict, terminals.get_view());
  Tetrodotoxin::Puffer::Resolution::Package::Repository conflict_repository;
  EXPECT(conflict_repository.register_buffer(core_buffer));
  EXPECT_NOT(conflict_repository.register_buffer(conflict_buffer));
  EXPECT_NOT(conflict_repository.register_buffer("not an archive"_view));

  Tetrodotoxin::Model::Environment wrong_environment;
  ASSERT(add_dependency(
      wrong_environment, dialect, "Example.Core"_view, Version(1, 1),
      "Provider"_view, core));
  Tetrodotoxin::Model::Source wrong_source(
      wrong_environment, "public Core : alias = Provider;"_view,
      "wrong.ttx"_view);
  const Tetrodotoxin::Model::Package& wrong =
      evaluate_package(wrong_source, dialect);
  const Static::Vector<Dependency, 1> wrong_dependencies = {{
    Dependency("Example.Core"_view, Version(1, 1)),
  }};
  Manifest wrong_manifest(
      "Example.Wrong"_view, Version(1, 0), wrong_dependencies);
  Dynamic::Bytes wrong_buffer =
      write_package(wrong_manifest, wrong, terminals.get_view());
  Tetrodotoxin::Puffer::Resolution::Package::Repository missing_repository;
  EXPECT(missing_repository.register_buffer(wrong_buffer));
  EXPECT(missing_repository.register_buffer(core_buffer));
  EXPECT(missing_repository.resolve("Example.Wrong"_view, Version(1, 0))
             .is<Invalid>());

  Allocator::Arena cycle_arena;
  const Abstract& empty_result = Tetrodotoxin::Model::Namespace::construct(
      cycle_arena, {}, View::Vector<Reference<Abstract>>());
  ASSERT(empty_result.is<Tetrodotoxin::Model::Namespace>());
  const auto& empty = empty_result.assume<Tetrodotoxin::Model::Namespace>();
  CyclicPackage first(cycle_arena, empty);
  CyclicPackage second(cycle_arena, empty);
  first.add_dependency(second);
  second.add_dependency(first);
  const Static::Vector<Dependency, 1> first_dependencies = {{
    Dependency("Cycle.B"_view, Version(1, 0)),
  }};
  const Static::Vector<Dependency, 1> second_dependencies = {{
    Dependency("Cycle.A"_view, Version(1, 0)),
  }};
  Dynamic::Bytes first_buffer = write_package(
      Manifest("Cycle.A"_view, Version(1, 0), first_dependencies), first,
      terminals.get_view());
  Dynamic::Bytes second_buffer = write_package(
      Manifest("Cycle.B"_view, Version(1, 0), second_dependencies), second,
      terminals.get_view());
  Tetrodotoxin::Puffer::Resolution::Package::Repository cycle_repository;
  ASSERT(cycle_repository.register_buffer(first_buffer));
  ASSERT(cycle_repository.register_buffer(second_buffer));
  EXPECT(cycle_repository.resolve("Cycle.A"_view, Version(1, 0)).is<Invalid>());
  EXPECT(cycle_repository.resolve("Cycle.B"_view, Version(1, 0)).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(RepositoryTests, external_paths_ignore_record_order) {
  Tetrodotoxin::Model::Environment empty_environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source first_core_source(
      empty_environment,
      "public Nested : group {} public Other : group {}"_view,
      "first.ttx"_view);
  Tetrodotoxin::Model::Source second_core_source(
      empty_environment,
      "public Other : group {} public Nested : group {}"_view,
      "second.ttx"_view);
  const Tetrodotoxin::Model::Package& first_core =
      evaluate_package(first_core_source, dialect);
  const Tetrodotoxin::Model::Package& second_core =
      evaluate_package(second_core_source, dialect);

  Tetrodotoxin::Model::Environment first_environment;
  Tetrodotoxin::Model::Environment second_environment;
  ASSERT(add_dependency(
      first_environment, dialect, "Example.Core"_view, Version(1, 2),
      "Provider"_view, first_core));
  ASSERT(add_dependency(
      second_environment, dialect, "Example.Core"_view, Version(1, 2),
      "Provider"_view, second_core));
  Tetrodotoxin::Model::Source first_consumer_source(
      first_environment, "public Shared : alias = Provider::Nested;"_view,
      "consumer.ttx"_view);
  Tetrodotoxin::Model::Source second_consumer_source(
      second_environment, "public Shared : alias = Provider::Nested;"_view,
      "consumer.ttx"_view);
  const Tetrodotoxin::Model::Package& first_consumer =
      evaluate_package(first_consumer_source, dialect);
  const Tetrodotoxin::Model::Package& second_consumer =
      evaluate_package(second_consumer_source, dialect);
  const Static::Vector<Dependency, 1> dependencies = {{
    Dependency("Example.Core"_view, Version(1, 2)),
  }};
  Manifest manifest("Example.Consumer"_view, Version(1, 0), dependencies);
  Allocator::Arena terminal_arena;
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(terminal_arena);
  build_terminals(terminal_arena, terminals);

  Dynamic::Bytes first =
      write_package(manifest, first_consumer, terminals.get_view());
  Dynamic::Bytes second =
      write_package(manifest, second_consumer, terminals.get_view());

  EXPECT(first == second);
}

PERIMORTEM_UNIT_TEST(RepositoryTests, materializer_rejects_escape) {
  Allocator::Arena arena;
  const Abstract& empty_result = Tetrodotoxin::Model::Namespace::construct(
      arena, {}, View::Vector<Reference<Abstract>>());
  ASSERT(empty_result.is<Tetrodotoxin::Model::Namespace>());
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(arena);
  terminals.insert(
      Tetrodotoxin::Model::Terminal(
          arena, "../outside.txt"_view, "escaped"_view));
  Tetrodotoxin::Model::Packages::Precompiled package(
      arena, empty_result.assume<Tetrodotoxin::Model::Namespace>(), {}, {},
      terminals.get_view());
  Manifest manifest(
      "Example.Escape"_view, Version(1, 0), View::Vector<Dependency>());
  char root_text[] = "/tmp/tetrodotoxin-escape-XXXXXX";
  char* root = mkdtemp(root_text);
  ASSERT(root != nullptr);

  EXPECT_NOT(
      Tetrodotoxin::Puffer::Package::Materializer::materialize(
          NullTerminated::to_view(root), manifest, package));
  Dynamic::Bytes outside(NullTerminated::to_view(root));
  append_path(outside, "outside.txt"_view);
  EXPECT_NOT(File::exists(outside));
  rmdir(root);
}
