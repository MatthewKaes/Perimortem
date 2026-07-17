// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Puffer;
using namespace Validation;

static constexpr View::Bytes disk_root =
    ".bin/bin/validation/ttx_res_disk_root.ttx"_view;
static constexpr View::Bytes disk_dep1 =
    ".bin/bin/validation/ttx_res_disk_dep1.ttx"_view;
static constexpr View::Bytes disk_dep2 =
    ".bin/bin/validation/ttx_res_disk_dep2.ttx"_view;
static constexpr View::Bytes disk_dep3 =
    ".bin/bin/validation/ttx_res_disk_dep3.ttx"_view;
static constexpr View::Bytes cycle_root =
    ".bin/bin/validation/ttx_res_cycle_root.ttx"_view;
static constexpr View::Bytes cycle_dep1 =
    ".bin/bin/validation/ttx_res_cycle_dep1.ttx"_view;
static constexpr View::Bytes cycle_dep2 =
    ".bin/bin/validation/ttx_res_cycle_dep2.ttx"_view;
static constexpr View::Bytes cycle_dep3 =
    ".bin/bin/validation/ttx_res_cycle_dep3.ttx"_view;

static auto remove_disk_sources() -> void {
  File::remove(disk_root);
  File::remove(disk_dep1);
  File::remove(disk_dep2);
  File::remove(disk_dep3);
  File::remove(cycle_root);
  File::remove(cycle_dep1);
  File::remove(cycle_dep2);
  File::remove(cycle_dep3);
}

static Harness TetrodotoxinResolution = {
  .name = "Puffer::Resolution"_view,
  .setup = []() { remove_disk_sources(); },
  .teardown = []() { remove_disk_sources(); },
};

static constexpr View::Bytes library_source = "dialect : Library;\n"_view;
static constexpr View::Bytes memory_a_source =
    "dialect : Library;\n"
    "import B : Library = \"b.ttx\";\n"_view;
static constexpr View::Bytes memory_b_cycle_source =
    "dialect : Library;\n"
    "import A : Library = \"a.ttx\";\n"_view;
static constexpr View::Bytes memory_c_source =
    "dialect : Library;\n"
    "import A : Library = \"a.ttx\";\n"_view;
static constexpr View::Bytes simple_render_source =
    "dialect : Render;\n"
    "public Render2D : Render {\n"
    "  public vertex : stage {\n"
    "    input [];\n"
    "    output [];\n"
    "  }\n"
    "}\n"_view;
static constexpr View::Bytes simple_shader_source =
    "dialect : Shader;\n"
    "import Renderer : Render = \"render.ttx\";\n"
    "shader B : Renderer::Render2D {\n"
    "  func vertex[] -> [] {\n"
    "    return;\n"
    "  }\n"
    "}\n"_view;

static auto write_source(View::Bytes source_path, View::Bytes source) -> Bool {
  return File::write(source, source_path);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, source_roots) {
  Resolution::Source::Roots roots;

  EXPECT(roots.include("packages/user/ui/package.ttx"_view));
  EXPECT(roots.include("packages/user/core/package.ttx"_view));
  EXPECT(roots.contains("packages/user/shared/types.ttx"_view));
  EXPECT(roots.include("unit/root.ttx"_view));
  EXPECT_NOT(roots.contains("tetrodotoxin/standard/private.ttx"_view));
  EXPECT(roots.include("generated/root.ttx"_view));
  EXPECT(roots.contains("generated/source.ttx"_view));
}

static auto register_package(
    Resolution::Resolver& resolver,
    Resolution::Resolver::Context& context,
    View::Bytes path) -> Bool {
  auto package_buffer = File::read(path);
  return !package_buffer.is_empty() &&
         resolver.register_package_buffer(context, path, package_buffer);
}

static auto register_standard_packages(
    Resolution::Resolver& resolver,
    Resolution::Resolver::Context& context) -> Bool {
  return register_package(
             resolver, context,
             ".bin/bin/tetrodotoxin/standard/Perimortem.Math/"
             "binary_archive.puffer"_view) &&
         register_package(
             resolver, context,
             ".bin/bin/tetrodotoxin/standard/Perimortem.Runtime/"
             "binary_archive.puffer"_view) &&
         register_package(
             resolver, context,
             ".bin/bin/tetrodotoxin/standard/Perimortem.Graphics/"
             "binary_archive.puffer"_view);
}

