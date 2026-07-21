// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/archiver/format.hpp"
#include "tetrodotoxin/archiver/reader.hpp"
#include "tetrodotoxin/archiver/writer.hpp"
#include "tetrodotoxin/interpreter/dialects/package.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/compiled.hpp"
#include "tetrodotoxin/model/packages/interpreted.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
#include "tetrodotoxin/model/packages/sources.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/callables/static.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Ttx::Concept;
using namespace Validation;

static Harness ArchiverTests = {
  .name = "Tetrodotoxin::Archiver"_view,
};

class UnsupportedAddressable final : public Ttx::Model::Addressable {
 public:
  UnsupportedAddressable(View::Bytes name, const Ttx::Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
};

class UnsupportedCallable final : public Ttx::Model::Callables::Static {
 public:
  auto get_name() const -> View::Bytes override { return "Call"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_parameters() const -> const Layout& override { return layout; }
  auto get_results() const -> const Layout& override { return layout; }

 private:
  Ttx::Model::Layouts::Fluid layout;
};

static auto evaluate_package(
    Tetrodotoxin::Model::Source& source,
    Tetrodotoxin::Interpreter::Dialects::Package& dialect)
    -> const Tetrodotoxin::Model::Package& {
  Ttx::Lexical::Errors errors;
  const Abstract& result = source.evaluate(dialect, errors);
  if (!errors.is_empty() || !result.is<Tetrodotoxin::Model::Package>()) {
    __builtin_trap();
  }

  return result.assume<Tetrodotoxin::Model::Package>();
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
  Allocator::Arena writer_arena;
  View::Bytes output = Tetrodotoxin::Archiver::Writer::write(
      writer_arena, manifest, package, terminals);
  return Dynamic::Bytes(output);
}

static auto find_terminal(
    const Tetrodotoxin::Model::Packages::Compiled& package,
    View::Bytes path) -> View::Bytes {
  View::Vector<Tetrodotoxin::Model::Terminal> terminals =
      package.get_terminals();
  for (Count i = 0; i < terminals.get_size(); i++) {
    if (terminals[i].get_path() == path) {
      return terminals[i].get_content();
    }
  }

  return {};
}

static auto read_offset(View::Bytes buffer, Format::Section section)
    -> Unsigned_64 {
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(buffer);
  reader.read_bytes(Format::magic.get_size());
  reader.read_unsigned_32();
  for (Count i = 0; i < Count(section); i++) {
    reader.read_unsigned_64();
  }

  return reader.read_unsigned_64();
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

static auto first_record_offset(View::Bytes buffer) -> Count {
  Count graph_offset = Count(read_offset(buffer, Format::Section::Graph));
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(
      buffer.slice(graph_offset));
  Count documentation_count = Count(skip_size(reader));
  for (Count i = 0; i < documentation_count; i++) {
    skip_size(reader);
  }

  Count export_count = Count(skip_size(reader));
  for (Count i = 0; i < export_count; i++) {
    skip_size(reader);
  }

  skip_size(reader);
  return graph_offset + reader.get_location();
}

static auto first_alias_target_offset(View::Bytes buffer) -> Count {
  Count graph_offset = Count(read_offset(buffer, Format::Section::Graph));
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(
      buffer.slice(graph_offset));
  Count documentation_count = Count(skip_size(reader));
  for (Count i = 0; i < documentation_count; i++) {
    skip_size(reader);
  }

  Count export_count = Count(skip_size(reader));
  for (Count i = 0; i < export_count; i++) {
    skip_size(reader);
  }

  Count record_count = Count(skip_size(reader));
  for (Count i = 0; i < record_count; i++) {
    Unsigned_8 code = reader.read_unsigned_8();
    skip_size(reader);
    Count line_count = Count(skip_size(reader));
    for (Count k = 0; k < line_count; k++) {
      skip_size(reader);
    }

    if (code == Unsigned_8(Format::RecordCode::Namespace)) {
      Count child_count = Count(skip_size(reader));
      for (Count k = 0; k < child_count; k++) {
        skip_size(reader);
      }
      continue;
    }

    reader.read_unsigned_8();
    return graph_offset + reader.get_location();
  }

  return Count(-1);
}

PERIMORTEM_UNIT_TEST(ArchiverTests, compact_manifest_versions) {
  Tetrodotoxin::Model::Environment empty_environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source source(empty_environment, {}, "package.ttx"_view);
  const Tetrodotoxin::Model::Package& package =
      evaluate_package(source, dialect);
  Manifest manifest(
      "Example.Core"_view, Version(1, 0), View::Vector<Dependency>());

  Dynamic::Bytes buffer = write_package(
      manifest, package, View::Vector<Tetrodotoxin::Model::Terminal>());

  ASSERT_NOT(buffer.is_empty());
  Unsigned_64 manifest_offset = read_offset(buffer, Format::Section::Manifest);
  Unsigned_64 string_offset = read_offset(buffer, Format::Section::Strings);
  EXPECT(string_offset - manifest_offset > Unsigned_64(4));
  EXPECT_EQ(buffer[Count(string_offset)], Unsigned_8(0));
  Allocator::Arena arena;
  const Manifest* restored =
      Tetrodotoxin::Archiver::Reader(buffer).read_manifest(arena);
  ASSERT(restored != nullptr);
  EXPECT_TEXT(restored->get_name(), "Example.Core"_view);
  EXPECT(restored->get_version() == Version(1, 0));
  EXPECT_EQ(restored->get_dependencies().get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(ArchiverTests, development_version) {
  Tetrodotoxin::Model::Environment empty_environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source source(empty_environment, {}, "package.ttx"_view);
  const Tetrodotoxin::Model::Package& package =
      evaluate_package(source, dialect);
  Manifest manifest(
      "Example.Development"_view, Version(0, 1), View::Vector<Dependency>());

  Dynamic::Bytes buffer = write_package(
      manifest, package, View::Vector<Tetrodotoxin::Model::Terminal>());
  ASSERT_NOT(buffer.is_empty());
  Unsigned_64 manifest_offset = read_offset(buffer, Format::Section::Manifest);
  Unsigned_64 string_offset = read_offset(buffer, Format::Section::Strings);
  EXPECT(string_offset - manifest_offset > Unsigned_64(4));
  EXPECT_EQ(buffer[Count(string_offset)], Unsigned_8(0));

  Allocator::Arena arena;
  const Manifest* restored =
      Tetrodotoxin::Archiver::Reader(buffer).read_manifest(arena);
  ASSERT(restored != nullptr);
  EXPECT(restored->get_version() == Version(0, 1));

  Manifest unset("Example.Unset"_view, Version(), View::Vector<Dependency>());
  EXPECT_NOT(unset.is_valid());
  EXPECT(write_package(
             unset, package, View::Vector<Tetrodotoxin::Model::Terminal>())
             .is_empty());

  const Static::Vector<Dependency, 1> unset_dependencies = {{
    Dependency("Example.Unset"_view, Version()),
  }};
  Manifest unset_dependency(
      "Example.Development"_view, Version(0, 1), unset_dependencies);
  EXPECT_NOT(unset_dependency.is_valid());
}

PERIMORTEM_UNIT_TEST(ArchiverTests, package_graph_and_terminals) {
  Tetrodotoxin::Model::Environment empty_environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source source(
      empty_environment,
      "// nested surface\n"
      "public Nested : group {\n"
      "  // leaf surface\n"
      "  public Leaf : group {}\n"
      "}\n"
      "// local redirect\n"
      "public Mirror : alias = Nested;"_view,
      "package.ttx"_view);
  const Tetrodotoxin::Model::Package& package =
      evaluate_package(source, dialect);
  ASSERT(package.is<Tetrodotoxin::Model::Packages::Interpreted>());
  EXPECT_NOT(package.is<Tetrodotoxin::Model::Packages::Compiled>());

  Allocator::Arena terminal_arena;
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(terminal_arena);
  build_terminals(terminal_arena, terminals);
  Manifest manifest(
      "Example.Core"_view, Version(1, 2), View::Vector<Dependency>());
  Allocator::Arena manifest_arena;
  Allocator::Arena restore_arena;
  const Manifest* independently_read = nullptr;
  const Abstract* restored_pointer = nullptr;
  {
    Dynamic::Bytes buffer =
        write_package(manifest, package, terminals.get_view());
    ASSERT_NOT(buffer.is_empty());
    independently_read =
        Tetrodotoxin::Archiver::Reader(buffer).read_manifest(manifest_arena);
    ASSERT(independently_read != nullptr);
    restored_pointer = &Tetrodotoxin::Archiver::Reader(buffer).read_package(
        restore_arena, *independently_read,
        View::Vector<Reference<Tetrodotoxin::Model::Package>>());
  }

  EXPECT(independently_read->get_version() == Version(1, 2));
  const Abstract& restored = *restored_pointer;
  ASSERT(restored.is<Tetrodotoxin::Model::Packages::Compiled>());
  EXPECT(restored.is<Tetrodotoxin::Model::Package>());
  EXPECT_NOT(restored.is<Tetrodotoxin::Model::Packages::Interpreted>());
  EXPECT(restored.get_name().is_empty());

  const Abstract& nested = restored.resolve_context("Nested"_view);
  ASSERT(nested.is<Tetrodotoxin::Model::Namespace>());
  EXPECT_TEXT(nested.get_documentation().get_line(0), "nested surface"_view);
  EXPECT(
      nested.resolve_context("Leaf"_view).is<Tetrodotoxin::Model::Namespace>());
  const Abstract& mirror = restored.resolve_context("Mirror"_view);
  ASSERT(mirror.is<Ttx::Model::Alias>());
  EXPECT(&mirror.resolve() == &nested);
  EXPECT_TEXT(mirror.get_documentation().get_line(0), "local redirect"_view);
  EXPECT_TEXT(
      restored.assume<Tetrodotoxin::Model::Package>().get_export(0).get_name(),
      "Nested"_view);
  EXPECT_TEXT(
      restored.assume<Tetrodotoxin::Model::Package>().get_export(1).get_name(),
      "Mirror"_view);
  EXPECT_EQ(
      restored.assume<Tetrodotoxin::Model::Package>().get_export_count(),
      Count(2));

  const auto& compiled =
      restored.assume<Tetrodotoxin::Model::Packages::Compiled>();
  View::Bytes text = find_terminal(compiled, "hello.txt"_view);
  View::Bytes library = find_terminal(compiled, "x86_64.a"_view);
  ASSERT_TEXT(text, "Hello Tetrodotoxin!"_view);
  ASSERT(library.get_size() > 8);
  EXPECT_TEXT(library.slice(0, 8), "!<arch>\n"_view);
  EXPECT(Algorithm::search(library, "module_entry"_view) != Count(-1));
  EXPECT(library.get_data() != terminals[1].get_content().get_data());

  EXPECT_TEXT(
      find_terminal(compiled, "hello.txt"_view), "Hello Tetrodotoxin!"_view);
  EXPECT(
      Algorithm::search(
          find_terminal(compiled, "x86_64.a"_view), "module_entry"_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(ArchiverTests, writer_rejects_unsupported_and_paths) {
  Tetrodotoxin::Model::Environment empty_environment;
  Allocator::Arena graph_arena;
  Ttx::Model::Types::Unsigned_8 type;
  const Static::Vector<Reference<Abstract>, 1> exported = {{type}};
  const Abstract& namespace_result =
      Tetrodotoxin::Model::Namespace::construct(graph_arena, {}, exported);
  ASSERT(namespace_result.is<Tetrodotoxin::Model::Namespace>());
  Tetrodotoxin::Model::Packages::Precompiled unsupported(
      graph_arena, namespace_result.assume<Tetrodotoxin::Model::Namespace>());
  Manifest manifest(
      "Example.Core"_view, Version(1, 2), View::Vector<Dependency>());
  Dynamic::Bytes unsupported_buffer = write_package(
      manifest, unsupported, View::Vector<Tetrodotoxin::Model::Terminal>());
  EXPECT(unsupported_buffer.is_empty());

  UnsupportedAddressable addressable("Value"_view, type);
  const Static::Vector<Reference<Abstract>, 1> addressable_export = {{
    addressable,
  }};
  const Abstract& addressable_namespace =
      Tetrodotoxin::Model::Namespace::construct(
          graph_arena, {}, addressable_export);
  ASSERT(addressable_namespace.is<Tetrodotoxin::Model::Namespace>());
  Tetrodotoxin::Model::Packages::Precompiled addressable_package(
      graph_arena,
      addressable_namespace.assume<Tetrodotoxin::Model::Namespace>());
  EXPECT(write_package(
             manifest, addressable_package,
             View::Vector<Tetrodotoxin::Model::Terminal>())
             .is_empty());

  UnsupportedCallable callable;
  const Static::Vector<Reference<Abstract>, 1> callable_export = {{callable}};
  const Abstract& callable_namespace =
      Tetrodotoxin::Model::Namespace::construct(
          graph_arena, {}, callable_export);
  ASSERT(callable_namespace.is<Tetrodotoxin::Model::Namespace>());
  Tetrodotoxin::Model::Packages::Precompiled callable_package(
      graph_arena, callable_namespace.assume<Tetrodotoxin::Model::Namespace>());
  EXPECT(write_package(
             manifest, callable_package,
             View::Vector<Tetrodotoxin::Model::Terminal>())
             .is_empty());

  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source source(empty_environment, {}, "package.ttx"_view);
  const Tetrodotoxin::Model::Package& package =
      evaluate_package(source, dialect);
  Allocator::Arena terminal_arena;
  const Static::Vector<View::Bytes, 5> invalid_paths = {{
    View::Bytes(),
    "/rooted"_view,
    "../escape"_view,
    "folder/./file"_view,
    "folder\\file"_view,
  }};
  for (Count i = 0; i < invalid_paths.get_size(); i++) {
    Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(terminal_arena);
    terminals.insert(
        Tetrodotoxin::Model::Terminal(
            terminal_arena, invalid_paths[i], "bytes"_view));
    Dynamic::Bytes output =
        write_package(manifest, package, terminals.get_view());
    EXPECT(output.is_empty());
  }

  Managed::Vector<Tetrodotoxin::Model::Terminal> duplicates(terminal_arena);
  duplicates.insert(
      Tetrodotoxin::Model::Terminal(
          terminal_arena, "same.bin"_view, "first"_view));
  duplicates.insert(
      Tetrodotoxin::Model::Terminal(
          terminal_arena, "same.bin"_view, "second"_view));
  Dynamic::Bytes duplicate_buffer =
      write_package(manifest, package, duplicates.get_view());
  EXPECT(duplicate_buffer.is_empty());
}

PERIMORTEM_UNIT_TEST(ArchiverTests, malformed_graph_is_invalid) {
  Tetrodotoxin::Model::Environment empty_environment;
  Tetrodotoxin::Interpreter::Dialects::Package dialect;
  Tetrodotoxin::Model::Source source(
      empty_environment,
      "public Nested : group {} public Mirror : alias = Nested;"_view,
      "package.ttx"_view);
  const Tetrodotoxin::Model::Package& package =
      evaluate_package(source, dialect);
  Allocator::Arena terminal_arena;
  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(terminal_arena);
  build_terminals(terminal_arena, terminals);
  Manifest manifest(
      "Example.Core"_view, Version(1, 2), View::Vector<Dependency>());
  Dynamic::Bytes valid = write_package(manifest, package, terminals.get_view());
  ASSERT_NOT(valid.is_empty());

  Dynamic::Bytes unknown_record(valid);
  unknown_record.get_access()[first_record_offset(unknown_record)] = 0x7f;
  Allocator::Arena unknown_arena;
  const Abstract& unknown =
      Tetrodotoxin::Archiver::Reader(unknown_record)
          .read_package(
              unknown_arena, manifest,
              View::Vector<Reference<Tetrodotoxin::Model::Package>>());
  EXPECT(unknown.is<Invalid>());

  Dynamic::Bytes invalid_local(valid);
  Count alias_target = first_alias_target_offset(invalid_local);
  ASSERT(alias_target != Count(-1));
  invalid_local.get_access()[alias_target] = 0x7f;
  Allocator::Arena local_arena;
  const Abstract& invalid_local_result =
      Tetrodotoxin::Archiver::Reader(invalid_local)
          .read_package(
              local_arena, manifest,
              View::Vector<Reference<Tetrodotoxin::Model::Package>>());
  EXPECT(invalid_local_result.is<Invalid>());

  Allocator::Arena truncated_arena;
  const Abstract& truncated =
      Tetrodotoxin::Archiver::Reader(valid.slice(0, valid.get_size() - 1))
          .read_package(
              truncated_arena, manifest,
              View::Vector<Reference<Tetrodotoxin::Model::Package>>());
  EXPECT(truncated.is<Invalid>());

  Dynamic::Bytes excessive(valid);
  Count graph_offset = Count(read_offset(excessive, Format::Section::Graph));
  for (Count i = 0; i < 10; i++) {
    excessive.get_access()[graph_offset + i] = 0xff;
  }
  Allocator::Arena excessive_arena;
  const Abstract& excessive_result =
      Tetrodotoxin::Archiver::Reader(excessive).read_package(
          excessive_arena, manifest,
          View::Vector<Reference<Tetrodotoxin::Model::Package>>());
  EXPECT(excessive_result.is<Invalid>());
}

PERIMORTEM_UNIT_TEST(ArchiverTests, deep_graph_uses_bounded_stack) {
  static constexpr Count depth = 4096;
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {}, "deep.ttx"_view);
  Tetrodotoxin::Model::Namespace root(source.get_arena(), {});
  Tetrodotoxin::Model::Namespace* child = nullptr;
  for (Count i = 0; i < depth; i++) {
    auto& parent = source.get_arena().construct<Tetrodotoxin::Model::Namespace>(
        source.get_arena(), "Node"_view);
    if (child != nullptr) {
      ASSERT(parent.add_export(*child));
    }
    child = &parent;
  }
  ASSERT(root.add_export(*child));

  const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 1> members = {{
    source,
  }};
  const Abstract& package = Tetrodotoxin::Model::Packages::Sources::construct(
      source.get_arena(), members, root);
  ASSERT(package.is<Tetrodotoxin::Model::Package>());
  Manifest manifest(
      "Example.Deep"_view, Version(1, 0), View::Vector<Dependency>());
  Dynamic::Bytes buffer = write_package(
      manifest, package.assume<Tetrodotoxin::Model::Package>(), {});
  ASSERT_NOT(buffer.is_empty());

  Allocator::Arena restored_arena;
  const Abstract& restored =
      Tetrodotoxin::Archiver::Reader(buffer).read_package(
          restored_arena, manifest,
          View::Vector<Reference<Tetrodotoxin::Model::Package>>());
  ASSERT(restored.is<Tetrodotoxin::Model::Package>());
  const Abstract* selected = &restored;
  for (Count i = 0; i < depth; i++) {
    selected = &selected->resolve_context("Node"_view);
    ASSERT(selected->is<Tetrodotoxin::Model::Namespace>());
  }
  EXPECT(selected->resolve_context("Node"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(ArchiverTests, cyclic_graph_is_invalid) {
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {}, "cycle.ttx"_view);
  Tetrodotoxin::Model::Namespace root(source.get_arena(), {});
  Tetrodotoxin::Model::Namespace first(source.get_arena(), "First"_view);
  Tetrodotoxin::Model::Namespace second(source.get_arena(), "Second"_view);
  ASSERT(first.add_export(second));
  ASSERT(second.add_export(first));
  ASSERT(root.add_export(first));

  const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 1> members = {{
    source,
  }};
  const Abstract& package = Tetrodotoxin::Model::Packages::Sources::construct(
      source.get_arena(), members, root);
  ASSERT(package.is<Tetrodotoxin::Model::Package>());
  Manifest manifest(
      "Example.Cycle"_view, Version(1, 0), View::Vector<Dependency>());
  Dynamic::Bytes buffer = write_package(
      manifest, package.assume<Tetrodotoxin::Model::Package>(), {});
  ASSERT_NOT(buffer.is_empty());

  Allocator::Arena restored_arena;
  EXPECT(
      Tetrodotoxin::Archiver::Reader(buffer)
          .read_package(
              restored_arena, manifest,
              View::Vector<Reference<Tetrodotoxin::Model::Package>>())
          .is<Invalid>());
}