static auto error_path(
    const Resolution::Resolver::Context& source_context,
    Count index = 0) -> View::Bytes {
  return source_context.get_errors()[index].get_source_path();
}

static auto error_message(
    const Resolution::Resolver::Context& source_context,
    Count index = 0) -> View::Bytes {
  return source_context.get_errors()[index].get_message();
}

static auto first_error_is(
    const Resolution::Resolver::Context& source_context,
    View::Bytes source_path,
    View::Bytes message) -> Bool {
  return source_context.get_errors().get_size() == 1 &&
         error_path(source_context) == source_path &&
         error_message(source_context) == message;
}

static auto has_error(
    const Resolution::Resolver::Context& source_context,
    View::Bytes source_path,
    View::Bytes message) -> Bool {
  for (Count i = 0; i < source_context.get_errors().get_size(); i++) {
    if (error_path(source_context, i) == source_path &&
        error_message(source_context, i) == message) {
      return True;
    }
  }

  return False;
}

static auto import_count(const Resolution::Source::Record* record) -> Count {
  return record->get_imports().get_size();
}

static auto import_name(const Resolution::Source::Record* record, Count index)
    -> View::Bytes {
  return record->get_imports()[index].get_source_name();
}

static auto root_type(const Resolution::Source::Record* record)
    -> const Ttx::Type* {
  if (record == nullptr) {
    return nullptr;
  }

  return &record->get_type();
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, package_imports) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(register_standard_packages(resolver, dependency_context));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;

  const Resolution::Source::Record* root_record = resolver.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = Perimortem.Graphics;\n"
      "import Math : Package = Perimortem.Math;\n"
      "import Runtime : Package = Perimortem.Runtime;\n"_view);
  ASSERT(root_record != nullptr);
  EXPECT_NOT(root_source_context.has_errors());
  EXPECT_EQ(import_count(root_record), Count(3));

  const Resolution::Source::Record* package_record =
      resolver.resolve("Perimortem.Graphics"_view);
  ASSERT(package_record != nullptr);
  EXPECT_EQ(import_count(package_record), Count(1));
  EXPECT_TEXT(import_name(package_record, 0), "Perimortem.Math"_view);
  EXPECT(resolver.resolve("Perimortem.Math"_view));
  EXPECT(resolver.resolve("Perimortem.Runtime"_view));

  EXPECT_TEXT(import_name(root_record, 0), "Perimortem.Graphics"_view);
  EXPECT_TEXT(import_name(root_record, 1), "Perimortem.Math"_view);
  EXPECT_TEXT(import_name(root_record, 2), "Perimortem.Runtime"_view);
  EXPECT(resolver.resolve(import_name(root_record, 0)) == package_record);
  EXPECT_NOT(resolver.resolve("TTX.Graphics"_view));
  EXPECT_NOT(resolver.resolve(
      "tetrodotoxin/standard/perimortem/graphics/shaders/default_2d.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, missing_imports) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context root_source_context;
  EXPECT_NOT(resolver.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import MissingA : Library = \"missing_a.ttx\";\n"
      "import MissingB : Library = \"missing_b.ttx\";\n"_view));

  ASSERT_EQ(root_source_context.get_errors().get_size(), Count(2));
  EXPECT(has_error(
      root_source_context, "unit/missing_a.ttx"_view,
      "Imported source file could not be read."_view));
  EXPECT(has_error(
      root_source_context, "unit/missing_b.ttx"_view,
      "Imported source file could not be read."_view));

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, path_slashes) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context bad_path_source_context;
  EXPECT_NOT(resolver.load_source(
      bad_path_source_context, "unit\\root.ttx"_view, library_source));
  EXPECT(first_error_is(
      bad_path_source_context, "unit\\root.ttx"_view,
      "File paths must use `/` separators."_view));
  Resolution::Resolver::Context bad_import_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_import_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Bad : Library = \"bad\\dep.ttx\";\n"_view));
  EXPECT(first_error_is(
      bad_import_source_context, "unit/root.ttx"_view,
      "File paths must use `/` separators."_view));
  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, error_source_view) {
  static constexpr View::Bytes source =
      "dialect : Library;\n"
      "bad source with enough text to notice if it is copied per error\n"_view;
  Resolution::Resolver::Context error_source_context;

  error_source_context.persist_errors(
      Ttx::Lexical::Errors::Error(
          "unit/root.ttx"_view, source, "First error."_view));
  error_source_context.persist_errors(
      Ttx::Lexical::Errors::Error(
          "unit/root.ttx"_view, source, "Second error."_view));

  ASSERT_EQ(error_source_context.get_errors().get_size(), Count(2));
  EXPECT(
      error_source_context.get_errors()[0].get_source().get_data() ==
      source.get_data());
  EXPECT(
      error_source_context.get_errors()[1].get_source().get_data() ==
      source.get_data());
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, parse_error_retry) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context invalid_root_source_context;
  EXPECT_NOT(resolver.load_source(
      invalid_root_source_context, "unit/root.ttx"_view, "dialect : ;\n"_view));
  EXPECT(invalid_root_source_context.has_errors());
  Resolution::Resolver::Context retried_root_source_context;

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT_NOT(retried_root_source_context.has_errors());

  ASSERT(resolver.load_source(
      retried_root_source_context, "unit/root.ttx"_view, library_source));
  Resolution::Resolver::Context memory_source_context;
  EXPECT(resolver.resolve("unit/root.ttx"_view));
  EXPECT_NOT(memory_source_context.has_errors());

  const Resolution::Source::Record* record = resolver.load_source(
      memory_source_context, "unit/memory.ttx"_view, library_source);
  ASSERT(record != nullptr);
  EXPECT_TEXT(record->get_dialect().get_name(), "Library"_view);
  EXPECT_EQ(import_count(record), Count(0));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, missing_isa) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context missing_isa_source_context;

  EXPECT_NOT(resolver.load_source(
      missing_isa_source_context, "unit/root.ttx"_view,
      "dialect : Missing;\n"_view));
  EXPECT(first_error_is(
      missing_isa_source_context, "unit/root.ttx"_view,
      "ISA `Missing` is not installed. Installed ISAs: App, Library, "
      "Package, Render, Scene, Shader."_view));
  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, isa_mismatch) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context target_source_context;
  ASSERT(resolver.load_source(
      target_source_context, "unit/target.ttx"_view, library_source));
  EXPECT_NOT(target_source_context.has_errors());
  Resolution::Resolver::Context root_source_context;

  EXPECT_NOT(resolver.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Target : Shader = \"target.ttx\";\n"_view));

  EXPECT(first_error_is(
      root_source_context, "unit/root.ttx"_view,
      "Imported source ISA does not match the requested ISA."_view));

  EXPECT_NOT(resolver.resolve("unit/root.ttx"_view));
  EXPECT(resolver.resolve("unit/target.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, bad_import_diags) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context malformed_source_context;

  EXPECT_NOT(resolver.load_source(
      malformed_source_context, "unit/malformed.ttx"_view,
      "dialect : Library;\n"
      "import Bad : Library = 10;\n"_view));
  EXPECT(first_error_is(
      malformed_source_context, "unit/malformed.ttx"_view,
      "Unknown import semantics. Expected either a string path or a package "
      "name."_view));

  Resolution::Resolver::Context a_source_context;
  ASSERT(resolver.load_source(
      a_source_context, "unit/a.ttx"_view, library_source));
  EXPECT_NOT(a_source_context.has_errors());
  Resolution::Resolver::Context b_source_context;
  ASSERT(resolver.load_source(
      b_source_context, "unit/b.ttx"_view, library_source));
  EXPECT_NOT(b_source_context.has_errors());

  Resolution::Resolver::Context duplicate_source_context;
  EXPECT_NOT(resolver.load_source(
      duplicate_source_context, "unit/duplicate.ttx"_view,
      "dialect : Library;\n"
      "import Same : Library = \"a.ttx\";\n"
      "import Same : Library = \"b.ttx\";\n"_view));
  EXPECT(first_error_is(
      duplicate_source_context, "unit/duplicate.ttx"_view,
      "Import type name is already defined."_view));

  Resolution::Resolver::Context dependency_context;
  ASSERT(register_standard_packages(resolver, dependency_context));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context wrong_isa_source_context;
  EXPECT_NOT(resolver.load_source(
      wrong_isa_source_context, "unit/wrong_isa.ttx"_view,
      "dialect : Library;\n"
      "import Math : Library = Perimortem.Math;\n"_view));
  EXPECT(first_error_is(
      wrong_isa_source_context, "unit/wrong_isa.ttx"_view,
      "Imported source ISA does not match the requested ISA."_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, memory_only_cache) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context missing_b_source_context;
  // A memory source is not cached until its imports resolve, so A cannot appear
  // in the graph before B exists somewhere useful.
  EXPECT_NOT(resolver.load_source(
      missing_b_source_context, "unit/a.ttx"_view, memory_a_source));
  EXPECT(has_error(
      missing_b_source_context, "unit/b.ttx"_view,
      "Imported source file could not be read."_view));
  Resolution::Resolver::Context missing_a_source_context;

  // The failed A load left no cache record, so B cannot close the cycle through
  // a stale in-memory A.
  EXPECT_NOT(resolver.load_source(
      missing_a_source_context, "unit/b.ttx"_view, memory_b_cycle_source));
  EXPECT(has_error(
      missing_a_source_context, "unit/a.ttx"_view,
      "Imported source file could not be read."_view));

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT_NOT(resolver.resolve("unit/b.ttx"_view));

  // Once B is valid, A can depend on the cached record even though B is not on
  // disk.
  Resolution::Resolver::Context b_source_context;
  ASSERT(resolver.load_source(
      b_source_context, "unit/b.ttx"_view, library_source));
  EXPECT_NOT(b_source_context.has_errors());

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view));

  Resolution::Resolver::Context a_source_context;
  const Resolution::Source::Record* a = resolver.load_source(
      a_source_context, "unit/a.ttx"_view, memory_a_source);
  ASSERT(a != nullptr);
  EXPECT_NOT(a_source_context.has_errors());

  EXPECT(resolver.resolve("unit/a.ttx"_view) == a);
  EXPECT(resolver.resolve("unit/b.ttx"_view));
  EXPECT_TEXT(import_name(a, 0), "b.ttx"_view);

  // C gives the invalidation path a transitive consumer.
  Resolution::Resolver::Context c_source_context;
  const Resolution::Source::Record* c = resolver.load_source(
      c_source_context, "unit/c.ttx"_view, memory_c_source);
  ASSERT(c != nullptr);
  EXPECT_NOT(c_source_context.has_errors());

  EXPECT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view));
  EXPECT(resolver.resolve("unit/c.ttx"_view) == c);
  EXPECT_TEXT(import_name(c, 0), "a.ttx"_view);

  // B can republish as a valid Shader. A and C still have to leave because
  // their import contracts were built against the old Library record.
  Resolution::Resolver::Context render_source_context;
  ASSERT(resolver.load_source(
      render_source_context, "unit/render.ttx"_view, simple_render_source));
  EXPECT_NOT(render_source_context.has_errors());

  Resolution::Resolver::Context shader_b_source_context;
  const Resolution::Source::Record* shader_b = resolver.load_source(
      shader_b_source_context, "unit/b.ttx"_view, simple_shader_source);
  ASSERT(shader_b != nullptr);
  EXPECT(has_error(
      shader_b_source_context, "unit/a.ttx"_view,
      "Imported source ISA does not match the requested ISA."_view));
  EXPECT(has_error(
      shader_b_source_context, "unit/c.ttx"_view,
      "Couldn't resolve import `unit/a.ttx`."_view));

  EXPECT_NOT(resolver.resolve("unit/a.ttx"_view));
  EXPECT(resolver.resolve("unit/b.ttx"_view) == shader_b);
  EXPECT_NOT(resolver.resolve("unit/c.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, disk_chain) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context root_source_context;

  ASSERT(write_source(
      disk_root,
      "dialect : Library;\n"
      "import Dep1 : Library = \"ttx_res_disk_dep1.ttx\";\n"
      "import Dep2 : Library = \"ttx_res_disk_dep2.ttx\";\n"_view));
  ASSERT(write_source(disk_dep1, library_source));
  ASSERT(write_source(
      disk_dep2,
      "dialect : Library;\n"
      "import Dep1 : Library = \"ttx_res_disk_dep1.ttx\";\n"_view));

  const Resolution::Source::Record* root =
      resolver.load_source(root_source_context, disk_root);
  ASSERT(root != nullptr);
  EXPECT_NOT(root_source_context.has_errors());

  EXPECT(resolver.resolve(disk_root) == root);
  const Resolution::Source::Record* dep1 = resolver.resolve(disk_dep1);
  const Resolution::Source::Record* dep2 = resolver.resolve(disk_dep2);
  ASSERT(dep1 != nullptr);
  ASSERT(dep2 != nullptr);
  EXPECT_EQ(import_count(root), Count(2));
  EXPECT_TEXT(import_name(root, 0), "ttx_res_disk_dep1.ttx"_view);
  EXPECT_TEXT(import_name(root, 1), "ttx_res_disk_dep2.ttx"_view);
  EXPECT_EQ(import_count(dep2), Count(1));
  EXPECT_TEXT(import_name(dep2, 0), "ttx_res_disk_dep1.ttx"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, disk_cycle) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context cycle_root_source_context;

  ASSERT(write_source(
      cycle_root,
      "dialect : Library;\n"
      "import Dep1 : Library = \"ttx_res_cycle_dep1.ttx\";\n"
      "import Dep2 : Library = \"ttx_res_cycle_dep2.ttx\";\n"
      "import Dep3 : Library = \"ttx_res_cycle_dep3.ttx\";\n"_view));
  ASSERT(write_source(
      cycle_dep1,
      "dialect : Library;\n"
      "import Dep2 : Library = \"ttx_res_cycle_dep2.ttx\";\n"_view));
  ASSERT(write_source(
      cycle_dep2,
      "dialect : Library;\n"
      "import Dep1 : Library = \"ttx_res_cycle_dep1.ttx\";\n"_view));
  ASSERT(write_source(cycle_dep3, library_source));

  EXPECT_NOT(resolver.load_source(cycle_root_source_context, cycle_root));
  EXPECT(cycle_root_source_context.has_errors());

  EXPECT_NOT(resolver.resolve(cycle_root));
  EXPECT_NOT(resolver.resolve(cycle_dep1));
  EXPECT_NOT(resolver.resolve(cycle_dep2));
  EXPECT(resolver.resolve(cycle_dep3));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, bad_update) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context root_source_context;

  ASSERT(write_source(
      disk_root,
      "dialect : Library;\n"
      "import Dep1 : Library = \"ttx_res_disk_dep1.ttx\";\n"
      "import Dep2 : Library = \"ttx_res_disk_dep2.ttx\";\n"
      "import Dep3 : Library = \"ttx_res_disk_dep3.ttx\";\n"_view));
  ASSERT(write_source(disk_dep1, library_source));
  ASSERT(write_source(
      disk_dep2,
      "dialect : Library;\n"
      "import Dep1 : Library = \"ttx_res_disk_dep1.ttx\";\n"_view));
  ASSERT(write_source(disk_dep3, library_source));

  const Resolution::Source::Record* root =
      resolver.load_source(root_source_context, disk_root);
  ASSERT(root != nullptr);
  EXPECT_NOT(root_source_context.has_errors());

  EXPECT(resolver.resolve(disk_root) == root);
  EXPECT(resolver.resolve(disk_dep1));
  EXPECT(resolver.resolve(disk_dep2));
  EXPECT(resolver.resolve(disk_dep3));
  const Resolution::Source::Record* dep1 = resolver.resolve(disk_dep1);
  const Resolution::Source::Record* dep2 = resolver.resolve(disk_dep2);
  ASSERT(dep1 != nullptr);
  ASSERT(dep2 != nullptr);
  const auto* dep1_type = &dep1->get_type();
  const auto* dep2_type = &dep2->get_type();

  EXPECT_EQ(import_count(root), Count(3));
  EXPECT_TEXT(import_name(root, 0), "ttx_res_disk_dep1.ttx"_view);
  EXPECT_TEXT(import_name(root, 1), "ttx_res_disk_dep2.ttx"_view);
  EXPECT_TEXT(import_name(root, 2), "ttx_res_disk_dep3.ttx"_view);

  Resolution::Resolver::Context broken_dep1_source_context;
  EXPECT_NOT(resolver.load_source(
      broken_dep1_source_context, disk_dep1, "bad ttx contents"_view));
  EXPECT(broken_dep1_source_context.has_errors());

  // Breaking dep1 removes every consumer that points into it, but dep3 survives
  // because it is not part of that dependency path.
  EXPECT_NOT(resolver.resolve(disk_root));
  EXPECT_NOT(resolver.resolve(disk_dep1));
  EXPECT_NOT(resolver.resolve(disk_dep2));
  EXPECT(resolver.resolve(disk_dep3));

  // Reloading root keeps the valid dep3 record and reloads the records that
  // were removed with dep1.
  Resolution::Resolver::Context reloaded_root_source_context;
  root = resolver.load_source(reloaded_root_source_context, disk_root);
  ASSERT(root != nullptr);
  EXPECT_NOT(reloaded_root_source_context.has_errors());

  EXPECT(resolver.resolve(disk_root) == root);
  EXPECT(resolver.resolve(disk_dep1));
  EXPECT(resolver.resolve(disk_dep2));
  EXPECT(resolver.resolve(disk_dep3));

  // Imports keep their authored spelling. Cache lookups use canonical paths.
  EXPECT_TEXT(import_name(root, 1), "ttx_res_disk_dep2.ttx"_view);
  // Rebuilt records publish new types, even when the source spelling is the
  // same. Consumers must not keep old type addresses alive.
  EXPECT_NOT(dep1_type == &resolver.resolve(disk_dep1)->get_type());
  EXPECT_NOT(dep2_type == &resolver.resolve(disk_dep2)->get_type());
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, package_loading) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(register_standard_packages(resolver, dependency_context));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;
  const Resolution::Source::Record* root = resolver.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = Perimortem.Graphics;\n"
      "private Default2D : alias = Graphics::Shaders::Default2D;\n"_view);
  ASSERT(root != nullptr);
  EXPECT_NOT(root_source_context.has_errors());

  // Package imports resolve through the public package name, while the private
  // source path stays hidden from root lookups.
  const Resolution::Source::Record* graphics =
      resolver.resolve("Perimortem.Graphics"_view);
  ASSERT(graphics != nullptr);
  EXPECT_TEXT(import_name(root, 0), "Perimortem.Graphics"_view);
  EXPECT(resolver.resolve(import_name(root, 0)) == graphics);
  EXPECT_NOT(resolver.resolve(
      "tetrodotoxin/standard/perimortem/graphics/shaders/default_2d.ttx"_view));

  const Ttx::Type* package = root_type(graphics);
  ASSERT(package != nullptr);
  EXPECT_TEXT(package->get_name(), "Package"_view);
  ASSERT_EQ(package->get_types().get_size(), Count(6));
  const Ttx::Type* size_2d = package->find_type("Size2D"_view);
  ASSERT(size_2d != nullptr);
  EXPECT_TEXT(
      size_2d->describe().get_view(),
      "Perimortem.Graphics::Size2D alias of "
      "Perimortem.Math::Geometry::Size2D"_view);
  const Ttx::Type* color = package->find_type("Color"_view);
  ASSERT(color != nullptr);
  EXPECT_TEXT(color->get_name(), "Color"_view);

  const Ttx::Type* render_2d = package->find_type("Render2D"_view);
  ASSERT(render_2d != nullptr);
  EXPECT_TEXT(render_2d->get_name(), "Render2D"_view);

  const Ttx::Type* shaders = package->find_type("Shaders"_view);
  ASSERT(shaders != nullptr);
  ASSERT_EQ(shaders->get_types().get_size(), Count(1));
  const Ttx::Type* default_2d = shaders->find_type("Default2D"_view);
  ASSERT(default_2d != nullptr);
  EXPECT_TEXT(default_2d->get_name(), "Default2D"_view);

  const Resolution::Source::Record* record =
      resolver.resolve("unit/root.ttx"_view);
  ASSERT(record != nullptr);
  EXPECT_EQ(import_count(record), Count(1));
  EXPECT_TEXT(import_name(record, 0), "Perimortem.Graphics"_view);

  Resolution::Resolver::Context direct_import_source_context;
  EXPECT_NOT(resolver.load_source(
      direct_import_source_context, "unit/direct.ttx"_view,
      "dialect : Library;\n"
      "import Default2D : Shader = "
      "\"../tetrodotoxin/standard/perimortem/graphics/shaders/"
      "default_2d.ttx\";\n"_view));
  EXPECT(has_error(
      direct_import_source_context, "unit/direct.ttx"_view,
      "Import source path is outside the source tree."_view));
  EXPECT_NOT(resolver.resolve("unit/direct.ttx"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, package_requires_buffer) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "User.Ui"_view);
  Resolution::Resolver::Context source_context;

  EXPECT_NOT(resolver.load_source(
      source_context, "packages/user/ui/package.ttx"_view,
      "dialect : Package;\n"
      "import Core : Package = User.Core;\n"
      "expose Core : alias = Core;\n"_view));
  EXPECT(first_error_is(
      source_context, "packages/user/ui/package.ttx"_view,
      "Puffer Buffer dependency is not registered: User.Core"_view));
  EXPECT_NOT(resolver.resolve("packages/user/ui/package.ttx"_view));
  EXPECT_NOT(resolver.resolve("User.Ui"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, package_surface) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "User.Types"_view);
  Resolution::Resolver::Context source_context;
  const Resolution::Source::Record* package = resolver.load_source(
      source_context, "user/types/package.ttx"_view,
      "dialect : Package;\n"
      "expose Value : alias = Unsigned_32;\n"
      "expose Geometry : group {\n"
      "  expose Position : alias = Vec2D;\n"
      "}\n"_view);

  ASSERT(package != nullptr);
  EXPECT_NOT(source_context.has_errors());
  const Ttx::Type& package_type = package->get_type();
  const Ttx::Type* value = package_type.find_type("Value"_view);
  const Ttx::Type* geometry = package_type.find_type("Geometry"_view);
  ASSERT(value != nullptr);
  ASSERT(geometry != nullptr);
  EXPECT(value->is_alias());
  EXPECT(geometry->find_type("Position"_view) != nullptr);
  EXPECT_TEXT(
      value->describe().get_view(),
      "User.Types::Value alias of Unsigned_32"_view);
  EXPECT_TEXT(geometry->describe().get_view(), "User.Types::Geometry"_view);
  EXPECT(resolver.resolve("user/types/package.ttx"_view) == package);
  EXPECT_NOT(resolver.resolve("User.Types"_view));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinResolution, package_diagnostics) {
  struct Failure {
    View::Bytes package_name;
    View::Bytes source;
    View::Bytes message;
  };
  constexpr Failure failures[] = {
    {
      {},
      "dialect : Package;\nexpose Value : alias = Unsigned_32;\n"_view,
      "Package compilation requires a package name."_view,
    },
    {
      "User.Kind"_view,
      "dialect : Package;\nexpose Child : Package = Child;\n"_view,
      "Expected package definition kind `alias` or `group`."_view,
    },
    {
      "User.Target"_view,
      "dialect : Package;\nexpose Value : alias;\n"_view,
      "Expected `=` before package export target."_view,
    },
    {
      "User.Unknown"_view,
      "dialect : Package;\nexpose Value : alias = Missing;\n"_view,
      "Package export target could not be resolved."_view,
    },
    {
      "User.Duplicate"_view,
      "dialect : Package;\n"
      "expose Value : alias = Unsigned_32;\n"
      "expose Value : alias = Unsigned_32;\n"_view,
      "Package export name is already defined."_view,
    },
    {
      "User.Group"_view,
      "dialect : Package;\n"
      "expose Types : group {\n"
      "  expose Value : alias = Unsigned_32;\n"
      "  expose Value : alias = Unsigned_32;\n"
      "}\n"_view,
      "Package export name is already defined."_view,
    },
    {
      "User.Group"_view,
      "dialect : Package;\n"
      "expose Types : group {\n"
      "  expose Value : alias = Unsigned_32;\n"_view,
      "Expected `}` after package group declaration."_view,
    },
  };
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  for (Count i = 0; i < Count(sizeof(failures) / sizeof(Failure)); i++) {
    Resolution::Resolver resolver(isa_registry, {}, failures[i].package_name);
    Resolution::Resolver::Context source_context;
    EXPECT_NOT(resolver.load_source(
        source_context, "unit/package.ttx"_view, failures[i].source));
    EXPECT(first_error_is(
        source_context, "unit/package.ttx"_view, failures[i].message));
  }
}
